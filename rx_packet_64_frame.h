#ifndef RX_PACKET_64_FRAME_H
#define RX_PACKET_64_FRAME_H

#include <cstdint>
#include <iterator>
#include <memory>
#include <vector>

#include "frame_data.h"
#include "util.h"

class rx_packet_64_frame : public frame_data
{
public:
    static const size_t SOURCE_ADDRESS_OFFSET;
    static const size_t RSSI_OFFSET;
    static const size_t OPTIONS_OFFSET;
    static const size_t RF_DATA_OFFSET;
    static const size_t MIN_FRAME_DATA_LENGTH;

    static std::shared_ptr<rx_packet_64_frame> parse_frame(
        std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end);

    // options field is used as bitset
    enum options_bit : uint8_t
    {
        address_broadcast = 1,
        pan_broadcast = 2
    };

    rx_packet_64_frame(
        uint64_t source_address, uint8_t rssi, uint8_t options, std::vector<uint8_t> rf_data);

    uint64_t get_source_address() const;
    const std::vector<uint8_t> &get_rf_data() const;
    bool is_broadcast_frame() const;

    operator std::vector<uint8_t>() const override;

private:
    uint64_t source_address;
    uint8_t rssi;    // received signal strength indicator, hex equivalent of (-dBm) value
    uint8_t options;
    std::vector<uint8_t> rf_data;
};

#endif
