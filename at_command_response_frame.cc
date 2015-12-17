#include "at_command_response_frame.h"

// api_identifier + frame_id + at_command (2 bytes) + status
// note: value can be empty
const size_t at_command_response_frame::MIN_FRAME_DATA_LENGTH = 5;

at_command_response_frame::at_command_response_frame(const std::vector<uint8_t> &frame)
    : frame_data(api_identifier::at_command_response)
{
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
        value = std::string(frame.begin() + 5, frame.end());
    }
}

at_command_response_frame::operator std::vector<uint8_t>() const
{
    std::vector<uint8_t> frame;

    frame.push_back(frame_id);
    frame.push_back(at_command[0]);
    frame.push_back(at_command[1]);
    frame.push_back(status);

    for (auto i : value)
    {
        frame.push_back(i);
    }

    return frame;
}
