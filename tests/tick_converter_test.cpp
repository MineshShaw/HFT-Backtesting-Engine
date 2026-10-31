#include "hft/data/tick_converter.hpp"

#include <gtest/gtest.h>

using namespace hft::data;

TEST(TickConverterTest, ConvertsValidTick) {
    TickView input{"1625097600", "AAPL", "35000.5", "100"};
    Tick output;

    bool result = convert_tick(input, output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output.timestamp, 1625097600);
    EXPECT_EQ(output.symbol, "AAPL");
    EXPECT_DOUBLE_EQ(output.price, 35000.5);
    EXPECT_DOUBLE_EQ(output.size, 100);
}

TEST(TickConverterTest, RejectsInvalidTimestamp) {
    TickView input{"invalid", "AAPL", "35000.5", "100"};
    Tick output;

    bool result = convert_tick(input, output);

    EXPECT_FALSE(result);
}

TEST(TickConverterTest, RejectsInvalidPrice) {
    TickView input{"1625097600", "AAPL", "invalid", "100"};
    Tick output;

    bool result = convert_tick(input, output);

    EXPECT_FALSE(result);
}

TEST(TickConverterTest, RejectsInvalidSize) {
    TickView input{"1625097600", "AAPL", "35000.5", "invalid"};
    Tick output;

    bool result = convert_tick(input, output);

    EXPECT_FALSE(result);
}

TEST(TickConverterTest, PreservesOutputOnFailure) {
    TickView input{"invalid", "AAPL", "35000.5", "100"};
    Tick output{1234567890, "OLD", 123.45, 10};

    bool result = convert_tick(input, output);

    EXPECT_FALSE(result);
    EXPECT_EQ(output.timestamp, 1234567890);
    EXPECT_EQ(output.symbol, "OLD");
    EXPECT_DOUBLE_EQ(output.price, 123.45);
    EXPECT_DOUBLE_EQ(output.size, 10);
}

TEST(TickConverterTest, PreservesSymbolView) {
    std::string symbol_str = "AAPL";
    TickView input{"1625097600", symbol_str, "35000.5", "100"};
    Tick output;

    bool result = convert_tick(input, output);

    EXPECT_TRUE(result);
    EXPECT_EQ(output.symbol, symbol_str);
}