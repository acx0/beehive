#include "message_segment.h"

const uint16_t message_segment::CHECKSUM_TARGET = 0xffff;
const uint8_t message_segment::MESSAGE_FLAGS_MASK = 0x0f;
const size_t message_segment::MESSAGE_TYPE_SHIFT_BITS = 4;
const size_t message_segment::SOURCE_PORT_OFFSET = 0;
const size_t message_segment::DESTINATION_PORT_OFFSET = SOURCE_PORT_OFFSET + sizeof(source_port);
const size_t message_segment::SEQUENCE_NUM_OFFSET
    = DESTINATION_PORT_OFFSET + sizeof(destination_port);
const size_t message_segment::CHECKSUM_OFFSET = SEQUENCE_NUM_OFFSET + sizeof(sequence_num);
const size_t message_segment::FLAGS_OFFSET = CHECKSUM_OFFSET + sizeof(checksum);
const size_t message_segment::MESSAGE_OFFSET = FLAGS_OFFSET + sizeof(flags);
const size_t message_segment::MIN_SEGMENT_LENGTH = MESSAGE_OFFSET;
const size_t message_segment::MAX_SEGMENT_LENGTH = uart_frame::MAX_FRAME_SIZE
    - uart_frame::HEADER_LENGTH - sizeof(frame_data::api_identifier_value)
    - sizeof(uint8_t) /* tx: frame_id, rx: rssi */ - sizeof(uint64_t) /* tx/rx: address */
    - sizeof(uint8_t) /* tx/rx: options */ - sizeof(source_port) - sizeof(destination_port)
    - sizeof(sequence_num) - sizeof(uint8_t) /* checksum */ - sizeof(checksum) - sizeof(flags);
const std::vector<uint8_t> message_segment::EMPTY_PAYLOAD;

message_segment::message_segment(uint16_t source_port, uint16_t destination_port,
    uint16_t sequence_num, uint8_t type, uint8_t flags, const std::vector<uint8_t> &message)
    : source_port(source_port), destination_port(destination_port), sequence_num(sequence_num),
      flags(0), message(message)
{
    this->flags += type << MESSAGE_TYPE_SHIFT_BITS;
    this->flags += flags & MESSAGE_FLAGS_MASK;
    checksum = compute_checksum();
}

message_segment::message_segment(const std::vector<uint8_t> &segment)
{
    if (segment.size() < MIN_SEGMENT_LENGTH)
    {
        // TODO: dont' use ctor directly, create separate method that can indicate failure/success
        return;
    }

    source_port = util::unpack_bytes_to_width<uint16_t>(segment.begin() + SOURCE_PORT_OFFSET);
    destination_port
        = util::unpack_bytes_to_width<uint16_t>(segment.begin() + DESTINATION_PORT_OFFSET);
    sequence_num = util::unpack_bytes_to_width<uint16_t>(segment.begin() + SEQUENCE_NUM_OFFSET);
    checksum = util::unpack_bytes_to_width<uint16_t>(segment.begin() + CHECKSUM_OFFSET);
    flags = segment[FLAGS_OFFSET];

    if (segment.size() > MESSAGE_OFFSET)
    {
        message = std::vector<uint8_t>(segment.begin() + MESSAGE_OFFSET, segment.end());
    }
}

std::shared_ptr<message_segment> message_segment::create_syn(
    uint16_t source_port, uint16_t destination_port)
{
    return std::make_shared<message_segment>(
        source_port, destination_port, 0, type::stream_segment, flag::syn, EMPTY_PAYLOAD);
}

std::shared_ptr<message_segment> message_segment::create_synack(
    uint16_t source_port, uint16_t destination_port)
{
    return std::make_shared<message_segment>(source_port, destination_port, 0, type::stream_segment,
        flag::syn | flag::ack, EMPTY_PAYLOAD);
}

std::shared_ptr<message_segment> message_segment::create_ack(
    uint16_t source_port, uint16_t destination_port, uint16_t sequence_number)
{
    return std::make_shared<message_segment>(source_port, destination_port, sequence_number,
        type::stream_segment, flag::ack, EMPTY_PAYLOAD);
}

std::shared_ptr<message_segment> message_segment::create_rst(
    uint16_t source_port, uint16_t destination_port)
{
    return std::make_shared<message_segment>(
        source_port, destination_port, 0, type::stream_segment, flag::rst, EMPTY_PAYLOAD);
}

std::shared_ptr<message_segment> message_segment::create_fin(
    uint16_t source_port, uint16_t destination_port)
{
    return std::make_shared<message_segment>(
        source_port, destination_port, 0, type::stream_segment, flag::fin, EMPTY_PAYLOAD);
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

uint16_t message_segment::get_checksum() const
{
    return checksum;
}

uint16_t message_segment::compute_checksum() const
{
    return CHECKSUM_TARGET - source_port - destination_port - sequence_num - flags
        - std::accumulate(message.begin(), message.end(), static_cast<uint16_t>(0));
}

uint8_t message_segment::get_message_type() const
{
    return flags >> MESSAGE_TYPE_SHIFT_BITS;
}

uint8_t message_segment::get_message_flags() const
{
    return flags & MESSAGE_FLAGS_MASK;
}

bool message_segment::flags_empty() const
{
    return get_message_flags() == flag::none;
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
    util::pack_value_as_bytes(std::back_inserter(segment), checksum);
    segment.push_back(flags);
    segment.insert(segment.end(), message.begin(), message.end());

    return segment;
}
