#include "message_segment.h"

// source_port (2 bytes) + destination_port (2 bytes) + sequence_num (2 bytes) + ack_num (2 bytes) + flags
// TODO: replace this with sum of class member sizeof()s
const size_t message_segment::MIN_SEGMENT_LENGTH = 9;
const std::vector<uint8_t> message_segment::EMPTY_PAYLOAD;

message_segment::message_segment(uint16_t source_port, uint16_t destination_port, uint16_t sequence_num, uint16_t ack_num, uint8_t type, uint8_t flags, const std::vector<uint8_t> &message)
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

    source_port = util::unpack_bytes_to_width<uint16_t>(segment.begin());
    destination_port = util::unpack_bytes_to_width<uint16_t>(segment.begin() + 2);
    sequence_num = util::unpack_bytes_to_width<uint16_t>(segment.begin() + 4);
    ack_num = util::unpack_bytes_to_width<uint16_t>(segment.begin() + 6);
    flags = segment[8];

    if (segment.size() > MIN_SEGMENT_LENGTH)
    {
        message = std::vector<uint8_t>(segment.begin() + MIN_SEGMENT_LENGTH, segment.end());
    }
}

std::shared_ptr<message_segment> message_segment::create_syn(uint16_t source_port, uint16_t destination_port)
{
    return std::make_shared<message_segment>(source_port, destination_port, 0, 0, type::stream_segment, flag::syn, EMPTY_PAYLOAD);
}

std::shared_ptr<message_segment> message_segment::create_synack(uint16_t source_port, uint16_t destination_port)
{
    return std::make_shared<message_segment>(source_port, destination_port, 0, 0, type::stream_segment, flag::syn | flag::ack, EMPTY_PAYLOAD);
}

std::shared_ptr<message_segment> message_segment::create_ack(uint16_t source_port, uint16_t destination_port, uint16_t sequence_number)
{
    return std::make_shared<message_segment>(source_port, destination_port, sequence_number, 0, type::stream_segment, flag::ack, EMPTY_PAYLOAD);
}

uint16_t message_segment::get_source_port() const
{
    return source_port;
}

uint16_t message_segment::get_destination_port() const
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
    return get_message_flags() == flag::ack;
}

bool message_segment::is_rst() const
{
    return get_message_flags() == flag::rst;
}

bool message_segment::is_syn() const
{
    return get_message_flags() == flag::syn;
}

bool message_segment::is_fin() const
{
    return get_message_flags() == flag::fin;
}

bool message_segment::is_synack() const
{
    return get_message_flags() == (flag::syn | flag::ack);
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

    util::pack_value_as_bytes(std::back_inserter(segment), source_port);
    util::pack_value_as_bytes(std::back_inserter(segment), destination_port);
    util::pack_value_as_bytes(std::back_inserter(segment), sequence_num);
    util::pack_value_as_bytes(std::back_inserter(segment), ack_num);
    segment.push_back(flags);
    segment.insert(segment.end(), message.begin(), message.end());

    return segment;
}
