#ifndef FRAME_DATA_H
#define FRAME_DATA_H

#include <cstdint>
#include <vector>

// TODO: derived classes should throw exception if payload > 100bytes
class frame_data
{
public:
    const uint8_t api_identifier;

    enum api_identifier : uint8_t
    {
        at_command = 0x08,
        at_command_response = 0x88,
    };

    virtual operator std::vector<uint8_t>() const = 0;

protected:
    frame_data(uint8_t api_identifier);
};

#endif
