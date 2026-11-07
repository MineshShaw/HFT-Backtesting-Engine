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

    const auto result = parser.next(tick);

    EXPECT_EQ(CsvParseResult::ValidRecord, result);

    EXPECT_EQ(tick.mTimestamp, "1625097600");
    EXPECT_EQ(tick.mSymbol, "BTC");
    EXPECT_EQ(tick.mPrice, "35000.5");
    EXPECT_EQ(tick.mSize, "1.2");
}

TEST(CsvParserTest, HandlesEmptyInput)
{
    std::string_view input = "";
    CsvParser parser(input);
    TickView tick;

    const auto result = parser.next(tick);

    EXPECT_EQ(CsvParseResult::EndOfInput, result);
}

TEST(CsvParserTest, HandlesInvalidRow)
{
    std::string_view input =
        "INVALID, ROW\n"
        "1625097601,ETH,2200.5,0.8\n";

    CsvParser parser(input);
    TickView tick;

    auto result = parser.next(tick);

    EXPECT_EQ(CsvParseResult::InvalidRecord, result);

    result = parser.next(tick);

    EXPECT_EQ(CsvParseResult::ValidRecord, result);

    EXPECT_EQ(tick.mTimestamp, "1625097601");
    EXPECT_EQ(tick.mSymbol, "ETH");
    EXPECT_EQ(tick.mPrice, "2200.5");
    EXPECT_EQ(tick.mSize, "0.8");
}

TEST(CsvParserTest, HandlesEmptyRow)
{
    std::string_view input =
        "1625097600,BTC,35000.5,1.2\n"
        "\n"
        "1625097601,ETH,2200.5,0.8\n";

    CsvParser parser(input);
    TickView tick;

    auto result = parser.next(tick);

    EXPECT_EQ(CsvParseResult::ValidRecord, result);

    EXPECT_EQ(tick.mTimestamp, "1625097600");
    EXPECT_EQ(tick.mSymbol, "BTC");
    EXPECT_EQ(tick.mPrice, "35000.5");
    EXPECT_EQ(tick.mSize, "1.2");

    result = parser.next(tick);

    EXPECT_EQ(CsvParseResult::InvalidRecord, result);

    result = parser.next(tick);

    EXPECT_EQ(CsvParseResult::ValidRecord, result);

    EXPECT_EQ(tick.mTimestamp, "1625097601");
    EXPECT_EQ(tick.mSymbol, "ETH");
    EXPECT_EQ(tick.mPrice, "2200.5");
    EXPECT_EQ(tick.mSize, "0.8");
}

TEST(CsvParserTest, HandlesFinalLineWithoutNewline)
{
    std::string_view input =
        "1625097600,BTC,35000.5,1.2\n"
        "1625097601,ETH,2200.5,0.8"; // No newline at the end

    CsvParser parser(input);
    TickView tick;

    auto result = parser.next(tick);

    EXPECT_EQ(CsvParseResult::ValidRecord, result);

    EXPECT_EQ(tick.mTimestamp, "1625097600");
    EXPECT_EQ(tick.mSymbol, "BTC");
    EXPECT_EQ(tick.mPrice, "35000.5");
    EXPECT_EQ(tick.mSize, "1.2");

    result = parser.next(tick);

    EXPECT_EQ(CsvParseResult::ValidRecord, result);

    EXPECT_EQ(tick.mTimestamp, "1625097601");
    EXPECT_EQ(tick.mSymbol, "ETH");
    EXPECT_EQ(tick.mPrice, "2200.5");
    EXPECT_EQ(tick.mSize, "0.8");

    result = parser.next(tick);
    EXPECT_EQ(CsvParseResult::EndOfInput, result); // No more rows
}

TEST(CsvParserTest, HandlesCRLFLineEndings)
{
    std::string_view input =
        "1625097600,BTC,35000.5,1.2\r\n"
        "1625097601,ETH,2200.5,0.8\r\n";

    CsvParser parser(input);
    TickView tick;

    auto result = parser.next(tick);

    EXPECT_EQ(CsvParseResult::ValidRecord, result);

    EXPECT_EQ(tick.mTimestamp, "1625097600");
    EXPECT_EQ(tick.mSymbol, "BTC");
    EXPECT_EQ(tick.mPrice, "35000.5");
    EXPECT_EQ(tick.mSize, "1.2");

    result = parser.next(tick);

    EXPECT_EQ(CsvParseResult::ValidRecord, result);

    EXPECT_EQ(tick.mTimestamp, "1625097601");
    EXPECT_EQ(tick.mSymbol, "ETH");
    EXPECT_EQ(tick.mPrice, "2200.5");
    EXPECT_EQ(tick.mSize, "0.8");

    result = parser.next(tick);
    EXPECT_EQ(CsvParseResult::EndOfInput, result); // No more rows
}

TEST(CsvParserTest, HandlesExtraColumns)
{
    std::string_view input =
        "1625097600,BTC,35000.5,1.2,EXTRA_COLUMN\n"
        "1625097601,ETH,2200.5,0.8\n";

    CsvParser parser(input);
    TickView tick;

    auto result = parser.next(tick);

    EXPECT_EQ(CsvParseResult::InvalidRecord, result);

    result = parser.next(tick);

    EXPECT_EQ(CsvParseResult::ValidRecord, result);

    EXPECT_EQ(tick.mTimestamp, "1625097601");
    EXPECT_EQ(tick.mSymbol, "ETH");
    EXPECT_EQ(tick.mPrice, "2200.5");
    EXPECT_EQ(tick.mSize, "0.8");

    result = parser.next(tick);
    EXPECT_EQ(CsvParseResult::EndOfInput, result); // No more rows
}

TEST(CsvParserTest, HandlesEmptyRequiredFields)
{
    std::string_view input =
        "1625097600,,35000.5,1.2\n" // Empty mSymbol
        "1625097601,ETH,,0.8\n"     // Empty mPrice
        "1625097602,BTC,35000.5,1.0\n"; // Correct row

    CsvParser parser(input);
    TickView tick;

    auto result = parser.next(tick);

    EXPECT_EQ(CsvParseResult::InvalidRecord, result);

    result = parser.next(tick);

    EXPECT_EQ(CsvParseResult::InvalidRecord, result);

    result = parser.next(tick);

    EXPECT_EQ(CsvParseResult::ValidRecord, result);

    EXPECT_EQ(tick.mTimestamp, "1625097602");
    EXPECT_EQ(tick.mSymbol, "BTC");
    EXPECT_EQ(tick.mPrice, "35000.5");
    EXPECT_EQ(tick.mSize, "1.0");
}

TEST(CsvParserTest, HandlesDefaultParser) {
    CsvParser parser;
    TickView tick;

    const auto result = parser.next(tick);

    EXPECT_EQ(CsvParseResult::EndOfInput, result); // No input provided, should return false
}