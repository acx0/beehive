#ifndef TX_REQUEST_64_FRAME_H
#define TX_REQUEST_64_FRAME_H

#include <cstdint>
#include <vector>

#include "frame_data.h"

class tx_request_64_frame : public frame_data
{
public:
    static const uint64_t BROADCAST_ADDRESS;

    enum options : uint8_t
    {
        none = 0x00,
        disable_ack = 0x01,
        send_with_broadcast_pan_id = 0x04,
    };

    tx_request_64_frame(uint64_t destination_address, const std::vector<uint8_t> &rf_data);

    operator std::vector<uint8_t>() const override;

private:
    uint8_t frame_id;
    uint64_t destination_address;
    uint8_t options;
    std::vector<uint8_t> rf_data;
};

#endif