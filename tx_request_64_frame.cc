#include "tx_request_64_frame.h"

tx_request_64_frame::tx_request_64_frame(uint64_t destination_address, const std::vector<uint8_t> &rf_data, bool enable_response_frame)
    : frame_data(api_identifier::tx_request_64), frame_id(enable_response_frame ? get_next_frame_id() : frame_data::FRAME_ID_DISABLE_RESPONSE_FRAME),
        destination_address(destination_address), options_value(options::disable_ack), rf_data(rf_data)
{
}

tx_request_64_frame::operator std::vector<uint8_t>() const
{
    std::vector<uint8_t> frame;

    frame.push_back(api_identifier_value);
    frame.push_back(frame_id);
    util::pack_value_as_bytes(std::back_inserter(frame), destination_address);  // pack 8 byte address as 8 single bytes
    frame.push_back(options_value);
    frame.insert(frame.end(), rf_data.begin(), rf_data.end());

    return frame;
}
