#include "at_command_response_frame.h"

// api_identifier + frame_id + at_command (2 bytes) + status
// note: value can be empty
const size_t at_command_response_frame::MIN_FRAME_DATA_LENGTH = 5;

at_command_response_frame::at_command_response_frame(const std::vector<uint8_t> &frame)
    : frame_data(api_identifier::at_command_response)
{
    if (frame[0] != api_identifier::at_command_response)
    {
        // TODO: exception
        std::cerr << "not an at_command_response frame" << std::endl;
    }

    if (frame.size() < MIN_FRAME_DATA_LENGTH)
    {
        // TODO: exception
        std::cerr << "invalid frame size" << std::endl;
    }

    frame_id = frame[1];
    at_command[0] = frame[2];
    at_command[1] = frame[3];
    status = frame[4];

    if (frame.size() > MIN_FRAME_DATA_LENGTH)
    {
        value = std::vector<uint8_t>(frame.begin() + MIN_FRAME_DATA_LENGTH, frame.end());
    }
}

uint8_t at_command_response_frame::get_status()
{
    return status;
}

const std::vector<uint8_t> &at_command_response_frame::get_value() const
{
    return value;
}

at_command_response_frame::operator std::vector<uint8_t>() const
{
    std::vector<uint8_t> frame;

    frame.push_back(api_identifier);
    frame.push_back(frame_id);
    frame.insert(frame.end(), std::begin(at_command), std::end(at_command));
    frame.push_back(status);
    frame.insert(frame.end(), value.begin(), value.end());

    return frame;
}
