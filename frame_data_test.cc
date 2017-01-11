#include <cstdint>

#include <gtest/gtest.h>

#include "frame_data.h"

namespace frame_data_test
{
    TEST(FrameDataTest, ConstValuesSpec)
    {
        ASSERT_EQ(0, frame_data::FRAME_ID_DISABLE_RESPONSE_FRAME);
        ASSERT_EQ(0, frame_data::API_IDENTIFIER_OFFSET);
        ASSERT_EQ(sizeof(uint8_t), sizeof(frame_data::api_identifier));
        ASSERT_EQ(0x00, frame_data::api_identifier::tx_request_64);
        ASSERT_EQ(0x08, frame_data::api_identifier::at_command);
        ASSERT_EQ(0x80, frame_data::api_identifier::rx_packet_64);
        ASSERT_EQ(0x88, frame_data::api_identifier::at_command_response);
        ASSERT_EQ(0x89, frame_data::api_identifier::tx_status);
    }
}
