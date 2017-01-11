#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "at_command.h"
#include "at_command_frame.h"

namespace at_command_frame_test
{
    TEST(ATCommandFrameTest, ConstValuesSpec)
    {
        ASSERT_EQ(std::vector<uint8_t>(), at_command_frame::REGISTER_QUERY);
    }

    TEST(ATCommandFrameTest, GetATCommand)
    {
        ASSERT_EQ(at_command::SERIAL_NUMBER_LOW,
            at_command_frame(at_command::SERIAL_NUMBER_LOW).get_at_command());
    }

    TEST(ATCommandFrameTest, OperatorVectorTestRegisterQuery)
    {
        ASSERT_EQ(std::vector<uint8_t>({0x08, 0x01, 0x43, 0x48}),
            static_cast<std::vector<uint8_t>>(at_command_frame(
                at_command::CHANNEL, at_command_frame::REGISTER_QUERY, true)));
    }

    TEST(ATCommandFrameTest, OperatorVectorTestRegisterConfiguration)
    {
        ASSERT_EQ(std::vector<uint8_t>({0x08, 0x01, 0x50, 0x4c, 0x02}),
            static_cast<std::vector<uint8_t>>(
                      at_command_frame(at_command::POWER_LEVEL, {0x02}, true)));
    }
}
