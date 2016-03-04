#include "frame_processor.h"

/*
 * specify length in ctor to prevent char * from being treated as null terminated C string
 *  - prefixed null terminator instructs socket to be created in abstract namespace (not mapped to filesystem)
 *  - suffix added to BEEHIVE_SOCKET_PATH to make testing multiple devices on same machine easier
 */
const std::string frame_processor::BEEHIVE_SOCKET_PATH = std::string("\0beehive0", 9);
const std::string frame_processor::CONTROL_PATH_PREFIX = std::string("\0beehive_ctrl", 13);
const std::string frame_processor::COMMUNICATION_PATH_PREFIX = std::string("\0beehive_comm", 13);
const size_t frame_processor::MAX_MESSAGE_SIZE = 100;
const std::string frame_processor::MESSAGE_LISTEN = std::string("LISTEN");
const std::string frame_processor::MESSAGE_CONNECT = std::string("CONNECT");
const std::string frame_processor::MESSAGE_ACCEPT = std::string("ACCEPT");
const std::string frame_processor::MESSAGE_INVALID = std::string("INVALID");
const std::string frame_processor::MESSAGE_USED = std::string("USED");
const std::string frame_processor::MESSAGE_FAILED = std::string("FAILED");
const std::string frame_processor::MESSAGE_OK = std::string("OK");
const std::string frame_processor::MESSAGE_SEPARATOR = std::string(":");

uint32_t frame_processor::socket_suffix = 0;
std::mutex frame_processor::socket_suffix_lock;

// TODO: robust recovery
bool frame_processor::try_initialize_hardware()
{
    if (!xbee.enable_api_mode())
    {
        return false;
    }

    if (!xbee.enable_64_bit_addressing())
    {
        return false;
    }

    if (!xbee.read_ieee_source_address())
    {
        return false;
    }

    return true;
}

void frame_processor::run()
{
    std::thread reader(&frame_processor::frame_reader, this);
    std::thread writer(&frame_processor::frame_writer, this);
    std::thread channel_manager(&frame_processor::channel_manager, this);

    reader.join();
    writer.join();
    channel_manager.join();
}

void frame_processor::log_segment(const connection_tuple &key, const message_segment &segment)
{
    std::string flags(4, ' ');

    flags[0] = segment.is_ack() ? 'A' : ' ';
    flags[1] = segment.is_rst() ? 'R' : ' ';
    flags[2] = segment.is_syn() ? 'S' : ' ';
    flags[3] = segment.is_fin() ? 'F' : ' ';

    std::clog << "recv  " << key.to_string() << "[" << flags << "] type(" << +segment.get_message_type() << "), msg ["
        << util::get_frame_hex(segment.get_message()) << "] (" << segment.get_message().size() << " bytes)" << std::endl;
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

void frame_processor::frame_reader()
{
    std::clog << "starting frame_reader thread" << std::endl;

    while (true)
    {
        auto frame = xbee.read_frame();
        if (frame == nullptr)
        {
            continue;
        }

        if (frame->get_api_identifier() == frame_data::api_identifier::rx_packet_64)
        {
            auto rx_packet = std::static_pointer_cast<rx_packet_64_frame>(frame->get_data());
            message_segment segment(rx_packet->get_rf_data());  // TODO: heap alloc?

            uint64_t destination_address = rx_packet->is_broadcast_frame()
                ? xbee_s1::BROADCAST_ADDRESS
                : xbee.get_address();

            connection_tuple key(rx_packet->get_source_address(), segment.get_source_port(), destination_address, segment.get_destination_port());
            log_segment(key, segment);

            if (segment.get_message_type() == message_segment::type::stream_segment)
            {
                std::shared_ptr<reliable_channel> channel;
                if (channel_map.find(key) == channel_map.end())
                {
                    channel_map[key] = std::make_shared<reliable_channel>();
                }

                channel = channel_map[key];
                channel->write(segment);
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
        std::clog << "received request message: [" << request_message << "]" << std::endl;

        if (is_message(MESSAGE_LISTEN, request_message))
        {
            std::clog << "received listen request" << std::endl;
            auto tokens = util::split(request_message, MESSAGE_SEPARATOR);

            if (tokens.size() != 2)
            {
                std::clog << "invalid request" << std::endl;
                send_message(remote_socket_fd, MESSAGE_INVALID);
                continue;
            }

            uint16_t port;

            try
            {
                int port_int = std::stoi(tokens[1]);
                if (!port_manager::is_valid_port(port_int))
                {
                    std::clog << "invalid port number" << std::endl;
                    send_message(remote_socket_fd, MESSAGE_INVALID);
                    continue;
                }

                port = static_cast<uint16_t>(port_int);
            }
            catch (const std::exception &e)
            {
                std::clog << "invalid port value" << std::endl;
                send_message(remote_socket_fd, MESSAGE_INVALID);
                continue;
            }

            if (!pm.try_open_listen_port(port))
            {
                std::clog << "port in use" << std::endl;
                send_message(remote_socket_fd, MESSAGE_USED);
                continue;
            }

            std::string socket_path = CONTROL_PATH_PREFIX + "_" + std::to_string(port);
            int socket_fd = util::create_passive_domain_socket(socket_path);
            if (socket_fd == -1)
            {
                send_message(remote_socket_fd, MESSAGE_FAILED);
                continue;
            }

            std::clog << "created fd " << socket_fd << " -> " << socket_path << std::endl;
            std::string response = MESSAGE_OK + MESSAGE_SEPARATOR + socket_path;
            send_message(remote_socket_fd, response);

            std::thread socket_manager(&frame_processor::passive_socket_manager, this, socket_fd);
            socket_manager.detach();
        }
        else if (is_message(MESSAGE_CONNECT, request_message))
        {
            std::clog << "received connect request" << std::endl;
            auto tokens = util::split(request_message, MESSAGE_SEPARATOR);

            if (tokens.size() != 3)
            {
                std::clog << "invalid request" << std::endl;
                send_message(remote_socket_fd, MESSAGE_INVALID);
                continue;
            }

            uint64_t destination_address;
            uint16_t port;
            std::istringstream iss(tokens[1]);

            if (!(iss >> std::hex >> destination_address))
            {
                std::clog << "invalid destination address" << std::endl;
                send_message(remote_socket_fd, MESSAGE_INVALID);
                continue;
            }

            try
            {
                int port_int = std::stoi(tokens[2]);
                if (!port_manager::is_valid_port(port_int))
                {
                    std::clog << "invalid port number" << std::endl;
                    send_message(remote_socket_fd, MESSAGE_INVALID);
                    continue;
                }

                port = static_cast<uint16_t>(port_int);
            }
            catch (const std::exception &e)
            {
                std::clog << "invalid port value" << std::endl;
                send_message(remote_socket_fd, MESSAGE_INVALID);
                continue;
            }

            std::string socket_path = CONTROL_PATH_PREFIX + "_" + util::to_hex_string(destination_address) + "_"
                + std::to_string(port) + "_" + std::to_string(get_next_socket_suffix());
            int socket_fd = util::create_passive_domain_socket(socket_path);
            if (socket_fd == -1)
            {
                send_message(remote_socket_fd, MESSAGE_FAILED);
                continue;
            }

            std::clog << "created fd " << socket_fd << " -> " << socket_path << std::endl;
            std::string response = MESSAGE_OK + MESSAGE_SEPARATOR + socket_path;
            send_message(remote_socket_fd, response);

            std::thread socket_manager(&frame_processor::active_socket_manager, this, socket_fd, destination_address, port);
            socket_manager.detach();
        }
    }
}

void frame_processor::passive_socket_manager(int socket_fd)
{
    std::clog << "starting passive_socket_manager thread for fd " << socket_fd << std::endl;
    while (true)
    {
        int request_socket_fd = util::accept_connection(socket_fd);
        if (request_socket_fd == -1)
        {
            continue;
        }

        std::string request_message = read_message(request_socket_fd);
        std::clog << "received request message: [" << request_message << "]" << std::endl;

        if (is_message(MESSAGE_ACCEPT, request_message))
        {
            std::clog << "received accept request" << std::endl;
        }
    }
}

void frame_processor::active_socket_manager(int socket_fd, uint64_t destination_address, uint16_t destination_port)
{
    std::clog << "starting active_socket_manager thread for fd " << socket_fd << " (dest: "
        << util::to_hex_string(destination_address) << ", port: " << +destination_port << ")" << std::endl;


    uint16_t source_port;
    if (!pm.try_get_random_ephemeral_port(source_port))
    {
        send_message(socket_fd, MESSAGE_FAILED);    // TODO: unique error for port exhaustion ?
        return;
    }

    message_segment segment(source_port, destination_port, 0, 0, message_segment::type::stream_segment, message_segment::flag::syn, std::vector<uint8_t>());
    uart_frame frame(std::make_shared<tx_request_64_frame>(destination_address, segment));
    auto payload = std::make_shared<std::vector<uint8_t>>(frame);
    write_queue.push(payload);
}
