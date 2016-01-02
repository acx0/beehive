#include "uart_frame.h"

const uint8_t uart_frame::FRAME_DELIMITER = 0x7e;
const uint8_t uart_frame::ESCAPE = 0x7d;
const uint8_t uart_frame::XON = 0x11;
const uint8_t uart_frame::XOFF = 0x13;
const uint8_t uart_frame::XOR_CONST = 0x20;

uart_frame::uart_frame(std::shared_ptr<frame_data> data)
    : data(data)
{
}

uart_frame::uart_frame(uint8_t length_msb, uint8_t length_lsb, std::shared_ptr<frame_data> data, uint8_t checksum)
    : length_msb(length_msb), length_lsb(length_lsb), data(data), checksum(checksum)
{
}

uint8_t uart_frame::get_api_identifier()
{
    return data->api_identifier;
}

std::shared_ptr<frame_data> uart_frame::get_data()
{
    return data;
}

uint8_t uart_frame::compute_checksum(const std::vector<uint8_t> &payload)
{
    return 0xff - std::accumulate(payload.begin(), payload.end(), static_cast<uint8_t>(0));
}

uart_frame::operator std::vector<uint8_t>() const
{
    std::vector<uint8_t> frame;

    frame.push_back(FRAME_DELIMITER);
    auto payload = static_cast<std::vector<uint8_t>>(*data);
    auto payload_length = static_cast<uint16_t>(payload.size());
    util::pack_value_as_bytes(frame, payload_length);   // pack 2 byte length as 2 single bytes
    frame.insert(frame.end(), payload.begin(), payload.end());
    frame.push_back(compute_checksum(payload));

    return frame;
}
