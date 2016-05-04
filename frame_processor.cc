#include "frame_processor.h"

/*
 * specify length in ctor to prevent char * from being treated as null terminated C string
 *  - prefixed null terminator instructs socket to be created in abstract namespace (not mapped to filesystem)
 *  - suffix added to BEEHIVE_SOCKET_PATH to make testing multiple devices on same machine easier
 */
const std::string frame_processor::BEEHIVE_SOCKET_PATH = std::string("\0beehive0", 9);
const std::string frame_processor::COMMUNICATION_PATH_PREFIX = std::string("\0beehive_comm", 13);
const size_t frame_processor::MAX_MESSAGE_SIZE = 100;
const std::string frame_processor::MESSAGE_LISTEN = std::string("LISTEN");
const std::string frame_processor::MESSAGE_CONNECT = std::string("CONNECT");
const std::string frame_processor::MESSAGE_ACCEPT = std::string("ACCEPT");
const std::string frame_processor::MESSAGE_CLOSE = std::string("CLOSE");
const std::string frame_processor::MESSAGE_INVALID = std::string("INVALID");
const std::string frame_processor::MESSAGE_USED = std::string("USED");
const std::string frame_processor::MESSAGE_FAILED = std::string("FAILED");
const std::string frame_processor::MESSAGE_OK = std::string("OK");
const std::string frame_processor::MESSAGE_SEPARATOR = std::string(":");

uint32_t frame_processor::socket_suffix = 0;
std::mutex frame_processor::socket_suffix_lock;

void frame_processor::run()
{
    if (!xbee.initialize())
    {
        return;
    }

    std::thread reader(&frame_processor::frame_reader, this);
    std::thread writer(&frame_processor::frame_writer, this);
    std::thread channel_manager(&frame_processor::channel_manager, this);

    reader.join();
    writer.join();
    channel_manager.join();
}

void frame_processor::log_segment(const connection_tuple &key, std::shared_ptr<message_segment> segment)
{
    std::string flags(4, ' ');

    flags[0] = segment->is_ack() || segment->is_synack() ? 'A' : ' ';
    flags[1] = segment->is_rst() ? 'R' : ' ';
    flags[2] = segment->is_syn() || segment->is_synack() ? 'S' : ' ';
    flags[3] = segment->is_fin() ? 'F' : ' ';

    std::clog << "recv  " << key.to_string() << "[" << flags << "] type(" << +segment->get_message_type() << "), msg ["
        << util::get_frame_hex(segment->get_message()) << "] (" << segment->get_message().size() << " bytes)" << std::endl;
}

uint32_t frame_processor::get_next_socket_suffix()
{
    std::lock_guard<std::mutex> lock(socket_suffix_lock);
    return socket_suffix++;
}

std::string frame_processor::read_message(int socket_fd)
{
    char buffer[MAX_MESSAGE_SIZE];
    ssize_t bytes_read = recv(socket_fd, buffer, sizeof(buffer), 0);

    if (bytes_read == 0)
    {
        std::clog << "remote connection closed" << std::endl;
        return std::string();
    }
    else if (bytes_read == -1)
    {
        perror("recv");
        return std::string();
    }

    return std::string(buffer, bytes_read);
}

void frame_processor::send_message(int socket_fd, const std::string &message)
{
    send(socket_fd, message.c_str(), message.size(), 0);
}

bool frame_processor::is_message(const std::string &message_type, const std::string &request)
{
    return request.size() >= message_type.size() && request.compare(0, message_type.size(), message_type) == 0;
}

bool frame_processor::try_parse_ieee_address(const std::string &str, uint64_t &address)
{
    std::istringstream iss(str);
    return static_cast<bool>(iss >> std::hex >> address);
}

bool frame_processor::try_parse_port(const std::string &str, int &port)
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

void frame_processor::frame_reader()
{
    std::clog << "starting frame_reader thread" << std::endl;

    while (true)
    {
        auto frame = xbee.read_frame();
        if (frame == nullptr)
        {
            // note: sleep is needed to prevent read_frame from keeping access lock held
            // TODO: explicitly schedule frame reads/writes?
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        if (frame->get_api_identifier() == frame_data::api_identifier::rx_packet_64)
        {
            auto rx_packet = std::static_pointer_cast<rx_packet_64_frame>(frame->get_data());
            auto segment = std::make_shared<message_segment>(rx_packet->get_rf_data());

            uint64_t source_address = rx_packet->get_source_address();
            uint64_t destination_address = rx_packet->is_broadcast_frame()
                ? xbee_s1::BROADCAST_ADDRESS
                : xbee.get_address();

            connection_tuple connection_key(source_address, segment->get_source_port(), destination_address, segment->get_destination_port());
            log_segment(connection_key, segment);

            if (destination_address == xbee_s1::BROADCAST_ADDRESS)
            {
            }
            else
            {
                if (segment->get_message_type() == message_segment::type::stream_segment)
                {
                    if (!pm.is_open(segment->get_destination_port()))
                    {
                        std::clog << "discarding frame destined for port " << +segment->get_destination_port() << std::endl;
                        continue;
                    }

                    if (segment->is_syn())
                    {
                        std::thread request_handler(&frame_processor::connection_handler, this, connection_key, segment);
                        request_handler.detach();
                    }
                    else
                    {
                        auto message_queue = segment_queue_map[connection_key];
                        if (message_queue != nullptr)
                        {
                            message_queue->push(segment);
                        }
                    }
                }
            }
        }
    }
}

void frame_processor::frame_writer()
{
    std::clog << "starting frame_writer thread" << std::endl;

    while (true)
    {
        auto frame = write_queue.wait_and_pop();
        xbee.write_frame(*frame);
    }
}

void frame_processor::channel_manager()
{
    std::clog << "starting channel_manager thread" << std::endl;
    int listen_socket_fd = util::create_passive_domain_socket(BEEHIVE_SOCKET_PATH);
    if (listen_socket_fd == -1)
    {
        return;     // TODO: retry logic somwhere...
    }

    while (true)
    {
        int remote_socket_fd = util::accept_connection(listen_socket_fd);
        if (remote_socket_fd == -1)
        {
            continue;
        }

        std::string request_message = read_message(remote_socket_fd);
        std::clog << "beehive: received request message: [" << request_message << "]" << std::endl;

        if (is_message(MESSAGE_LISTEN, request_message))
        {
            auto tokens = util::split(request_message, MESSAGE_SEPARATOR);

            if (tokens.size() != 2)
            {
                std::clog << "invalid request" << std::endl;
                send_message(remote_socket_fd, MESSAGE_INVALID);
                continue;
            }

            int port_int;
            if (!try_parse_port(tokens[1], port_int))
            {
                std::clog << "invalid port value" << std::endl;
                send_message(remote_socket_fd, MESSAGE_INVALID);
                continue;
            }

            if (!port_manager::is_listen_port(port_int))
            {
                std::clog << "invalid port number" << std::endl;
                send_message(remote_socket_fd, MESSAGE_INVALID);
                continue;
            }

            uint16_t port = static_cast<uint16_t>(port_int);
            if (!pm.try_open_listen_port(port))
            {
                std::clog << "port in use" << std::endl;
                send_message(remote_socket_fd, MESSAGE_USED);
                continue;
            }

            connection_requests[port] = std::make_shared<threadsafe_blocking_queue<std::pair<uint64_t, uint16_t>>>();
            send_message(remote_socket_fd, MESSAGE_OK);

            std::thread socket_manager(&frame_processor::passive_socket_manager, this, remote_socket_fd, port);
            socket_manager.detach();
        }
        else if (is_message(MESSAGE_CONNECT, request_message))
        {
            auto tokens = util::split(request_message, MESSAGE_SEPARATOR);

            if (tokens.size() != 3)
            {
                std::clog << "invalid request" << std::endl;
                send_message(remote_socket_fd, MESSAGE_INVALID);
                continue;
            }

            uint64_t destination_address;
            if (!try_parse_ieee_address(tokens[1], destination_address))
            {
                std::clog << "invalid destination address" << std::endl;
                send_message(remote_socket_fd, MESSAGE_INVALID);
                continue;
            }

            int port_int;
            if (!try_parse_port(tokens[2], port_int))
            {
                std::clog << "invalid port value" << std::endl;
                send_message(remote_socket_fd, MESSAGE_INVALID);
                continue;
            }

            if (!port_manager::is_listen_port(port_int))
            {
                std::clog << "invalid port number" << std::endl;
                send_message(remote_socket_fd, MESSAGE_INVALID);
                continue;
            }

            uint16_t port = static_cast<uint16_t>(port_int);
            std::thread socket_manager(&frame_processor::active_socket_manager, this, remote_socket_fd, destination_address, port);
            socket_manager.detach();
        }
    }
}

void frame_processor::passive_socket_manager(int socket_fd, uint16_t listen_port)
{
    std::clog << "starting passive_socket_manager thread for port " << +listen_port << std::endl;
    auto request_queue = connection_requests[listen_port];

    while (true)
    {
        std::string request_message = read_message(socket_fd);
        if (request_message.empty())
        {
            return;
        }

        std::clog << +listen_port << ": received request message: [" << request_message << "]" << std::endl;

        if (is_message(MESSAGE_ACCEPT, request_message))
        {
            std::clog << "waiting for client request on port " << +listen_port << std::endl;
            auto client_info = request_queue->wait_and_pop();

            uint64_t source_address = client_info.first;
            uint16_t source_port = client_info.second;

            std::clog << "received request on port " << +listen_port << " from (dest: " << util::to_hex_string(source_address) << ", port: " << +source_port << ")" << std::endl;
            std::string communication_socket_path = COMMUNICATION_PATH_PREFIX + "/" + util::to_hex_string(source_address) + "/" + std::to_string(source_port) + "/"
                + std::to_string(get_next_socket_suffix());

            int listen_socket_fd = util::create_passive_domain_socket(communication_socket_path);
            if (listen_socket_fd == 1)
            {
                std::clog << "error creating communication socket" << std::endl;
                continue;
            }

            connection_tuple connection_key(source_address, source_port, xbee.get_address(), listen_port);
            send_message(socket_fd, MESSAGE_OK + MESSAGE_SEPARATOR + communication_socket_path);
            std::thread payload_read_handler(&frame_processor::payload_read_handler, this, listen_socket_fd, connection_key);
            payload_read_handler.detach();
        }
    }
}

void frame_processor::active_socket_manager(int control_socket_fd, uint64_t destination_address, uint16_t destination_port)
{
    std::clog << "starting active_socket_manager thread for fd " << control_socket_fd << " (dest: "
        << util::to_hex_string(destination_address) << ", port: " << +destination_port << ")" << std::endl;

    uint16_t source_port;
    if (!pm.try_get_random_ephemeral_port(source_port))
    {
        send_message(control_socket_fd, MESSAGE_FAILED);    // TODO: unique error for port exhaustion ?
        return;
    }

    // note: connection_tuple created based on expected response tuple
    connection_tuple connection_key(destination_address, destination_port, xbee.get_address(), source_port);
    auto segment_queue = segment_queue_map[connection_key] = std::make_shared<threadsafe_blocking_queue<std::shared_ptr<message_segment>>>();

    if (destination_address != xbee.get_address())
    {
        auto segment = message_segment::create_syn(source_port, destination_port);
        uart_frame frame(std::make_shared<tx_request_64_frame>(destination_address, *segment));
        write_queue.push(std::make_shared<std::vector<uint8_t>>(frame));

        auto response = segment_queue->wait_and_pop();
        if (!response->is_synack())
        {
            return;
        }

        segment = message_segment::create_ack(source_port, destination_port);
        frame = uart_frame(std::make_shared<tx_request_64_frame>(destination_address, *segment));
        write_queue.push(std::make_shared<std::vector<uint8_t>>(frame));
    }

    std::string communication_socket_path = COMMUNICATION_PATH_PREFIX + "/" + util::to_hex_string(destination_address) + "/" + std::to_string(destination_port) + "/"
        + std::to_string(get_next_socket_suffix());
    int listen_socket_fd = util::create_passive_domain_socket(communication_socket_path);
    if (listen_socket_fd == 1)
    {
        std::clog << "error creating communication socket" << std::endl;
        return;
    }

    send_message(control_socket_fd, MESSAGE_OK + MESSAGE_SEPARATOR + communication_socket_path);
    std::thread payload_write_handler(&frame_processor::payload_write_handler, this, control_socket_fd, listen_socket_fd, connection_key);
    payload_write_handler.detach();
}

void frame_processor::connection_handler(connection_tuple connection_key, std::shared_ptr<message_segment> segment)
{
    std::clog << "starting connection_handler thread" << std::endl;
    auto segment_queue = segment_queue_map[connection_key] = std::make_shared<threadsafe_blocking_queue<std::shared_ptr<message_segment>>>();

    auto response = message_segment::create_synack(connection_key.destination_port, connection_key.source_port);
    uart_frame frame(std::make_shared<tx_request_64_frame>(connection_key.source_address, *response));
    write_queue.push(std::make_shared<std::vector<uint8_t>>(frame));

    auto message = segment_queue_map[connection_key]->wait_and_pop();
    if (!message->is_ack())
    {
        return;
    }

    auto request_queue = connection_requests[connection_key.destination_port];
    if (request_queue == nullptr)
    {
        std::clog << "request_queue is null" << std::endl;
        return;
    }

    request_queue->push(std::make_pair(connection_key.source_address, connection_key.source_port));
}

void frame_processor::payload_read_handler(int listen_socket_fd, connection_tuple connection_key)
{
    std::clog << "starting payload_read_handler thread" << std::endl;

    int communication_socket_fd = util::accept_connection(listen_socket_fd);
    if (communication_socket_fd == -1)
    {
        std::clog << "error accepting communication connection" << std::endl;
        return;
    }

    std::clog << "client connected to communication socket" << std::endl;

    auto segment_queue = segment_queue_map[connection_key];
    if (segment_queue == nullptr)
    {
        return;
    }

    auto channel = std::make_shared<reliable_channel<uint16_t>>(connection_key, communication_socket_fd, write_queue, segment_queue);
    channel->start_receiving();
}

void frame_processor::payload_write_handler(int control_socket_fd, int listen_socket_fd, connection_tuple connection_key)
{
    int communication_socket_fd = util::accept_connection(listen_socket_fd);
    if (communication_socket_fd == -1)
    {
        std::clog << "error accepting communication connection" << std::endl;
        return;
    }

    std::clog << "client connected to communication socket" << std::endl;
    if (!util::try_configure_nonblocking_receive_timeout(communication_socket_fd))
    {
        return;
    }

    auto segment_queue = segment_queue_map[connection_key];
    if (segment_queue == nullptr)
    {
        return;
    }

    auto channel = std::make_shared<reliable_channel<uint16_t>>(connection_key, communication_socket_fd, write_queue, segment_queue);
    std::thread reliable_sender(&reliable_channel<uint16_t>::start_sending, std::ref(*channel));

    while (true)
    {
        std::string control_message = read_message(control_socket_fd);
        if (control_message.empty())
        {
            continue;
        }

        if (is_message(MESSAGE_CLOSE, control_message))
        {
            channel->request_channel_close();
            break;
        }
    }

    reliable_sender.join();
}
