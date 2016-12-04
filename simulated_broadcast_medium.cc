#include "simulated_broadcast_medium.h"

// TODO: change signature to return failure status as bool and frame payload as out param? (0 sized payload is 'valid')
std::vector<uint8_t> simulated_broadcast_medium::read_frame(int socket_fd)
{
    std::vector<uint8_t> buffer(uart_frame::MAX_FRAME_SIZE);
    ssize_t bytes_read = recv(socket_fd, buffer.data(), buffer.size(), 0);      // TODO: refactor into util::recv

    if (bytes_read <= 0)
    {
        return std::vector<uint8_t>();
    }

    buffer.resize(bytes_read);
    return buffer;
}

// TODO: cleanup socket fds
bool simulated_broadcast_medium::start()
{
    LOG("starting broadcast server");

    int listen_socket_fd = util::create_passive_abstract_domain_socket(beehive_config::BROADCAST_SERVER_SOCKET_PATH, SOCK_SEQPACKET);
    if (listen_socket_fd == -1)
    {
        LOG_ERROR("broadcast server socket creation failed");
        return false;
    }

    while (true)
    {
        int client_socket_fd = util::accept_connection(listen_socket_fd);
        if (client_socket_fd == -1)
        {
            continue;
        }

        std::vector<uint8_t> frame = read_frame(client_socket_fd);

        // first frame is expected to be address of node
        if (frame.size() != sizeof(uint64_t))
        {
            // TODO: send ack back?
            LOG_ERROR("invalid address supplied by node");
            continue;
        }

        uint64_t address = util::unpack_bytes_to_width<uint64_t>(frame.begin());
        node_sockets[address] = client_socket_fd;

        LOG("client ", util::to_hex_string(address), " connected");

        std::thread traffic_forwarder(&simulated_broadcast_medium::node_traffic_forwarder, this, address, client_socket_fd);
        traffic_forwarder.detach();
    }

    return true;
}

void simulated_broadcast_medium::node_traffic_forwarder(uint64_t node_address, int node_socket_fd)
{
    std::vector<uint8_t> buffer;

    while (!(buffer = read_frame(node_socket_fd)).empty())
    {
        std::shared_ptr<uart_frame> frame = uart_frame::parse_frame(buffer.begin(), buffer.end());
        if (frame == nullptr)
        {
            continue;
        }

        if (frame->get_api_identifier() == frame_data::api_identifier::tx_request_64)
        {
            // simulate frame transmission and convert tx_request_64_frame into rx_packet_64_frame
            std::shared_ptr<tx_request_64_frame> tx_frame = std::static_pointer_cast<tx_request_64_frame>(frame->get_data());
            uint64_t destination_address = tx_frame->get_destination_address();
            uint8_t options = destination_address == xbee_s1::BROADCAST_ADDRESS ? (1 << rx_packet_64_frame::options_bit::address_broadcast) : 0;
            uart_frame rx_frame(std::make_shared<rx_packet_64_frame>(node_address, 0, options, tx_frame->get_rf_data()));   // TODO: simulate rssi
            auto payload = static_cast<std::vector<uint8_t>>(rx_frame);

            if (destination_address == xbee_s1::BROADCAST_ADDRESS)
            {
                for (auto &entry : node_sockets.get_data())
                {
                    if (entry.first != node_address)
                    {
                        send(entry.second, payload.data(), payload.size(), 0);  // TODO: util::send
                    }
                }
            }
            else
            {
                int destination_node_socket_fd;
                if (!node_sockets.try_get(destination_address, destination_node_socket_fd))
                {
                    continue;
                }

                send(destination_node_socket_fd, payload.data(), payload.size(), 0);    // TODO: util::send
            }
        }
        else
        {
            LOG_ERROR("unsupported api frame");
        }
    }
}
