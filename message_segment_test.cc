#include <cstdint>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "message_segment.h"

namespace message_segment_test
{
    uint16_t any_source_port = 0xffff;
    uint16_t any_destination_port = 0x00ab;
    uint16_t any_sequence_number = 123;
    uint8_t any_type = message_segment::type::stream_segment;
    uint8_t any_flags = message_segment::flag::syn;
    std::vector<uint8_t> any_message{'t', 'e', 's', 't'};

    message_segment get_valid_message_segment()
    {
        return message_segment(any_source_port, any_destination_port, any_sequence_number, any_type,
            any_flags, any_message);
    }

    message_segment get_valid_message_segment_empty_message()
    {
        return message_segment(any_source_port, any_destination_port, any_sequence_number, any_type,
            any_flags, std::vector<uint8_t>());
    }

    message_segment get_valid_message_segment_empty_flags()
    {
        return message_segment(any_source_port, any_destination_port, any_sequence_number, any_type,
            message_segment::flag::none, any_message);
    }

    TEST(MessageSegmentTest, ConstValuesSpec)
    {
        ASSERT_EQ(0xffff, message_segment::CHECKSUM_TARGET);
        ASSERT_EQ(0x0f, message_segment::MESSAGE_FLAGS_MASK);
        ASSERT_EQ(4, message_segment::MESSAGE_TYPE_SHIFT_BITS);

        ASSERT_EQ(0, message_segment::SOURCE_PORT_OFFSET);
        ASSERT_EQ(2, message_segment::DESTINATION_PORT_OFFSET);
        ASSERT_EQ(4, message_segment::SEQUENCE_NUM_OFFSET);
        ASSERT_EQ(6, message_segment::CHECKSUM_OFFSET);
        ASSERT_EQ(8, message_segment::FLAGS_OFFSET);
        ASSERT_EQ(9, message_segment::MESSAGE_OFFSET);
        ASSERT_EQ(9, message_segment::MIN_SEGMENT_LENGTH);
        ASSERT_EQ(91, message_segment::MAX_SEGMENT_LENGTH);
        ASSERT_EQ(std::vector<uint8_t>(), message_segment::EMPTY_PAYLOAD);

        ASSERT_EQ(sizeof(uint8_t), sizeof(message_segment::type));
        ASSERT_EQ(0, message_segment::type::stream_segment);
        ASSERT_EQ(1, message_segment::type::datagram_segment);
        ASSERT_EQ(2, message_segment::type::neighbour_discovery);

        ASSERT_EQ(sizeof(uint8_t), sizeof(message_segment::flag));
        ASSERT_EQ(0x0, message_segment::flag::none);
        ASSERT_EQ(0x1, message_segment::flag::ack);
        ASSERT_EQ(0x2, message_segment::flag::rst);
        ASSERT_EQ(0x4, message_segment::flag::syn);
        ASSERT_EQ(0x8, message_segment::flag::fin);
    }

    TEST(MessageSegmentTest, CreateSynTest)
    {
        std::shared_ptr<message_segment> msg
            = message_segment::create_syn(any_source_port, any_destination_port);
        ASSERT_EQ(message_segment::flag::syn, msg->get_message_flags());
    }

    TEST(MessageSegmentTest, CreateSynAckTest)
    {
        std::shared_ptr<message_segment> msg
            = message_segment::create_synack(any_source_port, any_destination_port);
        ASSERT_EQ(
            message_segment::flag::syn | message_segment::flag::ack, msg->get_message_flags());
    }

    TEST(MessageSegmentTest, CreateAckTest)
    {
        std::shared_ptr<message_segment> msg
            = message_segment::create_ack(any_source_port, any_destination_port);
        ASSERT_EQ(message_segment::flag::ack, msg->get_message_flags());
    }

    TEST(MessageSegmentTest, CreateFinTest)
    {
        std::shared_ptr<message_segment> msg
            = message_segment::create_fin(any_source_port, any_destination_port);
        ASSERT_EQ(message_segment::flag::fin, msg->get_message_flags());
    }

    TEST(MessageSegmentTest, GetSourcePortTest)
    {
        std::shared_ptr<message_segment> msg
            = message_segment::create_fin(any_source_port, any_destination_port);
        ASSERT_EQ(any_source_port, msg->get_source_port());
    }

    TEST(MessageSegmentTest, GetDestinationPortTest)
    {
        std::shared_ptr<message_segment> msg
            = message_segment::create_fin(any_source_port, any_destination_port);
        ASSERT_EQ(any_destination_port, msg->get_destination_port());
    }

    TEST(MessageSegmentTest, GetSequenceNumberTest)
    {
        message_segment msg = get_valid_message_segment();
        ASSERT_EQ(any_sequence_number, msg.get_sequence_num());
    }

    TEST(MessageSegmentTest, GetChecksumTest)
    {
        message_segment msg = get_valid_message_segment();
        ASSERT_EQ(64790, msg.get_checksum());
    }

    TEST(MessageSegmentTest, ComputeChecksumEmptyMessage)
    {
        message_segment msg = get_valid_message_segment_empty_message();
        ASSERT_EQ(65238, msg.compute_checksum());
    }

    TEST(MessageSegmentTest, ComputeChecksumNonemptyMessage)
    {
        message_segment msg = get_valid_message_segment();
        ASSERT_EQ(64790, msg.get_checksum());
    }

    TEST(MessageSegmentTest, GetMessageTypeTest)
    {
        message_segment msg = get_valid_message_segment();
        ASSERT_EQ(any_type, msg.get_message_type());
    }

    TEST(MessageSegmentTest, GetMessageFlagsTest)
    {
        message_segment msg = get_valid_message_segment();
        ASSERT_EQ(any_flags, msg.get_message_flags());
    }

    TEST(MessageSegmentTest, FlagsEmptyTest)
    {
        message_segment msg = get_valid_message_segment_empty_flags();
        ASSERT_TRUE(msg.flags_empty());
    }

    TEST(MessageSegmentTest, IsAckTest)
    {
        std::shared_ptr<message_segment> msg
            = message_segment::create_ack(any_source_port, any_destination_port);
        ASSERT_TRUE(msg->is_ack());
    }

    TEST(MessageSegmentTest, SynAckIsNotAck)
    {
        std::shared_ptr<message_segment> msg
            = message_segment::create_synack(any_source_port, any_destination_port);
        ASSERT_FALSE(msg->is_ack());
    }

    TEST(MessageSegmentTest, IsRstTest)
    {
        std::shared_ptr<message_segment> msg
            = message_segment::create_rst(any_source_port, any_destination_port);
        ASSERT_TRUE(msg->is_rst());
    }

    TEST(MessageSegmentTest, IsSynTest)
    {
        std::shared_ptr<message_segment> msg
            = message_segment::create_syn(any_source_port, any_destination_port);
        ASSERT_TRUE(msg->is_syn());
    }

    TEST(MessageSegmentTest, IsSynAckTest)
    {
        std::shared_ptr<message_segment> msg
            = message_segment::create_synack(any_source_port, any_destination_port);
        ASSERT_TRUE(msg->is_synack());
    }

    TEST(MessageSegmentTest, GetMessageTest)
    {
        message_segment msg = get_valid_message_segment();
        ASSERT_EQ(any_message, msg.get_message());
    }

    TEST(MessageSegmentTest, OperatorLessThanTest)
    {
        message_segment m1 = message_segment(any_source_port, any_destination_port,
            any_sequence_number, any_type, any_flags, any_message);
        message_segment m2 = message_segment(any_source_port, any_destination_port,
            any_sequence_number + 1, any_type, any_flags, any_message);
        ASSERT_TRUE(m1 < m2);
    }

    TEST(MessageSegmentTest, OperatorVectorTest)
    {
        message_segment msg = get_valid_message_segment();
        ASSERT_EQ(static_cast<std::vector<uint8_t>>(msg),
            std::vector<uint8_t>({0xff, 0xff, 0x00, 0xab, 0x00, 0x7b, 0xfd, 0x16, 0x04, 0x74, 0x65,
                0x73, 0x74}));
    }
}
