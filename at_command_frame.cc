#include "at_command_frame.h"

at_command_frame::at_command_frame(const std::string &command, const std::string &parameter)
    : frame_data(api_identifier::at_command), frame_id(get_next_frame_id())
{
    if (command.size() != 2)
    {
        // TODO: throw exception
    }

    at_command[0] = command[0];
    at_command[1] = command[1];

    if (parameter != at_command::REGISTER_QUERY)
    {
        this->parameter = parameter;
    }
}

at_command_frame::operator std::vector<uint8_t>() const
{
    std::vector<uint8_t> frame;

    frame.push_back(api_identifier);
    frame.push_back(frame_id);
    frame.push_back(at_command[0]);
    frame.push_back(at_command[1]);

    for (auto i : parameter)
    {
        frame.push_back(i);
    }

    return frame;
}
