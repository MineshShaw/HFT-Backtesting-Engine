#include "hft/data/csv_parser.hpp"
#include <gtest/gtest.h>

#include <string_view>

using namespace hft::data;

TEST(CsvParserTest, ParsesValidRow)
{
    std::string_view input =
        "1625097600,BTC,35000.5,1.2\n";

    CsvParser parser(input);
    TickView tick;

    bool result = parser.next(tick);

    EXPECT_TRUE(result);

    EXPECT_EQ(tick.timestamp, "1625097600");
    EXPECT_EQ(tick.symbol, "BTC");
    EXPECT_EQ(tick.price, "35000.5");
    EXPECT_EQ(tick.size, "1.2");
}

TEST(CsvParserTest, HandlesEmptyInput)
{
    std::string_view input = "";
    CsvParser parser(input);
    TickView tick;

    bool result = parser.next(tick);

    EXPECT_FALSE(result);
}

TEST(CsvParserTest, HandlesInvalidRow)
{
    std::string_view input =
        "INVALID, ROW\n"
        "1625097601,ETH,2200.5,0.8\n";

    CsvParser parser(input);
    TickView tick;

    bool result = parser.next(tick);

    EXPECT_TRUE(result);

    EXPECT_EQ(tick.timestamp, "1625097601");
    EXPECT_EQ(tick.symbol, "ETH");
    EXPECT_EQ(tick.price, "2200.5");
    EXPECT_EQ(tick.size, "0.8");
}

TEST(CsvParserTest, HandlesEmptyRow)
{
    std::string_view input =
        "1625097600,BTC,35000.5,1.2\n"
        "\n"
        "1625097601,ETH,2200.5,0.8\n";

    CsvParser parser(input);
    TickView tick;

    bool result = parser.next(tick);

    EXPECT_TRUE(result);

    EXPECT_EQ(tick.timestamp, "1625097600");
    EXPECT_EQ(tick.symbol, "BTC");
    EXPECT_EQ(tick.price, "35000.5");
    EXPECT_EQ(tick.size, "1.2");

    result = parser.next(tick);

    EXPECT_TRUE(result);

    EXPECT_EQ(tick.timestamp, "1625097601");
    EXPECT_EQ(tick.symbol, "ETH");
    EXPECT_EQ(tick.price, "2200.5");
    EXPECT_EQ(tick.size, "0.8");
}

TEST(CsvParserTest, HandlesFinalLineWithoutNewline)
{
    std::string_view input =
        "1625097600,BTC,35000.5,1.2\n"
        "1625097601,ETH,2200.5,0.8"; // No newline at the end

    CsvParser parser(input);
    TickView tick;

    bool result = parser.next(tick);

    EXPECT_TRUE(result);

    EXPECT_EQ(tick.timestamp, "1625097600");
    EXPECT_EQ(tick.symbol, "BTC");
    EXPECT_EQ(tick.price, "35000.5");
    EXPECT_EQ(tick.size, "1.2");

    result = parser.next(tick);

    EXPECT_TRUE(result);

    EXPECT_EQ(tick.timestamp, "1625097601");
    EXPECT_EQ(tick.symbol, "ETH");
    EXPECT_EQ(tick.price, "2200.5");
    EXPECT_EQ(tick.size, "0.8");

    result = parser.next(tick);
    EXPECT_FALSE(result); // No more rows
}

TEST(CsvParserTest, HandlesCRLFLineEndings)
{
    std::string_view input =
        "1625097600,BTC,35000.5,1.2\r\n"
        "1625097601,ETH,2200.5,0.8\r\n";

    CsvParser parser(input);
    TickView tick;

    bool result = parser.next(tick);

    EXPECT_TRUE(result);

    EXPECT_EQ(tick.timestamp, "1625097600");
    EXPECT_EQ(tick.symbol, "BTC");
    EXPECT_EQ(tick.price, "35000.5");
    EXPECT_EQ(tick.size, "1.2");

    result = parser.next(tick);

    EXPECT_TRUE(result);

    EXPECT_EQ(tick.timestamp, "1625097601");
    EXPECT_EQ(tick.symbol, "ETH");
    EXPECT_EQ(tick.price, "2200.5");
    EXPECT_EQ(tick.size, "0.8");

    result = parser.next(tick);
    EXPECT_FALSE(result); // No more rows
}

TEST(CsvParserTest, HandlesExtraColumns)
{
    std::string_view input =
        "1625097600,BTC,35000.5,1.2,EXTRA_COLUMN\n"
        "1625097601,ETH,2200.5,0.8\n";

    CsvParser parser(input);
    TickView tick;

    bool result = parser.next(tick);

    EXPECT_TRUE(result);

    EXPECT_EQ(tick.timestamp, "1625097601");
    EXPECT_EQ(tick.symbol, "ETH");
    EXPECT_EQ(tick.price, "2200.5");
    EXPECT_EQ(tick.size, "0.8");

    result = parser.next(tick);
    EXPECT_FALSE(result); // No more rows
}

TEST(CsvParserTest, HandlesEmptyRequiredFields)
{
    std::string_view input =
        "1625097600,,35000.5,1.2\n" // Empty symbol
        "1625097601,ETH,,0.8\n"     // Empty price
        "1625097602,BTC,35000.5,1.0\n"; // Correct row

    CsvParser parser(input);
    TickView tick;

    bool result = parser.next(tick);

    EXPECT_TRUE(result);

    EXPECT_EQ(tick.timestamp, "1625097602");
    EXPECT_EQ(tick.symbol, "BTC");
    EXPECT_EQ(tick.price, "35000.5");
    EXPECT_EQ(tick.size, "1.0");
}