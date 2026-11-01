
#include "hft/data/csv_parser.hpp"

namespace hft::data {

CsvParser::CsvParser(std::string_view input) : mInput(input), mPosition(0) {}

constexpr std::string_view trim(std::string_view input) noexcept {
    const auto start = input.find_first_not_of(" \t\n\r\f\v");
    
    if (start == std::string_view::npos) {
        return {};
    }
    
    const auto end = input.find_last_not_of(" \t\n\r\f\v");
    
    return input.substr(start, end - start + 1);
}

CsvParseResult CsvParser::next(TickView& tick)
{
    if (mPosition < mInput.size()) {
        auto newlinePos = mInput.find('\n', mPosition);

        bool hasNewline = (newlinePos != std::string_view::npos);
        
        if (!hasNewline)
            newlinePos = mInput.size();

        auto currentLine = mInput.substr(
            mPosition,
            newlinePos - mPosition
        );

        if (!currentLine.empty() && currentLine.back() == '\r') {
            currentLine.remove_suffix(1);
        }

        if (hasNewline)
            mPosition = newlinePos + 1;
        else
            mPosition = mInput.size();

        if (!currentLine.empty() && parse_line(currentLine, tick))
            return CsvParseResult::ValidRecord;

        return CsvParseResult::InvalidRecord;
    }
    return CsvParseResult::EndOfInput;
}


bool CsvParser::parse_line(std::string_view line, TickView& tick)
{
    TickView tempTick;
    auto firstComma = line.find(',');

    if (firstComma == std::string_view::npos)
        return false;

    tempTick.timestamp = line.substr(0, firstComma);

    auto secondComma = line.find(',', firstComma + 1);
    
    if (secondComma == std::string_view::npos)
        return false;

    tempTick.symbol = line.substr(firstComma + 1, secondComma - firstComma - 1);

    auto thirdComma = line.find(',', secondComma + 1);

    if (thirdComma == std::string_view::npos)
        return false;

    tempTick.price = line.substr(secondComma + 1, thirdComma - secondComma - 1);

    if (line.find(',', thirdComma + 1) != std::string_view::npos)
        return false;

    tempTick.size = line.substr(thirdComma + 1);

    tempTick.timestamp = trim(tempTick.timestamp);
    tempTick.symbol = trim(tempTick.symbol);
    tempTick.price = trim(tempTick.price);
    tempTick.size = trim(tempTick.size);

    if (tempTick.timestamp.size() > 0 &&
        tempTick.symbol.size() > 0 &&
        tempTick.price.size() > 0 &&
        tempTick.size.size() > 0) {
                tick = tempTick;
                return true;
    }

    return false;
}


} // namespace hft::data
