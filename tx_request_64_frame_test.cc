#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "frame_data.h"
#include "tx_request_64_frame.h"

namespace tx_request_64_frame_test
{
    uint8_t any_frame_id = 1;
    uint64_t any_destination_address = 0xabcdef0123456789;
    uint8_t any_options = tx_request_64_frame::options::none;
    std::vector<uint8_t> any_valid_payload{'t', 'e', 's', 't'};

    tx_request_64_frame get_valid_tx_request_64_frame_empty_rf_data()
    {
        return tx_request_64_frame(
            any_frame_id, any_destination_address, any_options, std::vector<uint8_t>());
    }

    tx_request_64_frame get_valid_tx_request_64_frame()
    {
        return tx_request_64_frame(
            any_frame_id, any_destination_address, any_options, any_valid_payload);
    }

    TEST(TXRequest64FrameTest, ConstValuesSpec)
    {
        ASSERT_EQ(1, tx_request_64_frame::FRAME_ID_OFFSET);
        ASSERT_EQ(2, tx_request_64_frame::DESTINATION_ADDRESS_OFFSET);
        ASSERT_EQ(10, tx_request_64_frame::OPTIONS_OFFSET);
        ASSERT_EQ(11, tx_request_64_frame::RF_DATA_OFFSET);
        ASSERT_EQ(11, tx_request_64_frame::MIN_FRAME_DATA_LENGTH);
        ASSERT_EQ(sizeof(uint8_t), sizeof(tx_request_64_frame::options));
        ASSERT_EQ(0x00, tx_request_64_frame::options::none);
        ASSERT_EQ(0x01, tx_request_64_frame::options::disable_ack);
        ASSERT_EQ(0x04, tx_request_64_frame::options::send_with_broadcast_pan_id);
    }

    TEST(TXRequest64FrameTest, ParseFrameTooSmall)
    {
        std::vector<uint8_t> frame = get_valid_tx_request_64_frame();
        frame.resize(tx_request_64_frame::MIN_FRAME_DATA_LENGTH - 1);
        ASSERT_EQ(nullptr, tx_request_64_frame::parse_frame(frame.cbegin(), frame.cend()));
    }

    TEST(TXRequest64FrameTest, ParseFrameNonTXRequest64)
    {
        std::vector<uint8_t> frame = get_valid_tx_request_64_frame();
        ++frame[frame_data::API_IDENTIFIER_OFFSET];
        ASSERT_EQ(nullptr, tx_request_64_frame::parse_frame(frame.cbegin(), frame.cend()));
    }

    TEST(TXRequest64FrameTest, ParseValidFrameEmptyRFData)
    {
        std::vector<uint8_t> frame = get_valid_tx_request_64_frame_empty_rf_data();
        ASSERT_NE(nullptr, tx_request_64_frame::parse_frame(frame.cbegin(), frame.cend()));
    }

    TEST(TXRequest64FrameTest, ParseValidFrameNonemptyRFData)
    {
        std::vector<uint8_t> frame = get_valid_tx_request_64_frame();
        ASSERT_NE(nullptr, tx_request_64_frame::parse_frame(frame.cbegin(), frame.cend()));
    }

    TEST(TXRequest64FrameTest, GetSourceAddressTest)
    {
        tx_request_64_frame frame = get_valid_tx_request_64_frame();
        ASSERT_EQ(any_destination_address, frame.get_destination_address());
    }

    TEST(TXRequest64FrameTest, GetRFDataTest)
    {
        tx_request_64_frame frame = get_valid_tx_request_64_frame();
        ASSERT_EQ(any_valid_payload, frame.get_rf_data());
    }

    TEST(TXRequest64FrameTest, OperatorVectorTest)
    {
        tx_request_64_frame frame = get_valid_tx_request_64_frame();
        ASSERT_EQ(std::vector<uint8_t>({0x00, 0x01, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
                      0x00, 0x74, 0x65, 0x73, 0x74}),
            static_cast<std::vector<uint8_t>>(frame));
    }
}
