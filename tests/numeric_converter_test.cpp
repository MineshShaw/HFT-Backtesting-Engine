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