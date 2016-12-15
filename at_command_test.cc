#include <gtest/gtest.h>

#include "at_command.h"

TEST(ATCommandTest, ConstValuesSpec)
{
    ASSERT_STREQ(at_command::RESPONSE_SUCCESS, "OK");
    ASSERT_STREQ(at_command::RESPONSE_ERROR, "ERROR");
    ASSERT_STREQ(at_command::AT_PREFIX, "AT");
    ASSERT_STREQ(at_command::API_ENABLE, "AP");
    ASSERT_STREQ(at_command::EXIT_COMMAND_MODE, "CN");
    ASSERT_STREQ(at_command::SOURCE_ADDRESS_16_BIT, "MY");
    ASSERT_STREQ(at_command::SERIAL_NUMBER_HIGH, "SH");
    ASSERT_STREQ(at_command::SERIAL_NUMBER_LOW, "SL");
    ASSERT_STREQ(at_command::SOFTWARE_RESET, "FR");
    ASSERT_STREQ(at_command::MAC_MODE, "MM");
    ASSERT_STREQ(at_command::INTERFACE_DATA_RATE, "BD");
    ASSERT_STREQ(at_command::PERSONAL_AREA_NETWORK_ID, "ID");
    ASSERT_STREQ(at_command::CHANNEL, "CH");
    ASSERT_STREQ(at_command::XBEE_RETRIES, "RR");
    ASSERT_STREQ(at_command::POWER_LEVEL, "PL");
    ASSERT_STREQ(at_command::CCA_THRESHOLD, "CA");
    ASSERT_STREQ(at_command::RESTORE_DEFAULTS, "RE");
    ASSERT_STREQ(at_command::WRITE, "WR");
    ASSERT_STREQ(at_command::REGISTER_QUERY, "");
    ASSERT_EQ(at_command::CR, '\r');
}

TEST(ATCommandTest, OperatorStringTestQuery)
{
    ASSERT_EQ(static_cast<std::string>(at_command(at_command::API_ENABLE)), "ATAP\r");
}

TEST(ATCommandTest, OperatorStringTestArgument)
{
    ASSERT_EQ(static_cast<std::string>(at_command(at_command::INTERFACE_DATA_RATE, "1")), "ATBD1\r");
}
