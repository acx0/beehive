#include "tx_request_64_frame.h"

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
    util::pack_value_as_bytes(frame, destination_address);  // pack 8 byte address as 8 single bytes
    frame.push_back(options);
    frame.insert(frame.end(), rf_data.begin(), rf_data.end());

    return frame;
}
