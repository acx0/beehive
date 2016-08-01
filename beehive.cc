#include "beehive.h"

/*
 * specify length in ctor to prevent char * from being treated as null terminated C string
 *  - prefixed null terminator instructs socket to be created in abstract namespace (not mapped to filesystem)
 *  - suffix added to BEEHIVE_SOCKET_PATH to make testing multiple devices on same machine easier
 */
const std::string beehive::BEEHIVE_SOCKET_PATH = std::string("\0beehive0", 9);

beehive::beehive(std::shared_ptr<communication_endpoint> endpoint)
    : endpoint(endpoint), frame_writer_queue(std::make_shared<threadsafe_blocking_queue<std::shared_ptr<std::vector<uint8_t>>>>()), _channel_manager(frame_writer_queue), _datagram_socket_manager(frame_writer_queue)
{
}

void beehive::run()
{
    // TODO: better way to expose these state variables to all classes?
    //  - include header with static vars?
    _channel_manager.set_local_address(endpoint->get_address());

    std::thread request_handler(&beehive::request_handler, this);
    std::thread frame_processor(&beehive::frame_processor, this);
    std::thread frame_reader(&beehive::frame_reader, this);
    std::thread frame_writer(&beehive::frame_writer, this);
    std::thread neighbour_discoverer(&beehive::neighbour_discoverer, this);

    request_handler.join();
    frame_processor.join();
    frame_reader.join();
    frame_writer.join();
}

void beehive::log_segment(const connection_tuple &key, std::shared_ptr<message_segment> segment)
{
    std::string flags(4, ' ');

    flags[0] = segment->is_ack() || segment->is_synack() ? 'A' : ' ';
    flags[1] = segment->is_rst() ? 'R' : ' ';
    flags[2] = segment->is_syn() || segment->is_synack() ? 'S' : ' ';
    flags[3] = segment->is_fin() ? 'F' : ' ';

    LOG("recv  ", key.to_string(), "[", flags, "] type(", +segment->get_message_type(), "), msg [", util::get_frame_hex(segment->get_message()), "] (", segment->get_message().size(), " bytes)");
}

bool beehive::try_parse_ieee_address(const std::string &str, uint64_t &address)
{
    std::istringstream iss(str);
    return static_cast<bool>(iss >> std::hex >> address);
}

bool beehive::try_parse_port(int client_socket_fd, const std::string &str, uint16_t &port)
{
    int port_int;

    try
    {
        port_int = std::stoi(str);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("invalid port value");
        beehive_message::send_message(client_socket_fd, beehive_message::INVALID);
        return false;
    }

    if (!port_manager::is_listen_port(port_int))
    {
        LOG_ERROR("invalid port number");
        beehive_message::send_message(client_socket_fd, beehive_message::INVALID);
        return false;
    }

    port = static_cast<uint16_t>(port_int);
    return true;
}

void beehive::request_handler()
{
    LOG("starting request_handler thread");
    int listen_socket_fd = util::create_passive_domain_socket(BEEHIVE_SOCKET_PATH, SOCK_STREAM);
    if (listen_socket_fd == -1)
    {
        return;     // TODO: retries
    }

    // TODO: threadpool request handlers ?
    while (true)
    {
        int client_socket_fd = util::accept_connection(listen_socket_fd);
        if (client_socket_fd == -1)
        {
            continue;
        }

        std::string request_message = beehive_message::read_message(client_socket_fd);
        LOG("beehive: received request message: [", request_message, "]");
        if (request_message.empty())
        {
            continue;
        }

        auto tokens = util::split(request_message, beehive_message::SEPARATOR);
        if (tokens.empty())
        {
            continue;
        }

        if (tokens[0] == beehive_message::LISTEN)
        {
            if (tokens.size() != 2)
            {
                LOG_ERROR("invalid request");
                beehive_message::send_message(client_socket_fd, beehive_message::INVALID);
                continue;
            }

            uint16_t port;
            if (!try_parse_port(client_socket_fd, tokens[1], port))
            {
                continue;
            }

            _channel_manager.try_create_passive_socket(client_socket_fd, port);
        }
        else if (tokens[0] == beehive_message::LISTEN_DGRAM)
        {
            if (tokens.size() != 2)
            {
                LOG_ERROR("invalid request");
                beehive_message::send_message(client_socket_fd, beehive_message::INVALID);
                continue;
            }

            uint16_t port;
            if (!try_parse_port(client_socket_fd, tokens[1], port))
            {
                continue;
            }

            _datagram_socket_manager.try_create_passive_socket(client_socket_fd, port);
        }
        else if (tokens[0] == beehive_message::CONNECT)
        {
            if (tokens.size() != 3)
            {
                LOG_ERROR("invalid request");
                beehive_message::send_message(client_socket_fd, beehive_message::INVALID);
                continue;
            }

            uint64_t destination_address;
            if (!try_parse_ieee_address(tokens[1], destination_address))
            {
                LOG_ERROR("invalid destination address");
                beehive_message::send_message(client_socket_fd, beehive_message::INVALID);
                continue;
            }

            uint16_t port;
            if (!try_parse_port(client_socket_fd, tokens[2], port))
            {
                continue;
            }

            _channel_manager.try_create_active_socket(client_socket_fd, destination_address, port);
        }
        else if (tokens[0] == beehive_message::SEND_DGRAM)
        {
            _datagram_socket_manager.try_create_active_socket(client_socket_fd);
        }
    }
}

void beehive::frame_processor()
{
    LOG("starting frame_processor thread");

    while (true)
    {
        auto frame = frame_processor_queue.wait_and_pop();

        if (frame->get_api_identifier() == frame_data::api_identifier::rx_packet_64)
        {
            auto rx_packet = std::static_pointer_cast<rx_packet_64_frame>(frame->get_data());
            auto segment = std::make_shared<message_segment>(rx_packet->get_rf_data());

            uint16_t received_checksum = segment->get_checksum();
            uint16_t computed_checksum = segment->compute_checksum();

            if (received_checksum != computed_checksum)
            {
                continue;
            }

            uint64_t source_address = rx_packet->get_source_address();
            uint64_t destination_address = rx_packet->is_broadcast_frame() ? xbee_s1::BROADCAST_ADDRESS : endpoint->get_address();

            connection_tuple connection_key(source_address, segment->get_source_port(), destination_address, segment->get_destination_port());
            log_segment(connection_key, segment);

            switch (segment->get_message_type())
            {
                case message_segment::type::stream_segment:
                    _channel_manager.process_stream_segment(connection_key, segment);
                    break;

                case message_segment::type::datagram_segment:
                    _datagram_socket_manager.process_segment(source_address, segment);
                    break;

                case message_segment::type::neighbour_discovery:
                    process_neighbour_discovery_message(source_address, segment);
                    break;
            }
        }
    }
}

void beehive::frame_reader()
{
    LOG("starting frame_reader thread");

    while (true)
    {
        auto frame = endpoint->receive_frame();
        if (frame != nullptr)
        {
            frame_processor_queue.push(frame);  // TODO: bound this queue to a certain size?
        }
    }
}

void beehive::frame_writer()
{
    LOG("starting frame_writer thread");

    while (true)
    {
        auto frame = frame_writer_queue->wait_and_pop();
        endpoint->transmit_frame(*frame);
    }
}

void beehive::neighbour_discoverer()
{
    LOG("starting neighbour_discoverer thread");

    auto last_expiration_check = std::chrono::system_clock::now();
    auto expiration_check_interval = std::chrono::seconds(10);
    auto expiration_threshold = std::chrono::seconds(10);
    auto segment = std::make_shared<message_segment>(0, 0, 0, 0, message_segment::type::neighbour_discovery, message_segment::flag::none, message_segment::EMPTY_PAYLOAD);
    uart_frame frame(std::make_shared<tx_request_64_frame>(xbee_s1::BROADCAST_ADDRESS, *segment));

    while (true)
    {
        frame_writer_queue->push(std::make_shared<std::vector<uint8_t>>(frame));
        std::this_thread::sleep_for(std::chrono::seconds(5));
        auto now = std::chrono::system_clock::now();

        if (now - last_expiration_check > expiration_check_interval)
        {
            last_expiration_check = now;

            for (auto &neighbour : neighbours.get_data())
            {
                if (now - neighbour.second.timestamp > expiration_threshold)
                {
                    neighbours.erase(neighbour.first);
                }
            }
        }
    }
}

void beehive::process_neighbour_discovery_message(uint64_t source_address, std::shared_ptr<message_segment> segment)
{
    if (segment->flags_empty())
    {
        // discovery request, reply with ack
        auto segment = std::make_shared<message_segment>(0, 0, 0, 0, message_segment::type::neighbour_discovery, message_segment::flag::ack, message_segment::EMPTY_PAYLOAD);
        uart_frame frame(std::make_shared<tx_request_64_frame>(source_address, *segment));
        frame_writer_queue->push(std::make_shared<std::vector<uint8_t>>(frame));
    }
    else if (segment->is_ack())
    {
        LOG("discovered neighbour: ", source_address);
        neighbours[source_address] = neighbour_info{source_address, std::chrono::system_clock::now()};
    }
}
