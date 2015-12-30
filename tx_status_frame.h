#ifndef TX_STATUS_H
#define TX_STATUS_H

#include <cstdint>
#include <iostream>
#include <vector>

#include "frame_data.h"

class tx_status_frame : public frame_data
{
public:
    static const size_t MIN_FRAME_DATA_LENGTH;

    // note: if transmitter broadcasts, only 0 or 2 will be returned
    enum status : uint8_t
    {
        success = 0x00,
        no_ack_received = 0x01, // when all retries are expired and no ack is received
        cca_failure = 0x02,
        purged = 0x03
    };

    tx_status_frame(const std::vector<uint8_t> &frame);

    operator std::vector<uint8_t>() const override;

private:
    uint8_t frame_id;
    uint8_t status;
};

#endif
