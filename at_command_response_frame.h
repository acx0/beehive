#ifndef AT_COMMAND_RESPONSE_FRAME_H
#define AT_COMMAND_RESPONSE_FRAME_H

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "frame_data.h"

class at_command_response_frame : public frame_data
{
public:
    static const size_t MIN_FRAME_DATA_LENGTH;

    at_command_response_frame(const std::vector<uint8_t> &frame);

    operator std::vector<uint8_t>() const override;

private:
    uint8_t frame_id;
    uint8_t at_command[2];
    uint8_t status;
    std::string value;     // TODO: string or vector<uint8_t>?
};

#endif
