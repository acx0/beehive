#include <gtest/gtest.h>

#include "uart_frame.h"

TEST(UARTFrameTest, ConstValuesSpec)
{
    ASSERT_EQ(uart_frame::FRAME_DELIMITER, 0x7e);
    ASSERT_EQ(uart_frame::ESCAPE, 0x7d);
    ASSERT_EQ(uart_frame::XON, 0x11);
    ASSERT_EQ(uart_frame::XOFF, 0x13);
    ASSERT_EQ(uart_frame::XOR_CONST, 0x20);
    ASSERT_EQ(uart_frame::HEADER_LENGTH, 3);
    ASSERT_EQ(uart_frame::MIN_FRAME_SIZE, uart_frame::HEADER_LENGTH + 2);
    ASSERT_EQ(uart_frame::MAX_FRAME_SIZE, 115);
    ASSERT_EQ(uart_frame::FRAME_DELIMITER_OFFSET, 0);
    ASSERT_EQ(uart_frame::LENGTH_MSB_OFFSET, 1);
    ASSERT_EQ(uart_frame::LENGTH_LSB_OFFSET, 2);
    ASSERT_EQ(uart_frame::API_IDENTIFIER_OFFSET, 3);
}
