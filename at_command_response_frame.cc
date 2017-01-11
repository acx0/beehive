#include "at_command_response_frame.h"

const size_t at_command_response_frame::FRAME_ID_OFFSET
    = frame_data::API_IDENTIFIER_OFFSET + sizeof(frame_data::api_identifier_value);
const size_t at_command_response_frame::AT_COMMAND_OFFSET = FRAME_ID_OFFSET + sizeof(frame_id);
const size_t at_command_response_frame::STATUS_OFFSET
    = AT_COMMAND_OFFSET + sizeof(uint16_t) /* at_command */;
const size_t at_command_response_frame::VALUE_OFFSET = STATUS_OFFSET + sizeof(status_value);
// note: value can be empty
const size_t at_command_response_frame::MIN_FRAME_DATA_LENGTH = VALUE_OFFSET;
const std::vector<uint8_t> at_command_response_frame::EMPTY_VALUE;

std::shared_ptr<at_command_response_frame> at_command_response_frame::parse_frame(
    std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end)
{
    auto size = static_cast<size_t>(std::distance(begin, end));
    if (size < MIN_FRAME_DATA_LENGTH)
    {
        return nullptr;
    }

    if (begin[frame_data::API_IDENTIFIER_OFFSET] != api_identifier::at_command_response)
    {
        return nullptr;
    }

    uint8_t frame_id = begin[FRAME_ID_OFFSET];
    std::string at_command(begin + AT_COMMAND_OFFSET, begin + STATUS_OFFSET);
    uint8_t status_value = begin[STATUS_OFFSET];
    std::vector<uint8_t> value;

    if (size > VALUE_OFFSET)
    {
        value = std::vector<uint8_t>(begin + VALUE_OFFSET, end);
    }

    return std::make_shared<at_command_response_frame>(frame_id, at_command, status_value, value);
}

at_command_response_frame::at_command_response_frame(uint8_t frame_id,
    const std::string &at_command, uint8_t status_value, const std::vector<uint8_t> &value)
    : frame_data(api_identifier::at_command_response), frame_id(frame_id), at_command(at_command),
      status_value(status_value), value(value)
{
}

uint8_t at_command_response_frame::get_status()
{
    return status_value;
}

const std::vector<uint8_t> &at_command_response_frame::get_value() const
{
    return value;
}

at_command_response_frame::operator std::vector<uint8_t>() const
{
    std::vector<uint8_t> frame;

    frame.push_back(api_identifier_value);
    frame.push_back(frame_id);
    frame.insert(frame.end(), at_command.cbegin(), at_command.cend());
    frame.push_back(status_value);
    frame.insert(frame.end(), value.cbegin(), value.cend());

    return frame;
}
