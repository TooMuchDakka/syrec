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

//std::optional<unsigned int> ParserUtilities::mapOperationToInternalFlag(const syrec_operation::operation& operation) {
//    std::optional<unsigned int> internalOperationFlag;
//
//    switch (operation) {
//        case syrec_operation::operation::AddAssign:
//            internalOperationFlag.emplace(syrec::AssignStatement::Add);
//            break;
//        case syrec_operation::operation::MinusAssign:
//            internalOperationFlag.emplace(syrec::AssignStatement::Subtract);
//            break;
//        case syrec_operation::operation::XorAssign:
//            internalOperationFlag.emplace(syrec::AssignStatement::Exor);
//            break;
//        case syrec_operation::operation::IncrementAssign:
//            internalOperationFlag.emplace(syrec::UnaryStatement::Increment);
//            break;
//        case syrec_operation::operation::DecrementAssign:
//            internalOperationFlag.emplace(syrec::UnaryStatement::Decrement);
//            break;
//        case syrec_operation::operation::InvertAssign:
//            internalOperationFlag.emplace(syrec::UnaryStatement::Invert);
//            break;
//        case syrec_operation::operation::ShiftLeft:
//            internalOperationFlag.emplace(syrec::ShiftExpression::Left);
//            break;
//        case syrec_operation::operation::ShiftRight:
//            internalOperationFlag.emplace(syrec::ShiftExpression::Right);
//            break;
//        case syrec_operation::operation::Addition:
//            internalOperationFlag.emplace(syrec::BinaryExpression::Add);
//            break;
//        case syrec_operation::operation::Subtraction:
//            internalOperationFlag.emplace(syrec::BinaryExpression::Subtract);
//            break;
//        case syrec_operation::operation::Multiplication:
//            internalOperationFlag.emplace(syrec::BinaryExpression::Multiply);
//            break;
//        case syrec_operation::operation::Division:
//            internalOperationFlag.emplace(syrec::BinaryExpression::Divide);
//            break;
//        case syrec_operation::operation::Modulo:
//            internalOperationFlag.emplace(syrec::BinaryExpression::Modulo);
//            break;
//        case syrec_operation::operation::UpperBitsMultiplication:
//            internalOperationFlag.emplace(syrec::BinaryExpression::FracDivide);
//            break;
//        case syrec_operation::operation::BitwiseXor:
//            internalOperationFlag.emplace(syrec::BinaryExpression::Exor);
//            break;
//        case syrec_operation::operation::LogicalAnd:
//            internalOperationFlag.emplace(syrec::BinaryExpression::LogicalAnd);
//            break;
//        case syrec_operation::operation::LogicalOr:
//            internalOperationFlag.emplace(syrec::BinaryExpression::LogicalOr);
//            break;
//        case syrec_operation::operation::BitwiseAnd:
//            internalOperationFlag.emplace(syrec::BinaryExpression::BitwiseAnd);
//            break;
//        case syrec_operation::operation::BitwiseOr:
//            internalOperationFlag.emplace(syrec::BinaryExpression::BitwiseOr);
//            break;
//        case syrec_operation::operation::LessThan:
//            internalOperationFlag.emplace(syrec::BinaryExpression::LessThan);
//            break;
//        case syrec_operation::operation::GreaterThan:
//            internalOperationFlag.emplace(syrec::BinaryExpression::GreaterThan);
//            break;
//        case syrec_operation::operation::Equals:
//            internalOperationFlag.emplace(syrec::BinaryExpression::Equals);
//            break;
//        case syrec_operation::operation::NotEquals:
//            internalOperationFlag.emplace(syrec::BinaryExpression::NotEquals);
//            break;
//        case syrec_operation::operation::LessEquals:
//            internalOperationFlag.emplace(syrec::BinaryExpression::LessEquals);
//            break;
//        case syrec_operation::operation::GreaterEquals:
//            internalOperationFlag.emplace(syrec::BinaryExpression::GreaterEquals);
//            break;
//        default:
//            break;
//    }
//    return internalOperationFlag;
//}
//
//std::optional<syrec_operation::operation> ParserUtilities::mapInternalBinaryOperationFlagToEnum(const unsigned int& internalOperationFlag) {
//    std::optional<syrec_operation::operation> mappedToEnumValue;
//
//    switch (internalOperationFlag) {
//        case syrec::BinaryExpression::Add:
//            mappedToEnumValue.emplace(syrec_operation::operation::Addition);
//            break;
//        case syrec::BinaryExpression::BitwiseAnd:
//            mappedToEnumValue.emplace(syrec_operation::operation::BitwiseAnd);
//            break;
//        case syrec::BinaryExpression::BitwiseOr:
//            mappedToEnumValue.emplace(syrec_operation::operation::BitwiseOr);
//            break;
//        case syrec::BinaryExpression::Divide:
//            mappedToEnumValue.emplace(syrec_operation::operation::Division);
//            break;
//        case syrec::BinaryExpression::Equals:
//            mappedToEnumValue.emplace(syrec_operation::operation::Equals);
//            break;
//        case syrec::BinaryExpression::Exor:
//            mappedToEnumValue.emplace(syrec_operation::operation::BitwiseXor);
//            break;
//        case syrec::BinaryExpression::FracDivide:
//            mappedToEnumValue.emplace(syrec_operation::operation::UpperBitsMultiplication);
//            break;
//        case syrec::BinaryExpression::GreaterEquals:
//            mappedToEnumValue.emplace(syrec_operation::operation::GreaterEquals);
//            break;
//        case syrec::BinaryExpression::GreaterThan:
//            mappedToEnumValue.emplace(syrec_operation::operation::GreaterThan);
//            break;
//        case syrec::BinaryExpression::LessEquals:
//            mappedToEnumValue.emplace(syrec_operation::operation::LessEquals);
//            break;
//        case syrec::BinaryExpression::LessThan:
//            mappedToEnumValue.emplace(syrec_operation::operation::LessThan);
//            break;
//        case syrec::BinaryExpression::LogicalAnd:
//            mappedToEnumValue.emplace(syrec_operation::operation::LogicalAnd);
//            break;
//        case syrec::BinaryExpression::LogicalOr:
//            mappedToEnumValue.emplace(syrec_operation::operation::LogicalOr);
//            break;
//        case syrec::BinaryExpression::Modulo:
//            mappedToEnumValue.emplace(syrec_operation::operation::Modulo);
//            break;
//        case syrec::BinaryExpression::Multiply:
//            mappedToEnumValue.emplace(syrec_operation::operation::Multiplication);
//            break;
//        case syrec::BinaryExpression::NotEquals:
//            mappedToEnumValue.emplace(syrec_operation::operation::NotEquals);
//            break;
//        case syrec::BinaryExpression::Subtract:
//            mappedToEnumValue.emplace(syrec_operation::operation::Subtraction);
//            break;
//        default:
//            break;
//    }
//    return mappedToEnumValue;
//}