#include "hft/data/numeric_converter.hpp"

#include <charconv>
#include <system_error>

#include <gtest/gtest.h>

using namespace hft::data;

TEST(NumericConverterTest, ParsesValidDouble)
{
    std::string_view input = "35000.5";
    double output;

    bool result = parse_double(input, output);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(output, 35000.5);
}

TEST(NumericConverterTest, RejectsInvalidDouble)
{
    std::string_view input = "invalid";
    double output;

    bool result = parse_double(input, output);

    EXPECT_FALSE(result);
}

TEST(NumericConverterTest, RejectsTrailingCharacters)
{
    std::string_view input = "35000.5abc";
    double output;

    bool result = parse_double(input, output);

    EXPECT_FALSE(result);
}

TEST(NumericConverterTest, ParsesValidInteger)
{
    std::string_view input = "1625097600";
    int64_t output;

    bool result = parse_integer(input, output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output, 1625097600);
}

TEST(NumericConverterTest, RejectsInvalidInteger)
{
    std::string_view input = "12abc";
    int64_t output;

    bool result = parse_integer(input, output);

    EXPECT_FALSE(result);
}

TEST(NumericConverterTest, RejectsEmptyInput)
{
    std::string_view input = "";
    double output;

    bool result = parse_double(input, output);

    EXPECT_FALSE(result);
}

TEST(NumericConverterTest, RejectsIntegerOverflow)
{
    std::string_view input = "9223372036854775808"; // One more than int64_t max
    int64_t output;

    bool result = parse_integer(input, output);

    EXPECT_FALSE(result);
}

TEST(NumericConverterTest, RejectsDoubleOverflow)
{
    std::string_view input = "1e309"; // Larger than double max
    double output;

    bool result = parse_double(input, output);

    EXPECT_FALSE(result);
}

TEST(NumericConverterTest, ParsesNegativeAndZeroInteger)
{
    std::string_view input = "-12345";
    int64_t output;

    bool result = parse_integer(input, output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output, -12345);

    input = "0";
    result = parse_integer(input, output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output, 0);
}

TEST(NumericConverterTest, ParsesNegativeAndZeroDouble)
{
    std::string_view input = "-12345.67";
    double output;

    bool result = parse_double(input, output);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(output, -12345.67);

    input = "0.0";
    result = parse_double(input, output);

    EXPECT_TRUE(result);
    EXPECT_DOUBLE_EQ(output, 0.0);
}

TEST(NumericConverterTest, RejectsWhitespaceInInteger)
{
    std::string_view input = " 12345 ";
    int64_t output;

    bool result = parse_integer(input, output);

    EXPECT_FALSE(result);
}

TEST(NumericConverterTest, RejectsWhitespaceInDouble)
{
    std::string_view input = " 12345.67 ";
    double output;

    bool result = parse_double(input, output);

    EXPECT_FALSE(result);
}

TEST(NumericConverterTest, RejectsMultipleDecimalPoints)
{
    std::string_view input = "123.45.67";
    double output;

    bool result = parse_double(input, output);

    EXPECT_FALSE(result);
}

TEST(NumericConverterTest, RejectsEmptyString)
{
    std::string_view input = "";
    double output;

    bool result = parse_double(input, output);

    EXPECT_FALSE(result);

    int64_t int_output;
    result = parse_integer(input, int_output);

    EXPECT_FALSE(result);
}

TEST(NumericConverterTest, RejectsNonNumericCharacters)
{
    std::string_view input = "123abc";
    double output;

    bool result = parse_double(input, output);

    EXPECT_FALSE(result);

    int64_t int_output;
    result = parse_integer(input, int_output);

    EXPECT_FALSE(result);
}