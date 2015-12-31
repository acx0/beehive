#ifndef UART_FRAME_H
#define UART_FRAME_H

#include <cstdint>
#include <memory>
#include <vector>

#include "frame_data.h"
#include "util.h"

class uart_frame
{
public:
    static const uint8_t FRAME_DELIMITER;
    static const uint8_t ESCAPE;
    static const uint8_t XON;
    static const uint8_t XOFF;
    static const uint8_t XOR_CONST;

    uart_frame(std::shared_ptr<frame_data> data);
    uart_frame(uint8_t length_msb, uint8_t length_lsb, std::shared_ptr<frame_data> data, uint8_t checksum);

    static uint8_t compute_checksum(const std::vector<uint8_t> &payload);

    operator std::vector<uint8_t>() const;

private:
    // note: length and checksum fields only populated when reading response frames
    uint8_t length_msb;
    uint8_t length_lsb;
    std::shared_ptr<frame_data> data;   // TODO: unique_ptr?
    uint8_t checksum;
};

#endif
