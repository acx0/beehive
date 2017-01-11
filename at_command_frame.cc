#include "at_command_frame.h"

const std::vector<uint8_t> at_command_frame::REGISTER_QUERY;

// TODO: for frame types that can be created as invalid even when using ctor, use helper func that
// validates or throw?
// TODO: could also require at_command_frame to be constructed from at_command, would still have to
// validate in there
at_command_frame::at_command_frame(
    const std::string &command, const std::vector<uint8_t> &parameter, bool test_frame_id)
    : frame_data(api_identifier::at_command), frame_id(test_frame_id ? 1 : get_next_frame_id()),
      at_command(command), parameter(parameter)
{
}

const std::string &at_command_frame::get_at_command() const
{
    return at_command;
}

at_command_frame::operator std::vector<uint8_t>() const
{
    std::vector<uint8_t> frame;

    frame.push_back(api_identifier_value);
    frame.push_back(frame_id);
    frame.insert(frame.end(), at_command.begin(), at_command.end());
    frame.insert(frame.end(), parameter.begin(), parameter.end());

    return frame;
}
