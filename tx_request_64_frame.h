#ifndef TX_REQUEST_64_FRAME_H
#define TX_REQUEST_64_FRAME_H

#include <cstdint>
#include <iterator>
#include <vector>

#include "frame_data.h"
#include "util.h"

class tx_request_64_frame : public frame_data
{
public:
    static const size_t MIN_FRAME_DATA_LENGTH;

    enum options : uint8_t
    {
        none = 0x00,
        disable_ack = 0x01,
        send_with_broadcast_pan_id = 0x04,
    };

    // TODO: can have diagnostic tx_requests sent with enable_response_frame = true to see if writes are actually occurring
    tx_request_64_frame(uint64_t destination_address, const std::vector<uint8_t> &rf_data, bool enable_response_frame = false);
    tx_request_64_frame(std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end);

    uint64_t get_destination_address() const;
    const std::vector<uint8_t> &get_rf_data() const;

    operator std::vector<uint8_t>() const override;

private:
    uint8_t frame_id;
    uint64_t destination_address;
    uint8_t options_value;
    std::vector<uint8_t> rf_data;   // TODO: max size 100 bytes
};

#endif
