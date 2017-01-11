#include "simulated_communication_endpoint.h"

uint64_t simulated_communication_endpoint::get_random_address()
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());

    // TODO: can generate address = (CONST_XBEE_SH_32_BITS + rand(32_bits))
    return dist(mt);
}

simulated_communication_endpoint::simulated_communication_endpoint()
    : address(get_random_address())
{
    socket_fd = util::create_active_abstract_domain_socket(
        beehive_config::BROADCAST_SERVER_SOCKET_PATH, SOCK_SEQPACKET);
    if (socket_fd == -1)
    {
        // TODO
        throw std::exception();
    }

    // register with broadcast server by sending local address
    std::vector<uint8_t> buffer;
    util::pack_value_as_bytes(std::back_inserter(buffer), address);
    send(socket_fd, buffer.data(), buffer.size(), 0);
}

uint64_t simulated_communication_endpoint::get_address()
{
    return address;
}

void simulated_communication_endpoint::transmit_frame(const std::vector<uint8_t> &payload)
{
    LOG("sim_write: [", util::get_frame_hex(payload), "]");
    send(socket_fd, payload.data(), payload.size(), 0);
}

// TODO: change signature to return failure status as bool and frame payload as out param? (0 sized
// payload is 'valid')
std::shared_ptr<uart_frame> simulated_communication_endpoint::receive_frame()
{
    int error;
    std::vector<uint8_t> buffer;
    ssize_t bytes_read
        = util::nonblocking_recv(socket_fd, buffer, uart_frame::MAX_FRAME_SIZE, error);

    if (bytes_read == 0)
    {
        // TODO: this should be fatal error/cause exit?
        return nullptr;
    }
    else if (bytes_read == -1)
    {
        if (error == EAGAIN)
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(25));    // TODO: make configurable in beehive_config?
            return nullptr;
        }

        return nullptr;    // TODO: fatal
    }

    LOG("sim_read:  [", util::get_frame_hex(buffer), "]");

    std::shared_ptr<uart_frame> frame = uart_frame::parse_frame(buffer.begin(), buffer.end());
    if (frame == nullptr)
    {
        return nullptr;
    }

    // TODO: util method verify_checksum
    uint8_t received_checksum = *(buffer.end() - 1);
    uint8_t calculated_checksum
        = uart_frame::compute_checksum(buffer.begin() + uart_frame::HEADER_LENGTH,
            buffer.end() - 1);    // ignore frame header and trailing checksum

    if (calculated_checksum != received_checksum)
    {
        LOG_ERROR("invalid checksum");
        return nullptr;
    }

    return frame;
}
