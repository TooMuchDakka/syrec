#include "core/syrec/parser/parser_utilities.hpp"

#include <fmt/format.h>

using namespace parser;

std::string ParserUtilities::createError(size_t line, size_t column, const std::string& errorMessage) {
    return fmt::format(messageFormat, line, column, errorMessage);
}

std::string ParserUtilities::createWarning(size_t line, size_t column, const std::string& warningMessage) {
    return fmt::format(messageFormat, line, column, warningMessage);
}

std::optional<unsigned int> ParserUtilities::convertToNumber(const antlr4::Token* token) {
    if (token == nullptr) {
        return std::nullopt;
    }

    return convertToNumber(token->getText());
}

std::optional<unsigned int> ParserUtilities::convertToNumber(const std::string& tokenText) {
    try {
        return std::stoul(tokenText) & UINT_MAX;
    } catch (std::invalid_argument&) {
        return std::nullopt;
    } catch (std::out_of_range&) {
        return std::nullopt;
    }
}