
#pragma once

#include <cstddef>
#include <string_view>

namespace hft::data {

enum class CsvParseResult {
    ValidRecord,
    InvalidRecord,
    EndOfInput
};

struct TickView {
    std::string_view timestamp;
    std::string_view symbol;
    std::string_view price;
    std::string_view size;
};

class CsvParser {
public:
    CsvParser() : mInput{}, mPosition(0) {}
    
    explicit CsvParser(std::string_view input);

    CsvParseResult next(TickView& tick);

private:
    std::string_view mInput;
    std::size_t mPosition{0};

    bool parse_line(std::string_view line, TickView& tick);
};

} // namespace hft::data
