#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "util.h"

namespace util_test
{
    TEST(UtilTest, PrefixNullTerminatorEmptyString)
    {
        ASSERT_EQ(util::prefix_null_terminator(""), std::string("\0", 1));
    }

    TEST(UtilTest, PrefixNullTerminatorNonemptyString)
    {
        const std::string any_string = "test";
        ASSERT_EQ(util::prefix_null_terminator(any_string), '\0' + any_string);
    }

    TEST(UtilTest, GetEscapedStringEmptySource)
    {
        ASSERT_EQ("", util::get_escaped_string(""));
    }

    TEST(UtilTest, GetEscapedStringNoEscapeSequencesInSource)
    {
        ASSERT_EQ("test", util::get_escaped_string("test"));
    }

    TEST(UtilTest, GetEscapedStringNewlinesInSource)
    {
        ASSERT_EQ("test\\r\\n", util::get_escaped_string("test\r\n"));
    }

    TEST(UtilTest, StripNewlineEmptySource)
    {
        ASSERT_EQ("", util::strip_newline(""));
    }

    TEST(UtilTest, StripNewlineNoNewlinesInSource)
    {
        ASSERT_EQ("test", util::strip_newline("test"));
    }

    TEST(UtilTest, StripNewlineNewlinesInSource)
    {
        ASSERT_EQ("test", util::strip_newline("test\r\n"));
    }

    TEST(UtilTest, GetFrameHexEmptyFrame)
    {
        ASSERT_EQ("", util::get_frame_hex(std::vector<uint8_t>()));
    }

    TEST(UtilTest, GetFrameHexNonemptyFrame)
    {
        std::vector<uint8_t> frame{0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89};
        ASSERT_EQ("ab cd ef 01 23 45 67 89", util::get_frame_hex(frame));
    }

    TEST(UtilTest, GetFrameHexNonemptyFrameShowPrefix)
    {
        std::vector<uint8_t> frame{0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89};
        ASSERT_EQ("0xab 0xcd 0xef 0x01 0x23 0x45 0x67 0x89", util::get_frame_hex(frame, true));
    }

    TEST(UtilTest, SplitEmptySource)
    {
        std::vector<std::string> tokens = util::split("", ",");
        ASSERT_EQ(std::vector<std::string>(), tokens);
    }

    TEST(UtilTest, SplitNoSeparatorInSource)
    {
        std::vector<std::string> tokens = util::split("test1 test2", ",");
        ASSERT_EQ(std::vector<std::string>({"test1 test2"}), tokens);
    }

    TEST(UtilTest, SplitSeparatorsInSource)
    {
        std::vector<std::string> tokens = util::split("test1,test2,test3", ",");
        ASSERT_EQ(std::vector<std::string>({"test1", "test2", "test3"}), tokens);
    }

    TEST(UtilTest, SplitNoEmptyTokens)
    {
        std::vector<std::string> tokens = util::split("test1,,test2", ",");
        ASSERT_EQ(std::vector<std::string>({"test1", "test2"}), tokens);
    }

    TEST(UtilTest, TryParseValidUint32)
    {
        uint32_t i;
        ASSERT_TRUE(util::try_parse_uint32_t("123456789", i));
        ASSERT_EQ(123456789, i);
    }

    TEST(UtilTest, TryParseInvalidUint32)
    {
        uint32_t i;
        ASSERT_FALSE(util::try_parse_uint32_t("test", i));
    }

    TEST(UtilTest, UnpackBytesToWidthUint8)
    {
        std::vector<uint8_t> buffer{0x0f};
        uint8_t unpacked = util::unpack_bytes_to_width<uint8_t>(buffer.cbegin());
        ASSERT_EQ(0x0f, unpacked);
    }

    TEST(UtilTest, UnpackBytesToWidthUint64)
    {
        std::vector<uint8_t> buffer{0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89};
        uint64_t unpacked = util::unpack_bytes_to_width<uint64_t>(buffer.cbegin());
        ASSERT_EQ(0xabcdef0123456789, unpacked);
    }

    TEST(UtilTest, UnpackBytesToWidthZeroPaddedUint64)
    {
        std::vector<uint8_t> buffer{0x00, 0x00, 0x00, 0x01, 0x23, 0x45, 0x67, 0x89};
        uint64_t unpacked = util::unpack_bytes_to_width<uint64_t>(buffer.cbegin());
        ASSERT_EQ(0x123456789, unpacked);
    }

    TEST(UtilTest, UnpackBytesToWidthZeroUint64)
    {
        std::vector<uint8_t> buffer(sizeof(uint64_t), 0x00);
        uint64_t unpacked = util::unpack_bytes_to_width<uint64_t>(buffer.cbegin());
        ASSERT_EQ(0, unpacked);
    }

    TEST(UtilTest, PackValueAsBytesUint8)
    {
        std::vector<uint8_t> buffer;
        util::pack_value_as_bytes(std::back_inserter(buffer), uint8_t(0x0f));
        ASSERT_EQ(std::vector<uint8_t>({0x0f}), buffer);
    }

    TEST(UtilTest, PackValueAsBytesUint64)
    {
        std::vector<uint8_t> buffer;
        util::pack_value_as_bytes(std::back_inserter(buffer), uint64_t(0xabcdef0123456789));
        ASSERT_EQ(std::vector<uint8_t>({0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89}), buffer);
    }

    TEST(UtilTest, PackValueAsBytesZeroPaddedUint64)
    {
        std::vector<uint8_t> buffer;
        util::pack_value_as_bytes(std::back_inserter(buffer), uint64_t(0x123456789));
        ASSERT_EQ(std::vector<uint8_t>({0x00, 0x00, 0x00, 0x01, 0x23, 0x45, 0x67, 0x89}), buffer);
    }

    TEST(UtilTest, PackValueAsBytesNonemptyBuffer)
    {
        std::vector<uint8_t> buffer{0x00, 0x11, 0x22};
        util::pack_value_as_bytes(std::back_inserter(buffer), uint32_t(0xabcdef01));
        ASSERT_EQ(std::vector<uint8_t>({0x00, 0x11, 0x22, 0xab, 0xcd, 0xef, 0x01}), buffer);
    }

    // verifies that uint8_t/char is printed as an integer and not as a character
    TEST(UtilTest, ToHexStringPromoteCharacterTypeToIntegralType)
    {
        ASSERT_EQ(util::to_hex_string(uint8_t(0x0f)), "0x0f");
    }

    TEST(UtilTest, ToHexStringUint64)
    {
        ASSERT_EQ(util::to_hex_string(uint64_t(0xabcdef0123456789)), "0xabcdef0123456789");
    }

    TEST(UtilTest, ToHexStringZeroPaddedUint64)
    {
        ASSERT_EQ(util::to_hex_string(uint64_t(0x123456789)), "0x0000000123456789");
    }
}
