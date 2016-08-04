#include "at_command_response_frame.h"

// note: value can be empty
const size_t at_command_response_frame::MIN_FRAME_DATA_LENGTH = sizeof(api_identifier_value) + sizeof(frame_id) + sizeof(at_command) + sizeof(status_value);

std::shared_ptr<at_command_response_frame> at_command_response_frame::parse_frame(std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end)
{
    if (end - begin < MIN_FRAME_DATA_LENGTH)
    {
        return nullptr;
    }

    if (begin[0] != api_identifier::at_command_response)
    {
        return nullptr;
    }

    uint8_t frame_id = begin[1];
    uint16_t at_command = util::unpack_bytes_to_width<uint16_t>(begin + 2);
    uint8_t status_value = begin[4];
    std::vector<uint8_t> value;

    if (end - begin > MIN_FRAME_DATA_LENGTH)
    {
        value = std::vector<uint8_t>(begin + MIN_FRAME_DATA_LENGTH, end);
    }

    return std::make_shared<at_command_response_frame>(frame_id, at_command, status_value, value);
}

at_command_response_frame::at_command_response_frame(uint8_t frame_id, uint16_t at_command, uint8_t status_value, const std::vector<uint8_t> &value)
    : frame_data(api_identifier::at_command_response), frame_id(frame_id), at_command(at_command), status_value(status_value), value(value)
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
    util::pack_value_as_bytes(std::back_inserter(frame), at_command);
    frame.push_back(status_value);
    frame.insert(frame.end(), value.begin(), value.end());

    return frame;
}
