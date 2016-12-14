#include <cstdint>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "frame_data.h"
#include "tx_request_64_frame.h"
#include "uart_frame.h"
#include "xbee_s1.h"

uart_frame get_valid_tx_request_64_frame()
{
    uint8_t any_frame_id = 0x01;
    uint64_t any_destination_address = xbee_s1::BROADCAST_ADDRESS;
    uint8_t any_options = tx_request_64_frame::options::none;
    std::vector<uint8_t> any_valid_payload{ 't', 'e', 's', 't' };
    return uart_frame(std::make_shared<tx_request_64_frame>(tx_request_64_frame(any_frame_id, any_destination_address, any_options, any_valid_payload)));
}

TEST(UARTFrameTest, ConstValuesSpec)
{
    ASSERT_EQ(uart_frame::FRAME_DELIMITER, 0x7e);
    ASSERT_EQ(uart_frame::ESCAPE, 0x7d);
    ASSERT_EQ(uart_frame::XON, 0x11);
    ASSERT_EQ(uart_frame::XOFF, 0x13);
    ASSERT_EQ(uart_frame::XOR_CONST, 0x20);
    ASSERT_EQ(uart_frame::CHECKSUM_TARGET, 0xff);
    ASSERT_EQ(uart_frame::HEADER_LENGTH, 3);
    ASSERT_EQ(uart_frame::MIN_FRAME_SIZE, uart_frame::HEADER_LENGTH + 2);
    ASSERT_EQ(uart_frame::MAX_FRAME_SIZE, 115);
    ASSERT_EQ(uart_frame::FRAME_DELIMITER_OFFSET, 0);
    ASSERT_EQ(uart_frame::LENGTH_MSB_OFFSET, 1);
    ASSERT_EQ(uart_frame::LENGTH_LSB_OFFSET, 2);
    ASSERT_EQ(uart_frame::API_IDENTIFIER_OFFSET, 3);
    ASSERT_EQ(uart_frame::IDENTIFIER_DATA_OFFSET, 4);
}

TEST(UARTFrameTest, ParseFrameTooSmall)
{
    std::vector<uint8_t> frame = get_valid_tx_request_64_frame();
    frame.resize(uart_frame::MIN_FRAME_SIZE - 1);
    ASSERT_EQ(uart_frame::parse_frame(frame.cbegin(), frame.cend()), nullptr);
}

// TODO: test with 'valid' large frame, i.e. correct frame structure, checksum
TEST(UARTFrameTest, ParseFrameTooLarge)
{
    std::vector<uint8_t> frame(uart_frame::MAX_FRAME_SIZE + 1, 0xff);
    ASSERT_EQ(uart_frame::parse_frame(frame.cbegin(), frame.cend()), nullptr);
}

TEST(UARTFrameTest, ParseFrameInvalidDelimiter)
{
    std::vector<uint8_t> frame = get_valid_tx_request_64_frame();
    ++frame[uart_frame::FRAME_DELIMITER_OFFSET];
    ASSERT_EQ(uart_frame::parse_frame(frame.cbegin(), frame.cend()), nullptr);
}

TEST(UARTFrameTest, ParseFrameIncorrectLength)
{
    std::vector<uint8_t> frame = get_valid_tx_request_64_frame();
    ++frame[uart_frame::LENGTH_LSB_OFFSET];
    ASSERT_EQ(uart_frame::parse_frame(frame.cbegin(), frame.cend()), nullptr);
}

TEST(UARTFrameTest, ParseValidFrame)
{
    std::vector<uint8_t> frame = get_valid_tx_request_64_frame();
    ASSERT_NE(uart_frame::parse_frame(frame.cbegin(), frame.cend()), nullptr);
}

TEST(UARTFrameTest, ComputeChecksumEmptyPayload)
{
    std::vector<uint8_t> frame;
    ASSERT_EQ(uart_frame::compute_checksum(frame.cbegin(), frame.cend()), uart_frame::CHECKSUM_TARGET);
}

TEST(UARTFrameTest, ComputeChecksumNonemptyPayload)
{
    std::vector<uint8_t> frame{ 0xde, 0xad, 0xbe, 0xef, 0xf0, 0x0d };
    ASSERT_EQ(uart_frame::compute_checksum(frame.cbegin(), frame.cend()), 0xca);
}

TEST(UARTFrameTest, OperatorVectorTest)
{
    uart_frame frame = get_valid_tx_request_64_frame();
    ASSERT_EQ(static_cast<std::vector<uint8_t>>(frame), std::vector<uint8_t>({ 0x7e, 0x00, 0x0f, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x74, 0x65, 0x73, 0x74, 0x40 }));
}
