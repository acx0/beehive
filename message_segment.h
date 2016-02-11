#ifndef MESSAGE_SEGMENT_H
#define MESSAGE_SEGMENT_H

#include <cstdint>
#include <iostream>
#include <vector>

#include "util.h"

class message_segment
{
public:
    static const size_t MIN_SEGMENT_LENGTH;

    enum type : uint8_t
    {
        stream_segment = 0
    };

    enum flag : uint8_t
    {
        none = 0x0,
        ack = 0x1,
        rst = 0x2,
        syn = 0x4,
        fin = 0x8,
    };

    message_segment(uint8_t source_port, uint8_t destination_port, uint16_t sequence_num, uint16_t ack_num, uint8_t type, uint8_t flags, const std::vector<uint8_t> &message);
    message_segment(const std::vector<uint8_t> &segment);

    uint8_t get_source_port() const;
    uint8_t get_destination_port() const;
    uint16_t get_sequence_num() const;
    uint16_t get_ack_num() const;
    uint8_t get_message_type() const;
    uint8_t get_message_flags() const;
    bool is_ack() const;
    bool is_rst() const;
    bool is_syn() const;
    bool is_fin() const;
    const std::vector<uint8_t> &get_message() const;

    bool operator<(const message_segment &rhs) const;
    operator std::vector<uint8_t>() const;

private:
    uint8_t source_port;
    uint8_t destination_port;
    uint16_t sequence_num;
    uint16_t ack_num;
    uint8_t flags;  // bits 0-3: message flags, bits 4-7: message type
    std::vector<uint8_t> message;  // TODO: limit to 93 bytes (i.e. max_rf_data_size - message_segment overhead)
};

#endif
