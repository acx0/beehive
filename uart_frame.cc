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

uint8_t uart_frame::compute_checksum(const std::vector<uint8_t> &payload)
{
    uint8_t checksum = 0;

    for (auto i : payload)
    {
        checksum += i;
    }

    return 0xff - checksum;
}

uart_frame::operator std::vector<uint8_t>() const
{
    std::vector<uint8_t> frame;

    frame.push_back(FRAME_DELIMITER);
    auto payload = static_cast<std::vector<uint8_t>>(*data);
    auto payload_length = static_cast<uint16_t>(payload.size());
    util::pack_value_as_bytes(frame, payload_length);   // pack 2 byte length as 2 single bytes

    for (auto i : payload)
    {
        frame.push_back(i);
    }

    frame.push_back(compute_checksum(payload));
    return frame;
}
