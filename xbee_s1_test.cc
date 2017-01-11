#include <cstdint>

#include <gtest/gtest.h>

#include "xbee_s1.h"

namespace xbee_s1_test
{
    TEST(XBee, ConstValuesSpec)
    {
        ASSERT_EQ(0xffffffffffffffff, xbee_s1::ADDRESS_UNKNOWN);
        ASSERT_EQ(sizeof(uint64_t), sizeof(xbee_s1::BROADCAST_ADDRESS));
        ASSERT_EQ(0xffff, xbee_s1::BROADCAST_ADDRESS);
        ASSERT_STREQ("+++", xbee_s1::COMMAND_SEQUENCE);
    }
}
