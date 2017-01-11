#include "tx_request_64_frame.h"

const size_t tx_request_64_frame::FRAME_ID_OFFSET
    = frame_data::API_IDENTIFIER_OFFSET + sizeof(frame_data::api_identifier_value);
const size_t tx_request_64_frame::DESTINATION_ADDRESS_OFFSET = FRAME_ID_OFFSET + sizeof(frame_id);
const size_t tx_request_64_frame::OPTIONS_OFFSET
    = DESTINATION_ADDRESS_OFFSET + sizeof(destination_address);
const size_t tx_request_64_frame::RF_DATA_OFFSET = OPTIONS_OFFSET + sizeof(options_value);
// TODO: verify if rf_data can be empty
const size_t tx_request_64_frame::MIN_FRAME_DATA_LENGTH = RF_DATA_OFFSET;

std::shared_ptr<tx_request_64_frame> tx_request_64_frame::parse_frame(
    std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end)
{
    auto size = static_cast<size_t>(std::distance(begin, end));
    if (size < MIN_FRAME_DATA_LENGTH)
    {
        return nullptr;
    }

    if (begin[frame_data::API_IDENTIFIER_OFFSET] != api_identifier::tx_request_64)
    {
        return nullptr;
    }

    uint8_t frame_id = begin[FRAME_ID_OFFSET];

    // unpack 8 byte address from bytes 2-9
    uint64_t destination_address
        = util::unpack_bytes_to_width<uint64_t>(begin + DESTINATION_ADDRESS_OFFSET);
    uint8_t options_value = begin[OPTIONS_OFFSET];
    std::vector<uint8_t> rf_data;

    if (size > RF_DATA_OFFSET)
    {
        rf_data = std::vector<uint8_t>(begin + RF_DATA_OFFSET, end);
    }

    return std::make_shared<tx_request_64_frame>(
        frame_id, destination_address, options_value, rf_data);
}

tx_request_64_frame::tx_request_64_frame(
    uint64_t destination_address, const std::vector<uint8_t> &rf_data, bool enable_response_frame)
    : tx_request_64_frame(
          enable_response_frame ? get_next_frame_id() : frame_data::FRAME_ID_DISABLE_RESPONSE_FRAME,
          destination_address, options::disable_ack, rf_data)
{
}

tx_request_64_frame::tx_request_64_frame(uint8_t frame_id, uint64_t destination_address,
    uint8_t options_value, const std::vector<uint8_t> &rf_data)
    : frame_data(api_identifier::tx_request_64), frame_id(frame_id),
      destination_address(destination_address), options_value(options_value), rf_data(rf_data)
{
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
    util::pack_value_as_bytes(
        std::back_inserter(frame), destination_address);    // pack 8 byte address as 8 single bytes
    frame.push_back(options_value);
    frame.insert(frame.end(), rf_data.begin(), rf_data.end());

    return frame;
}
