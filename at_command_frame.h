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
    static const std::vector<uint8_t> REGISTER_QUERY;

    at_command_frame(const std::string &command, const std::vector<uint8_t> &parameter = REGISTER_QUERY);

    const std::string &get_at_command() const;

    operator std::vector<uint8_t>() const override;

private:
    uint8_t frame_id;
    std::string at_command;
    std::vector<uint8_t> parameter;
};

#endif
