#ifndef ANTLR_VISITOR_UTILITIES_HPP
#define ANTLR_VISITOR_UTILITIES_HPP

#include "Token.h"

#include <optional>
#include <string>

#pragma once

namespace parser {
class ParserUtilities {
private:
    inline static const std::string messageFormat = "-- line {0:d} col {1:d}: {2:s}";

public:
    [[nodiscard]] static std::string createError(size_t line, size_t column, const std::string& errorMessage);
    [[nodiscard]] static std::string createWarning(size_t line, size_t column, const std::string& warningMessage);

    [[nodiscard]] static std::optional<unsigned int> convertToNumber(const antlr4::Token* token);
    [[nodiscard]] static std::optional<unsigned int> convertToNumber(const std::string& tokenText);    
};
}

#endif