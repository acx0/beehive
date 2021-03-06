#include "uart_frame.h"

const uint8_t uart_frame::FRAME_DELIMITER = 0x7e;
const uint8_t uart_frame::ESCAPE = 0x7d;
const uint8_t uart_frame::XON = 0x11;
const uint8_t uart_frame::XOFF = 0x13;
const uint8_t uart_frame::XOR_CONST = 0x20;
const uint8_t uart_frame::CHECKSUM_TARGET = 0xff;
const size_t uart_frame::FRAME_DELIMITER_OFFSET = 0;
const size_t uart_frame::LENGTH_MSB_OFFSET = FRAME_DELIMITER_OFFSET + sizeof(FRAME_DELIMITER);
const size_t uart_frame::LENGTH_LSB_OFFSET = LENGTH_MSB_OFFSET + sizeof(length_msb);
const size_t uart_frame::API_IDENTIFIER_OFFSET = LENGTH_LSB_OFFSET + sizeof(length_lsb);
const size_t uart_frame::IDENTIFIER_DATA_OFFSET
    = API_IDENTIFIER_OFFSET + sizeof(frame_data::api_identifier_value);
const size_t uart_frame::HEADER_LENGTH = API_IDENTIFIER_OFFSET;
// TODO: actually larger than this, depending on smallest valid api frame
const size_t uart_frame::MIN_FRAME_SIZE
    = HEADER_LENGTH + sizeof(frame_data::api_identifier_value) + sizeof(checksum);
// max size seen so far: 115 byte tx_request_64 frame (100 byte payload) -> 115 byte rx_packet_64 frame
const size_t uart_frame::MAX_FRAME_SIZE = 115;

std::shared_ptr<uart_frame> uart_frame::parse_frame(
    std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end)
{
    auto size = static_cast<size_t>(std::distance(begin, end));
    if (size < MIN_FRAME_SIZE || size > MAX_FRAME_SIZE)
    {
        return nullptr;
    }

    if (begin[FRAME_DELIMITER_OFFSET] != FRAME_DELIMITER)
    {
        return nullptr;
    }

    if (util::unpack_bytes_to_width<uint16_t>(begin + LENGTH_MSB_OFFSET)
        != std::distance(begin + API_IDENTIFIER_OFFSET, end - 1))
    {
        return nullptr;
    }

    std::shared_ptr<frame_data> data;
    switch (begin[API_IDENTIFIER_OFFSET])
    {
        case frame_data::api_identifier::tx_request_64:
            data = tx_request_64_frame::parse_frame(begin + API_IDENTIFIER_OFFSET, end - 1);
            break;

        case frame_data::api_identifier::rx_packet_64:
            data = rx_packet_64_frame::parse_frame(begin + API_IDENTIFIER_OFFSET, end - 1);
            break;

        case frame_data::api_identifier::at_command_response:
            data = at_command_response_frame::parse_frame(begin + API_IDENTIFIER_OFFSET, end - 1);
            break;

        case frame_data::api_identifier::tx_status:
            data = tx_status_frame::parse_frame(begin + API_IDENTIFIER_OFFSET, end - 1);
            break;

        default:
            LOG_ERROR("invalid api identifier value: ", +begin[API_IDENTIFIER_OFFSET]);
            return nullptr;
    }

    if (data == nullptr)
    {
        LOG_ERROR("failed to parse frame");
        return nullptr;
    }

    return std::make_shared<uart_frame>(
        begin[LENGTH_MSB_OFFSET], begin[LENGTH_LSB_OFFSET], data, *(end - 1));
}

uart_frame::uart_frame(std::shared_ptr<frame_data> data)
    : data(data)
{
}

uart_frame::uart_frame(
    uint8_t length_msb, uint8_t length_lsb, std::shared_ptr<frame_data> data, uint8_t checksum)
    : length_msb(length_msb), length_lsb(length_lsb), data(data), checksum(checksum)
{
}

uint8_t uart_frame::get_api_identifier() const
{
    return data->api_identifier_value;
}

std::shared_ptr<frame_data> uart_frame::get_data()
{
    return data;
}

uint8_t uart_frame::compute_checksum(
    std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end)
{
    return CHECKSUM_TARGET - std::accumulate(begin, end, static_cast<uint8_t>(0));
}

uart_frame::operator std::vector<uint8_t>() const
{
    std::vector<uint8_t> frame;

    frame.push_back(FRAME_DELIMITER);
    auto payload = static_cast<std::vector<uint8_t>>(*data);
    auto payload_length = static_cast<uint16_t>(payload.size());
    util::pack_value_as_bytes(
        std::back_inserter(frame), payload_length);    // pack 2 byte length as 2 single bytes
    frame.insert(frame.end(), payload.begin(), payload.end());
    frame.push_back(compute_checksum(payload.begin(), payload.end()));

    return frame;
}
