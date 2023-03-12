#include "core/syrec/parser/parser_utilities.hpp"

#include "core/syrec/expression.hpp"
#include "core/syrec/statement.hpp"
#include "core/syrec/parser/infix_iterator.hpp"

#include <sstream>
#include <fmt/format.h>

using namespace parser;

std::string ParserUtilities::combineErrors(const std::vector<std::string>& errors, char const* errorDelimiter) {
    std::ostringstream errorsConcatinatedBuffer;
    std::copy(errors.cbegin(), errors.cend(), infix_ostream_iterator<std::string>(errorsConcatinatedBuffer, errorDelimiter));
    return errorsConcatinatedBuffer.str();
}

std::vector<std::string> ParserUtilities::splitCombinedErrors(const std::string& combinedErrors, char const* errorDelimiter) {
    if (combinedErrors.empty()) {
        return {};
    }

    std::vector<std::string> actualErrors{};
    size_t                   previousErrorEndPosition = 0;
    size_t                   currErrorEndPosition     = combinedErrors.find_first_of(errorDelimiter, previousErrorEndPosition);
    bool                     foundErrorSeparator;
    do {
        foundErrorSeparator = std::string::npos != currErrorEndPosition;
        if (foundErrorSeparator) {
            actualErrors.emplace_back(combinedErrors.substr(previousErrorEndPosition, (currErrorEndPosition - previousErrorEndPosition)));
            previousErrorEndPosition = currErrorEndPosition + 1;
            currErrorEndPosition     = combinedErrors.find_first_of(errorDelimiter, previousErrorEndPosition);
        } else {
            currErrorEndPosition = combinedErrors.size();
            actualErrors.emplace_back(combinedErrors.substr(previousErrorEndPosition, (currErrorEndPosition - previousErrorEndPosition)));
        }
    } while (foundErrorSeparator);
    return actualErrors;
}


std::string ParserUtilities::createError(size_t line, size_t column, const std::string& errorMessage) {
    return fmt::format(messageFormat, line, column, errorMessage);
}

std::string ParserUtilities::createWarning(size_t line, size_t column, const std::string& warningMessage) {
    return fmt::format(messageFormat, line, column, warningMessage);
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

std::optional<unsigned int> ParserUtilities::mapOperationToInternalFlag(const syrec_operation::operation& operation) {
    std::optional<unsigned int> internalOperationFlag;

    switch (operation) {
        case syrec_operation::operation::add_assign:
            internalOperationFlag.emplace(syrec::AssignStatement::Add);
            break;
        case syrec_operation::operation::minus_assign:
            internalOperationFlag.emplace(syrec::AssignStatement::Subtract);
            break;
        case syrec_operation::operation::xor_assign:
            internalOperationFlag.emplace(syrec::AssignStatement::Exor);
            break;
        case syrec_operation::operation::increment_assign:
            internalOperationFlag.emplace(syrec::UnaryStatement::Increment);
            break;
        case syrec_operation::operation::decrement_assign:
            internalOperationFlag.emplace(syrec::UnaryStatement::Decrement);
            break;
        case syrec_operation::operation::invert_assign:
            internalOperationFlag.emplace(syrec::UnaryStatement::Invert);
            break;
        case syrec_operation::operation::shift_left:
            internalOperationFlag.emplace(syrec::ShiftExpression::Left);
            break;
        case syrec_operation::operation::shift_right:
            internalOperationFlag.emplace(syrec::ShiftExpression::Right);
            break;
        case syrec_operation::operation::addition:
            internalOperationFlag.emplace(syrec::BinaryExpression::Add);
            break;
        case syrec_operation::operation::subtraction:
            internalOperationFlag.emplace(syrec::BinaryExpression::Subtract);
            break;
        case syrec_operation::operation::multiplication:
            internalOperationFlag.emplace(syrec::BinaryExpression::Multiply);
            break;
        case syrec_operation::operation::division:
            internalOperationFlag.emplace(syrec::BinaryExpression::Divide);
            break;
        case syrec_operation::operation::modulo:
            internalOperationFlag.emplace(syrec::BinaryExpression::Modulo);
            break;
        case syrec_operation::operation::upper_bits_multiplication:
            internalOperationFlag.emplace(syrec::BinaryExpression::FracDivide);
            break;
        case syrec_operation::operation::bitwise_xor:
            internalOperationFlag.emplace(syrec::BinaryExpression::Exor);
            break;
        case syrec_operation::operation::logical_and:
            internalOperationFlag.emplace(syrec::BinaryExpression::LogicalAnd);
            break;
        case syrec_operation::operation::logical_or:
            internalOperationFlag.emplace(syrec::BinaryExpression::LogicalOr);
            break;
        case syrec_operation::operation::bitwise_and:
            internalOperationFlag.emplace(syrec::BinaryExpression::BitwiseAnd);
            break;
        case syrec_operation::operation::bitwise_or:
            internalOperationFlag.emplace(syrec::BinaryExpression::BitwiseOr);
            break;
        case syrec_operation::operation::less_than:
            internalOperationFlag.emplace(syrec::BinaryExpression::LessThan);
            break;
        case syrec_operation::operation::greater_than:
            internalOperationFlag.emplace(syrec::BinaryExpression::GreaterThan);
            break;
        case syrec_operation::operation::equals:
            internalOperationFlag.emplace(syrec::BinaryExpression::Equals);
            break;
        case syrec_operation::operation::not_equals:
            internalOperationFlag.emplace(syrec::BinaryExpression::NotEquals);
            break;
        case syrec_operation::operation::less_equals:
            internalOperationFlag.emplace(syrec::BinaryExpression::LessEquals);
            break;
        case syrec_operation::operation::greater_equals:
            internalOperationFlag.emplace(syrec::BinaryExpression::GreaterEquals);
            break;
        default:
            break;
    }
    return internalOperationFlag;
}

static std::optional<syrec_operation::operation> mapInternalBinaryOperationFlagToEnum(const unsigned int& internalOperationFlag) {
    std::optional<syrec_operation::operation> mappedToEnumValue;

    switch (internalOperationFlag) {
        case syrec::BinaryExpression::Add:
            mappedToEnumValue.emplace(syrec_operation::operation::addition);
            break;
        case syrec::BinaryExpression::BitwiseAnd:
            mappedToEnumValue.emplace(syrec_operation::operation::bitwise_and);
            break;
        case syrec::BinaryExpression::BitwiseOr:
            mappedToEnumValue.emplace(syrec_operation::operation::bitwise_or);
            break;
        case syrec::BinaryExpression::Divide:
            mappedToEnumValue.emplace(syrec_operation::operation::division);
            break;
        case syrec::BinaryExpression::Equals:
            mappedToEnumValue.emplace(syrec_operation::operation::equals);
            break;
        case syrec::BinaryExpression::Exor:
            mappedToEnumValue.emplace(syrec_operation::operation::bitwise_xor);
            break;
        case syrec::BinaryExpression::FracDivide:
            mappedToEnumValue.emplace(syrec_operation::operation::upper_bits_multiplication);
            break;
        case syrec::BinaryExpression::GreaterEquals:
            mappedToEnumValue.emplace(syrec_operation::operation::greater_equals);
            break;
        case syrec::BinaryExpression::GreaterThan:
            mappedToEnumValue.emplace(syrec_operation::operation::greater_than);
            break;
        case syrec::BinaryExpression::LessEquals:
            mappedToEnumValue.emplace(syrec_operation::operation::less_equals);
            break;
        case syrec::BinaryExpression::LessThan:
            mappedToEnumValue.emplace(syrec_operation::operation::less_than);
            break;
        case syrec::BinaryExpression::LogicalAnd:
            mappedToEnumValue.emplace(syrec_operation::operation::logical_and);
            break;
        case syrec::BinaryExpression::LogicalOr:
            mappedToEnumValue.emplace(syrec_operation::operation::logical_or);
            break;
        case syrec::BinaryExpression::Modulo:
            mappedToEnumValue.emplace(syrec_operation::operation::modulo);
            break;
        case syrec::BinaryExpression::Multiply:
            mappedToEnumValue.emplace(syrec_operation::operation::multiplication);
            break;
        case syrec::BinaryExpression::NotEquals:
            mappedToEnumValue.emplace(syrec_operation::operation::not_equals);
            break;
        case syrec::BinaryExpression::Subtract:
            mappedToEnumValue.emplace(syrec_operation::operation::subtraction);
            break;
        default:
            break;
    }
    return mappedToEnumValue;
}