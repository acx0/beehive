#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "frame_data.h"
#include "rx_packet_64_frame.h"

uint64_t any_source_address = 0xabcdef0123456789;
uint8_t any_options = 0;
uint8_t any_rssi = 0x0f;
std::vector<uint8_t> any_valid_payload{'t', 'e', 's', 't'};

rx_packet_64_frame get_valid_rx_packet_64_frame_empty_rf_data()
{
    return rx_packet_64_frame(any_source_address, any_rssi, any_options, std::vector<uint8_t>());
}

rx_packet_64_frame get_valid_rx_packet_64_frame()
{
    return rx_packet_64_frame(any_source_address, any_rssi, any_options, any_valid_payload);
}

TEST(RXPacket64FrameTest, ConstValuesSpec)
{
    ASSERT_EQ(11, rx_packet_64_frame::MIN_FRAME_DATA_LENGTH);
    ASSERT_EQ(sizeof(uint8_t), sizeof(rx_packet_64_frame::options_bit));
    ASSERT_EQ(1, rx_packet_64_frame::options_bit::address_broadcast);
    ASSERT_EQ(2, rx_packet_64_frame::options_bit::pan_broadcast);
}

TEST(RXPacket64FrameTest, ParseFrameTooSmall)
{
    std::vector<uint8_t> frame = get_valid_rx_packet_64_frame();
    frame.resize(rx_packet_64_frame::MIN_FRAME_DATA_LENGTH - 1);
    ASSERT_EQ(nullptr, rx_packet_64_frame::parse_frame(frame.cbegin(), frame.cend()));
}

TEST(RXPacket64FrameTest, ParseFrameNonRxPacket64)
{
    std::vector<uint8_t> frame = get_valid_rx_packet_64_frame();
    ++frame[frame_data::API_IDENTIFIER_OFFSET];
    ASSERT_EQ(nullptr, rx_packet_64_frame::parse_frame(frame.cbegin(), frame.cend()));
}

TEST(RXPacket64FrameTest, ParseValidFrameEmptyRFData)
{
    std::vector<uint8_t> frame = get_valid_rx_packet_64_frame_empty_rf_data();
    ASSERT_NE(nullptr, rx_packet_64_frame::parse_frame(frame.cbegin(), frame.cend()));
}

TEST(RXPacket64FrameTest, ParseValidFrameNonemptyRFData)
{
    std::vector<uint8_t> frame = get_valid_rx_packet_64_frame();
    ASSERT_NE(nullptr, rx_packet_64_frame::parse_frame(frame.cbegin(), frame.cend()));
}

TEST(RXPacket64FrameTest, GetSourceAddressTest)
{
    rx_packet_64_frame frame = get_valid_rx_packet_64_frame();
    ASSERT_EQ(any_source_address, frame.get_source_address());
}

TEST(RXPacket64FrameTest, GetRFDataTest)
{
    rx_packet_64_frame frame = get_valid_rx_packet_64_frame();
    ASSERT_EQ(any_valid_payload, frame.get_rf_data());
}

TEST(RXPacket64FrameTest, IsBroadcastFrameTest)
{
    uint8_t no_broadcast = 0;
    uint8_t address_broadcast = 1 << rx_packet_64_frame::options_bit::address_broadcast;
    uint8_t pan_broadcast = 1 << rx_packet_64_frame::options_bit::pan_broadcast;

    rx_packet_64_frame frame1(any_source_address, any_rssi, address_broadcast, any_valid_payload);
    ASSERT_TRUE(frame1.is_broadcast_frame());

    rx_packet_64_frame frame2(any_source_address, any_rssi, pan_broadcast, any_valid_payload);
    ASSERT_TRUE(frame2.is_broadcast_frame());

    rx_packet_64_frame frame3(any_source_address, any_rssi, no_broadcast, any_valid_payload);
    ASSERT_FALSE(frame3.is_broadcast_frame());
}

TEST(RXPacket64FrameTest, OperatorVectorTest)
{
    rx_packet_64_frame frame = get_valid_rx_packet_64_frame();
    ASSERT_EQ(std::vector<uint8_t>({0x80, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0x0f,
                  0x00, 0x74, 0x65, 0x73, 0x74}),
        static_cast<std::vector<uint8_t>>(frame));
}
