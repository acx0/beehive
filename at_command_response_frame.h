#ifndef AT_COMMAND_RESPONSE_FRAME_H
#define AT_COMMAND_RESPONSE_FRAME_H

#include <cstdint>
#include <iostream>
#include <vector>

#include "frame_data.h"

class at_command_response_frame : public frame_data
{
public:
    static const size_t MIN_FRAME_DATA_LENGTH;

    enum status : uint8_t
    {
        ok = 0x00,
        error = 0x01,
        invalid_command = 0x02,
        invalid_parameter = 0x03
    };

    at_command_response_frame(const std::vector<uint8_t> &frame);

    uint8_t get_status();
    const std::vector<uint8_t> &get_value() const;

    operator std::vector<uint8_t>() const override;

private:
    uint8_t frame_id;
    uint8_t at_command[2];
    uint8_t status_value;
    std::vector<uint8_t> value;
};

#endif
