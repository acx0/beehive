#ifndef RX_PACKET_64_FRAME_H
#define RX_PACKET_64_FRAME_H

#include <cstdint>
#include <iostream>
#include <vector>

#include "frame_data.h"

class rx_packet_64_frame : public frame_data
{
public:
    static const size_t MIN_FRAME_DATA_LENGTH;

    // options field is used as bitset
    enum options_bit : uint8_t
    {
        address_broadcast = 1,
        pan_broadcast = 2
    };

    rx_packet_64_frame(const std::vector<uint8_t> &frame);

    operator std::vector<uint8_t>() const override;

private:
    uint64_t source_address;
    uint8_t rssi;   // received signal strength indicator, hex equivalent of (-dBm) value
    uint8_t options;
    std::vector<uint8_t> rf_data;
};

#endif