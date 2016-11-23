#include "simulated_communication_endpoint.h"

uint64_t simulated_communication_endpoint::get_random_address()
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());

    return dist(mt);
}

simulated_communication_endpoint::simulated_communication_endpoint()
    : address(get_random_address())
{
    socket_fd = util::create_active_abstract_domain_socket(beehive_config::BROADCAST_SERVER_SOCKET_PATH, SOCK_SEQPACKET);
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

std::shared_ptr<uart_frame> simulated_communication_endpoint::receive_frame()
{
    std::vector<uint8_t> buffer = simulated_broadcast_medium::read_frame(socket_fd);
    LOG("sim_read:  [", util::get_frame_hex(buffer), "]");

    std::shared_ptr<uart_frame> frame = uart_frame::parse_frame(buffer.begin(), buffer.end());
    if (frame == nullptr)
    {
        return nullptr;
    }

    // TODO: util method verify_checksum
    uint8_t received_checksum = *(buffer.end() - 1);
    uint8_t calculated_checksum = uart_frame::compute_checksum(buffer.begin() + uart_frame::HEADER_LENGTH, buffer.end() - 1);   // ignore frame header and trailing checksum

    if (calculated_checksum != received_checksum)
    {
        LOG_ERROR("invalid checksum");
        return nullptr;
    }

    return frame;
}
