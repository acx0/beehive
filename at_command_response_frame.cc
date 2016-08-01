#include "at_command_response_frame.h"

// note: value can be empty
const size_t at_command_response_frame::MIN_FRAME_DATA_LENGTH = sizeof(api_identifier_value) + sizeof(frame_id) + sizeof(at_command) + sizeof(status_value);

at_command_response_frame::at_command_response_frame(std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end)
    : frame_data(api_identifier::at_command_response)
{
    // TODO: move this logic outside of constructor into parse method?
    if (end - begin < MIN_FRAME_DATA_LENGTH)
    {
        // TODO: exception
        std::cerr << "invalid frame size" << std::endl;
    }

    if (begin[0] != api_identifier::at_command_response)
    {
        // TODO: exception
        std::cerr << "not an at_command_response frame" << std::endl;
    }

    frame_id = begin[1];
    at_command[0] = begin[2];
    at_command[1] = begin[3];
    status_value = begin[4];

    if (end - begin > MIN_FRAME_DATA_LENGTH)
    {
        value = std::vector<uint8_t>(begin + MIN_FRAME_DATA_LENGTH, end);
    }
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
    frame.insert(frame.end(), std::begin(at_command), std::end(at_command));
    frame.push_back(status_value);
    frame.insert(frame.end(), value.begin(), value.end());

    return frame;
}
