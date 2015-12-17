#ifndef AT_COMMAND_FRAME_H
#define AT_COMMAND_FRAME_H

#include <cstdint>
#include <string>
#include <vector>

#include "at_command.h"
#include "frame_data.h"

class at_command_frame : public frame_data
{
public:
    at_command_frame(const std::string &command, const std::string &parameter = at_command::REGISTER_QUERY);

    operator std::vector<uint8_t>() const override;

private:
    uint8_t frame_id;
    uint8_t at_command[2];
    std::string parameter;
};

#endif
