#include "beehive.h"

/*
 * specify length in ctor to prevent char * from being treated as null terminated C string
 *  - prefixed null terminator instructs socket to be created in abstract namespace (not mapped to filesystem)
 *  - suffix added to BEEHIVE_SOCKET_PATH to make testing multiple devices on same machine easier
 */
const std::string beehive::BEEHIVE_SOCKET_PATH = std::string("\0beehive0", 9);
const std::chrono::milliseconds beehive::FRAME_READER_SLEEP_DURATION(25);

beehive::beehive()
    : frame_writer_queue(std::make_shared<threadsafe_blocking_queue<std::shared_ptr<std::vector<uint8_t>>>>()), channel_manager(frame_writer_queue)
{
}

void beehive::run()
{
    if (!xbee.initialize())
    {
        return;
    }

    channel_manager.set_local_address(xbee.get_address());

    std::thread request_handler(&beehive::request_handler, this);
    std::thread frame_processor(&beehive::frame_processor, this);
    std::thread frame_reader(&beehive::frame_reader, this);
    std::thread frame_writer(&beehive::frame_writer, this);

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

bool beehive::try_parse_port(const std::string &str, int &port)
{
    try
    {
        port = std::stoi(str);
        return true;
    }
    catch (const std::exception &e)
    {
        return false;
    }
}

void beehive::request_handler()
{
    LOG("starting request_handler thread");
    int listen_socket_fd = util::create_passive_domain_socket(BEEHIVE_SOCKET_PATH);
    if (listen_socket_fd == -1)
    {
        return;
    }

    while (true)
    {
        int client_socket_fd = util::accept_connection(listen_socket_fd);
        if (client_socket_fd == -1)
        {
            continue;
        }

        std::string request_message = beehive_message::read_message(client_socket_fd);
        LOG("beehive: received request message: [", request_message, "]");

        if (beehive_message::is_message(beehive_message::LISTEN, request_message))
        {
            auto tokens = util::split(request_message, beehive_message::SEPARATOR);

            if (tokens.size() != 2)
            {
                LOG_ERROR("invalid request");
                beehive_message::send_message(client_socket_fd, beehive_message::INVALID);
                continue;
            }

            int port_int;
            if (!try_parse_port(tokens[1], port_int))
            {
                LOG_ERROR("invalid port value");
                beehive_message::send_message(client_socket_fd, beehive_message::INVALID);
                continue;
            }

            if (!port_manager::is_listen_port(port_int))
            {
                LOG_ERROR("invalid port number");
                beehive_message::send_message(client_socket_fd, beehive_message::INVALID);
                continue;
            }

            uint16_t port = static_cast<uint16_t>(port_int);
            channel_manager.try_create_passive_socket(client_socket_fd, port);
        }
        else if (beehive_message::is_message(beehive_message::CONNECT, request_message))
        {
            auto tokens = util::split(request_message, beehive_message::SEPARATOR);

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

            int port_int;
            if (!try_parse_port(tokens[2], port_int))
            {
                LOG_ERROR("invalid port value");
                beehive_message::send_message(client_socket_fd, beehive_message::INVALID);
                continue;
            }

            if (!port_manager::is_listen_port(port_int))
            {
                LOG_ERROR("invalid port number");
                beehive_message::send_message(client_socket_fd, beehive_message::INVALID);
                continue;
            }

            uint16_t port = static_cast<uint16_t>(port_int);
            channel_manager.try_create_active_socket(client_socket_fd, destination_address, port);
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

            uint64_t source_address = rx_packet->get_source_address();
            uint64_t destination_address = rx_packet->is_broadcast_frame() ? xbee_s1::BROADCAST_ADDRESS : xbee.get_address();

            connection_tuple connection_key(source_address, segment->get_source_port(), destination_address, segment->get_destination_port());
            log_segment(connection_key, segment);

            if (destination_address == xbee_s1::BROADCAST_ADDRESS)
            {
            }
            else
            {
                if (segment->get_message_type() == message_segment::type::stream_segment)
                {
                    channel_manager.process_stream_segment(connection_key, segment);
                }
            }
        }
    }
}

void beehive::frame_reader()
{
    LOG("starting frame_reader thread");

    while (true)
    {
        auto frame = xbee.read_frame();
        if (frame == nullptr)
        {
            // note: sleep is needed to prevent xbee_s1::read_frame from keeping access lock held
            // TODO: explicitly schedule frame reads/writes?
            std::this_thread::sleep_for(FRAME_READER_SLEEP_DURATION);
            continue;
        }

        frame_processor_queue.push(frame);
    }
}

void beehive::frame_writer()
{
    LOG("starting frame_writer thread");

    while (true)
    {
        auto frame = frame_writer_queue->wait_and_pop();
        xbee.write_frame(*frame);
    }
}
