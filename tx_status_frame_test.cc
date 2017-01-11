#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "frame_data.h"
#include "tx_status_frame.h"

namespace tx_status_frame_test
{
    tx_status_frame get_valid_tx_status_frame()
    {
        uint8_t any_frame_id = 1;
        uint8_t any_status = tx_status_frame::status::success;
        return tx_status_frame(any_frame_id, any_status);
    }

    TEST(TXStatusFrameTest, ConstValuesSpec)
    {
        ASSERT_EQ(1, tx_status_frame::FRAME_ID_OFFSET);
        ASSERT_EQ(2, tx_status_frame::STATUS_OFFSET);
        ASSERT_EQ(3, tx_status_frame::MIN_FRAME_DATA_LENGTH);
        ASSERT_EQ(sizeof(uint8_t), sizeof(tx_status_frame::status));
        ASSERT_EQ(0x00, tx_status_frame::status::success);
        ASSERT_EQ(0x01, tx_status_frame::status::no_ack_received);
        ASSERT_EQ(0x02, tx_status_frame::status::cca_failure);
        ASSERT_EQ(0x03, tx_status_frame::status::purged);
    }

    TEST(TXStatusFrameTest, ParseFrameTooSmall)
    {
        std::vector<uint8_t> frame = get_valid_tx_status_frame();
        frame.resize(tx_status_frame::MIN_FRAME_DATA_LENGTH - 1);
        ASSERT_EQ(nullptr, tx_status_frame::parse_frame(frame.cbegin(), frame.cend()));
    }

    TEST(TXStatusFrameTest, ParseFrameNonTXStatus)
    {
        std::vector<uint8_t> frame = get_valid_tx_status_frame();
        ++frame[frame_data::API_IDENTIFIER_OFFSET];
        ASSERT_EQ(nullptr, tx_status_frame::parse_frame(frame.cbegin(), frame.cend()));
    }

    TEST(TXStatusFrameTest, ParseValidFrame)
    {
        std::vector<uint8_t> frame = get_valid_tx_status_frame();
        ASSERT_NE(nullptr, tx_status_frame::parse_frame(frame.cbegin(), frame.cend()));
    }

    TEST(TXStatusFrameTest, OperatorVectorTest)
    {
        tx_status_frame frame = get_valid_tx_status_frame();
        ASSERT_EQ(
            std::vector<uint8_t>({0x89, 0x01, 0x00}), static_cast<std::vector<uint8_t>>(frame));
    }
}
