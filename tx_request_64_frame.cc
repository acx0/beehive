#include "tx_request_64_frame.h"

const uint64_t tx_request_64_frame::BROADCAST_ADDRESS = 0xffff;

tx_request_64_frame::tx_request_64_frame(uint64_t destination_address, const std::vector<uint8_t> &rf_data)
    : frame_data(api_identifier::tx_request_64), frame_id(get_next_frame_id()),
        destination_address(destination_address), options(options::none), rf_data(rf_data)
{
}

tx_request_64_frame::operator std::vector<uint8_t>() const
{
    std::vector<uint8_t> frame;

    frame.push_back(api_identifier);
    frame.push_back(frame_id);

    // pack 8 byte address as 8 single bytes, MSB first
    uint64_t mask = 0xff;
    for (int i = 7; i >= 0; --i)
    {
        auto shift = sizeof(uint8_t) * i * 8;
        frame.push_back((destination_address & (mask << shift)) >> shift);
    }

    frame.push_back(options);

    for (auto i : rf_data)
    {
        frame.push_back(i);
    }

    return frame;
}
