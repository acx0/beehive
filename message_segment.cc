#include "message_segment.h"

// source_port + destination_port + sequence_num (2 bytes) + ack_num (2 bytes) + flags
// TODO: replace this with sum of class member sizeof()s
const size_t message_segment::MIN_SEGMENT_LENGTH = 7;

message_segment::message_segment(uint8_t source_port, uint8_t destination_port, uint16_t sequence_num, uint16_t ack_num, uint8_t type, uint8_t flags, const std::vector<uint8_t> &message)
    : source_port(source_port), destination_port(destination_port), sequence_num(sequence_num), ack_num(ack_num), flags(0), message(message)
{
    this->flags += type << 4;
    this->flags += flags & 0x0f;
}

message_segment::message_segment(const std::vector<uint8_t> &segment)
{
    if (segment.size() < MIN_SEGMENT_LENGTH)
    {
        // TODO: exception
        std::cerr << "invalid message_segment size" << std::endl;
    }

    source_port = segment[0];
    destination_port = segment[1];
    sequence_num = util::unpack_bytes_to_width<uint16_t>(std::vector<uint8_t>(segment.begin() + 2, segment.begin() + 4));
    ack_num = util::unpack_bytes_to_width<uint16_t>(std::vector<uint8_t>(segment.begin() + 4, segment.begin() + 6));
    flags = segment[6];

    if (segment.size() > MIN_SEGMENT_LENGTH)
    {
        message = std::vector<uint8_t>(segment.begin() + MIN_SEGMENT_LENGTH, segment.end());
    }
}

uint8_t message_segment::get_source_port() const
{
    return source_port;
}

uint8_t message_segment::get_destination_port() const
{
    return destination_port;
}

uint16_t message_segment::get_sequence_num() const
{
    return sequence_num;
}

uint16_t message_segment::get_ack_num() const
{
    return ack_num;
}

uint8_t message_segment::get_message_type() const
{
    return flags >> 4;
}

uint8_t message_segment::get_message_flags() const
{
    return flags & 0x0f;
}

bool message_segment::is_ack() const
{
    return flags & flag::ack;
}

bool message_segment::is_rst() const
{
    return flags & flag::rst;
}

bool message_segment::is_syn() const
{
    return flags & flag::syn;
}

bool message_segment::is_fin() const
{
    return flags & flag::fin;
}

const std::vector<uint8_t> &message_segment::get_message() const
{
    return message;
}

bool message_segment::operator<(const message_segment &rhs) const
{
    return sequence_num < rhs.sequence_num;
}

message_segment::operator std::vector<uint8_t>() const
{
    std::vector<uint8_t> segment;

    segment.push_back(source_port);
    segment.push_back(destination_port);
    util::pack_value_as_bytes(segment, sequence_num);
    util::pack_value_as_bytes(segment, ack_num);
    segment.push_back(flags);
    segment.insert(segment.end(), message.begin(), message.end());

    return segment;
}
