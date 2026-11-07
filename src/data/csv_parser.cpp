
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

    tempTick.mTimestamp = line.substr(0, firstComma);

    auto secondComma = line.find(',', firstComma + 1);
    
    if (secondComma == std::string_view::npos)
        return false;

    tempTick.mSymbol = line.substr(firstComma + 1, secondComma - firstComma - 1);

    auto thirdComma = line.find(',', secondComma + 1);

    if (thirdComma == std::string_view::npos)
        return false;

    tempTick.mPrice = line.substr(secondComma + 1, thirdComma - secondComma - 1);

    if (line.find(',', thirdComma + 1) != std::string_view::npos)
        return false;

    tempTick.mSize = line.substr(thirdComma + 1);

    tempTick.mTimestamp = trim(tempTick.mTimestamp);
    tempTick.mSymbol = trim(tempTick.mSymbol);
    tempTick.mPrice = trim(tempTick.mPrice);
    tempTick.mSize = trim(tempTick.mSize);

    if (tempTick.mTimestamp.size() > 0 &&
        tempTick.mSymbol.size() > 0 &&
        tempTick.mPrice.size() > 0 &&
        tempTick.mSize.size() > 0) {
                tick = tempTick;
                return true;
    }

    return false;
}


} // namespace hft::data
