#include "tx_request_64_frame.h"

// TODO: verify if rf_data can be empty
const size_t tx_request_64_frame::MIN_FRAME_DATA_LENGTH = sizeof(api_identifier) + sizeof(frame_id) + sizeof(destination_address) + sizeof(options_value);

tx_request_64_frame::tx_request_64_frame(uint64_t destination_address, const std::vector<uint8_t> &rf_data, bool enable_response_frame)
    : frame_data(api_identifier::tx_request_64), frame_id(enable_response_frame ? get_next_frame_id() : frame_data::FRAME_ID_DISABLE_RESPONSE_FRAME),
        destination_address(destination_address), options_value(options::disable_ack), rf_data(rf_data)
{
}

tx_request_64_frame::tx_request_64_frame(std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end)
    : frame_data(api_identifier::tx_request_64)
{
    if (end - begin < MIN_FRAME_DATA_LENGTH)
    {
        // TODO: move into parse method so we can return error here
    }

    if (begin[0] != api_identifier::tx_request_64)
    {
    }

    frame_id = begin[1];

    // unpack 8 byte address from bytes 2-9
    auto offset = 2;
    destination_address = util::unpack_bytes_to_width<uint64_t>(begin + offset);
    options_value = begin[10];

    if (end - begin > MIN_FRAME_DATA_LENGTH)
    {
        rf_data = std::vector<uint8_t>(begin + MIN_FRAME_DATA_LENGTH, end);
    }
}

uint64_t tx_request_64_frame::get_destination_address() const
{
    return destination_address;
}

const std::vector<uint8_t> &tx_request_64_frame::get_rf_data() const
{
    return rf_data;
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
