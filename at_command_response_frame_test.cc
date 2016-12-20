#include <cstdint>
#include <iterator>
#include <string>

#include <gtest/gtest.h>

#include "at_command.h"
#include "at_command_response_frame.h"
#include "frame_data.h"
#include "util.h"

uint32_t any_address_high = 0xabcdef01;

at_command_response_frame get_valid_at_command_response_frame_empty_value()
{
    uint8_t any_frame_id = 0x01;
    return at_command_response_frame(any_frame_id, at_command::INTERFACE_DATA_RATE, at_command_response_frame::status::ok);
}

at_command_response_frame get_valid_at_command_response_frame()
{
    uint8_t any_frame_id = 0x01;
    std::vector<uint8_t> buffer;
    util::pack_value_as_bytes(std::back_inserter(buffer), any_address_high);
    return at_command_response_frame(any_frame_id, at_command::SERIAL_NUMBER_HIGH, at_command_response_frame::status::ok, buffer);
}

TEST(ATCommandResponseFrameTest, ConstValuesSpec)
{
    ASSERT_EQ(1, at_command_response_frame::FRAME_ID_OFFSET);
    ASSERT_EQ(2, at_command_response_frame::AT_COMMAND_OFFSET);
    ASSERT_EQ(4, at_command_response_frame::STATUS_OFFSET);
    ASSERT_EQ(5, at_command_response_frame::VALUE_OFFSET);
    ASSERT_EQ(5, at_command_response_frame::MIN_FRAME_DATA_LENGTH);
    ASSERT_EQ(sizeof(uint8_t), sizeof(at_command_response_frame::status::ok));
    ASSERT_EQ(0x00, at_command_response_frame::status::ok);
    ASSERT_EQ(0x01, at_command_response_frame::status::error);
    ASSERT_EQ(0x02, at_command_response_frame::status::invalid_command);
    ASSERT_EQ(0x03, at_command_response_frame::status::invalid_parameter);
}

TEST(ATCommandResponseFrameTest, ParseFrameTooSmall)
{
    std::vector<uint8_t> frame = get_valid_at_command_response_frame();
    frame.resize(at_command_response_frame::MIN_FRAME_DATA_LENGTH - 1);
    ASSERT_EQ(nullptr, at_command_response_frame::parse_frame(frame.cbegin(), frame.cend()));
}

TEST(ATCommandResponseFrameTest, ParseFrameNonATCommandResponse)
{
    std::vector<uint8_t> frame = get_valid_at_command_response_frame();
    ++frame[frame_data::API_IDENTIFIER_OFFSET];
    ASSERT_EQ(nullptr, at_command_response_frame::parse_frame(frame.cbegin(), frame.cend()));
}

TEST(ATCommandResponseFrameTest, ParseValidFrameEmptyValue)
{
    std::vector<uint8_t> frame = get_valid_at_command_response_frame_empty_value();
    ASSERT_NE(nullptr, at_command_response_frame::parse_frame(frame.cbegin(), frame.cend()));
}

TEST(ATCommandResponseFrameTest, ParseValidFrameNonemptyValue)
{
    std::vector<uint8_t> frame = get_valid_at_command_response_frame();
    ASSERT_NE(nullptr, at_command_response_frame::parse_frame(frame.cbegin(), frame.cend()));
}

TEST(ATCommandResponseFrameTest, GetStatusTest)
{
    at_command_response_frame frame = get_valid_at_command_response_frame();
    ASSERT_EQ(at_command_response_frame::status::ok, frame.get_status());
}

TEST(ATCommandResponseFrameTest, GetValueTest)
{
    at_command_response_frame frame = get_valid_at_command_response_frame();
    ASSERT_EQ(any_address_high, util::unpack_bytes_to_width<uint32_t>(frame.get_value().cbegin()));
}

TEST(ATCommandResponseFrameTest, OperatorVectorTest)
{
    at_command_response_frame frame = get_valid_at_command_response_frame();
    ASSERT_EQ(std::vector<uint8_t>({ 0x88, 0x01, 0x53, 0x48, 0x00, 0xab, 0xcd, 0xef, 0x01 }), static_cast<std::vector<uint8_t>>(frame));
}
