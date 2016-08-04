#ifndef AT_COMMAND_RESPONSE_FRAME_H
#define AT_COMMAND_RESPONSE_FRAME_H

#include <cstdint>
#include <iterator>
#include <memory>
#include <vector>

#include "frame_data.h"
#include "util.h"

class at_command_response_frame : public frame_data
{
public:
    static const size_t MIN_FRAME_DATA_LENGTH;

    static std::shared_ptr<at_command_response_frame> parse_frame(std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end);

    enum status : uint8_t
    {
        ok = 0x00,
        error = 0x01,
        invalid_command = 0x02,
        invalid_parameter = 0x03
    };

    at_command_response_frame(uint8_t frame_id, uint16_t at_command, uint8_t status_value, const std::vector<uint8_t> &value);

    uint8_t get_status();
    const std::vector<uint8_t> &get_value() const;

    operator std::vector<uint8_t>() const override;

private:

    uint8_t frame_id;
    uint16_t at_command;
    uint8_t status_value;
    std::vector<uint8_t> value;
};

#endif
