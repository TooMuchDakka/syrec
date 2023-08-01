#include "core/syrec/parser/operation.hpp"

#include "core/syrec/expression.hpp"
#include "core/syrec/statement.hpp"

using namespace syrec_operation;

bool syrec_operation::isOperandUsedAsLhsInOperationIdentityElement(operation operation, unsigned operand) noexcept {
    if (isCommutative(operation)) {
        return isOperandUseAsRhsInOperationIdentityElement(operation, operand);
    }

    switch (operation) {
        case operation::Addition:
        case operation::ShiftLeft:
        case operation::ShiftRight:
            return operand == 0;
        case operation::Multiplication:
            return operand == 1;
        default:
            return false;
    }
}

bool syrec_operation::isOperandUseAsRhsInOperationIdentityElement(const operation operation, const unsigned int operand) noexcept {
    switch (operation) {
        case operation::Addition:
        case operation::AddAssign:
        case operation::Subtraction:
        case operation::MinusAssign:
        case operation::ShiftLeft:
        case operation::ShiftRight:
            return operand == 0;
        case operation::Division:
        case operation::Multiplication:
        case operation::Modulo:
            return operand == 1;
        default:
            return false;
    }
}

std::optional<unsigned int> syrec_operation::apply(const operation operation, const unsigned int leftOperand, const unsigned int rightOperand) noexcept
{
    unsigned int result;
    bool         isValidBinaryOperation = true;
    switch (operation) {
        case operation::Addition:
        case operation::AddAssign:
            result = leftOperand + rightOperand;
            break;
        case operation::Subtraction:
        case operation::MinusAssign:
            result = leftOperand - rightOperand;
            break;
        case operation::Multiplication:
            // TODO: Implement correctly
        case operation::UpperBitsMultiplication:
            result = leftOperand * rightOperand;
            break;
        case operation::Division: {
            if (rightOperand == 0) {
                isValidBinaryOperation = false;
            } else {
                result = leftOperand / rightOperand;
            }
            break;   
        }
        case operation::BitwiseAnd:
            result = leftOperand & rightOperand;
            break;
        case operation::BitwiseOr:
            result = leftOperand | rightOperand;
            break;
        case operation::BitwiseXor:
        case operation::XorAssign:
            result = leftOperand ^ rightOperand;
            break;
        case operation::Modulo:
            result = leftOperand % rightOperand;
            break;
        case operation::LogicalAnd:
            result = leftOperand && rightOperand;
            break;
        case operation::LogicalOr:
            result = leftOperand || rightOperand;
            break;
        case operation::LessThan:
            result = leftOperand < rightOperand;
            break;
        case operation::GreaterThan:
            result = leftOperand > rightOperand;
            break;
        case operation::Equals:
            result = leftOperand == rightOperand;
            break;
        case operation::NotEquals:
            result = leftOperand != rightOperand;
            break;
        case operation::LessEquals:
            result = leftOperand <= rightOperand;
            break;
        case operation::GreaterEquals:
            result = leftOperand >= rightOperand;
            break;
        case operation::ShiftLeft:
            result = leftOperand << rightOperand;
            break;
        case operation::ShiftRight:
            result = leftOperand >> rightOperand;
            break;
        default:
            isValidBinaryOperation = false;
            break;
    }

    return isValidBinaryOperation ? std::optional(result) : std::nullopt;
}

std::optional<unsigned int> syrec_operation::apply(const operation operation, const unsigned int operand) noexcept {
    unsigned int result = 0;
    bool isValidUnaryOperation = true;

    switch (operation) {
        case operation::BitwiseNegation:
            result = ~operand;
            break;
        case operation::LogicalNegation:
            result = operand == 0 ? 1 : 0;
            break;
        case operation::IncrementAssign:
            result = operand + 1;
            break;
        case operation::DecrementAssign:
            result = operand - 1;
            break;
        case operation::InvertAssign:
            result = ~operand;
            break;
        default:
            isValidUnaryOperation = false;
    }

    return isValidUnaryOperation ? std::optional(result) : std::nullopt;
}

bool syrec_operation::isCommutative(operation operation) noexcept {
    switch (operation) {
        case operation::Addition:
        case operation::Multiplication:
        case operation::UpperBitsMultiplication:
        case operation::BitwiseAnd:
        case operation::BitwiseOr:
        case operation::BitwiseXor:
        case operation::LogicalAnd:
        case operation::LogicalOr:
        case operation::Equals:
        case operation::NotEquals:
            return true;
        default:
            return false;
    }
}

std::optional<operation> syrec_operation::invert(operation operation) noexcept {
    std::optional<syrec_operation::operation> mappedToInverseOperation;
    switch (operation) {
        case operation::AddAssign:
            mappedToInverseOperation.emplace(operation::MinusAssign);
            break;
        case operation::MinusAssign:
            mappedToInverseOperation.emplace(operation::AddAssign);
            break;
        case operation::XorAssign:
            mappedToInverseOperation.emplace(operation::XorAssign);
            break;
        case operation::IncrementAssign:
            mappedToInverseOperation.emplace(operation::DecrementAssign);
            break;
        case operation::DecrementAssign:
            mappedToInverseOperation.emplace(operation::IncrementAssign);
            break;
        case operation::InvertAssign:
            mappedToInverseOperation.emplace(operation::InvertAssign);
            break;

        case operation::Addition:
            mappedToInverseOperation.emplace(operation::Subtraction);
            break;
        case operation::Subtraction:
            mappedToInverseOperation.emplace(operation::Addition);
            break;
        case operation::BitwiseXor:
            mappedToInverseOperation.emplace(operation::BitwiseXor);
            break;
        case operation::Multiplication:
            mappedToInverseOperation.emplace(operation::Division);
            break;
        case operation::Division:
            mappedToInverseOperation.emplace(operation::Multiplication);
            break;
        case operation::LessThan:
            mappedToInverseOperation.emplace(operation::GreaterEquals);
            break;
        case operation::GreaterThan:
            mappedToInverseOperation.emplace(operation::LessEquals);
            break;
        case operation::LessEquals:
            mappedToInverseOperation.emplace(operation::GreaterThan);
            break;
        case operation::GreaterEquals:
            mappedToInverseOperation.emplace(operation::LessThan);
            break;
        case operation::Equals:
            mappedToInverseOperation.emplace(operation::NotEquals);
            break;
        case operation::NotEquals:
            mappedToInverseOperation.emplace(operation::Equals);
            break;
        case operation::ShiftLeft:
            mappedToInverseOperation.emplace(operation::ShiftRight);
            break;
        case operation::ShiftRight:
            mappedToInverseOperation.emplace(operation::ShiftLeft);
            break;
        default:
            break;
    }
    return mappedToInverseOperation;
}

std::optional<operation> syrec_operation::getMatchingAssignmentOperationForOperation(operation operation) noexcept {
    switch (operation) {
        case operation::Addition:
            return std::make_optional(operation::AddAssign);
        case operation::Subtraction:
            return std::make_optional(operation::MinusAssign);
        case operation::BitwiseXor:
            return std::make_optional(operation::XorAssign);
        default:
            return std::nullopt;
    }
}

 std::optional<operation> syrec_operation::tryMapBinaryOperationFlagToEnum(unsigned int binaryOperation) noexcept {
    switch (binaryOperation) {
        case syrec::BinaryExpression::Add:
            return std::make_optional(operation::Addition);
        case syrec::BinaryExpression::Subtract:
            return std::make_optional(operation::Subtraction);
        case syrec::BinaryExpression::Multiply:
            return std::make_optional(operation::Multiplication);
        case syrec::BinaryExpression::Divide:
            return std::make_optional(operation::Division);
        case syrec::BinaryExpression::Modulo:
            return std::make_optional(operation::Modulo);
        case syrec::BinaryExpression::FracDivide:
            return std::make_optional(operation::UpperBitsMultiplication);
        case syrec::BinaryExpression::Exor:
            return std::make_optional(operation::BitwiseXor);
        case syrec::BinaryExpression::LogicalAnd:
            return std::make_optional(operation::LogicalAnd);
        case syrec::BinaryExpression::LogicalOr:
            return std::make_optional(operation::LogicalOr);
        case syrec::BinaryExpression::BitwiseAnd:
            return std::make_optional(operation::BitwiseAnd);
        case syrec::BinaryExpression::BitwiseOr:
            return std::make_optional(operation::BitwiseOr);
        case syrec::BinaryExpression::LessThan:
            return std::make_optional(operation::LessThan);
        case syrec::BinaryExpression::GreaterThan:
            return std::make_optional(operation::GreaterThan);
        case syrec::BinaryExpression::Equals:
            return std::make_optional(operation::Equals);
        case syrec::BinaryExpression::NotEquals:
            return std::make_optional(operation::NotEquals);
        case syrec::BinaryExpression::LessEquals:
            return std::make_optional(operation::LessEquals);
        case syrec::BinaryExpression::GreaterEquals:
            return std::make_optional(operation::GreaterEquals);
        default:
            return std::nullopt;
    }
}

 std::optional<operation> syrec_operation::tryMapShiftOperationFlagToEnum(unsigned int shiftOperation) noexcept {
    switch (shiftOperation) {
        case syrec::ShiftExpression::Left:
            return std::make_optional(operation::ShiftLeft);
        case syrec::ShiftExpression::Right:
            return std::make_optional(operation::ShiftRight);
        default:
            return std::nullopt;
    }
}


 std::optional<operation> syrec_operation::tryMapAssignmentOperationFlagToEnum(unsigned int assignmentOperation) noexcept {
    switch (assignmentOperation) {
        case syrec::AssignStatement::Add:
            return std::make_optional(operation::AddAssign);
        case syrec::AssignStatement::Subtract:
            return std::make_optional(operation::MinusAssign);
        case syrec::AssignStatement::Exor:
            return std::make_optional(operation::XorAssign);
        default:
            return std::nullopt;
    }
}

std::optional<operation> syrec_operation::tryMapUnaryAssignmentOperationFlagToEnum(unsigned int unaryAssignmentOperation) noexcept {
    switch (unaryAssignmentOperation) {
        case syrec::UnaryStatement::Increment:
            return std::make_optional(operation::IncrementAssign);
        case syrec::UnaryStatement::Decrement:
            return std::make_optional(operation::DecrementAssign);
        case syrec::UnaryStatement::Invert:
            return std::make_optional(operation::InvertAssign);
        default:
            return std::nullopt;
    }
}

std::optional<unsigned> syrec_operation::tryMapBinaryOperationEnumToFlag(const operation binaryOperation) noexcept {
    switch (binaryOperation) {
        case operation::Addition:
            return std::make_optional(syrec::BinaryExpression::Add);
        case operation::Subtraction:
            return std::make_optional(syrec::BinaryExpression::Subtract);
        case operation::Multiplication:
            return std::make_optional(syrec::BinaryExpression::Multiply);
        case operation::Division:
            return std::make_optional(syrec::BinaryExpression::Divide);
        case operation::Modulo:
            return std::make_optional(syrec::BinaryExpression::Modulo);
        case operation::UpperBitsMultiplication:
            return std::make_optional(syrec::BinaryExpression::FracDivide);
        case operation::BitwiseXor:
            return std::make_optional(syrec::BinaryExpression::Exor);
        case operation::LogicalAnd:
            return std::make_optional(syrec::BinaryExpression::LogicalAnd);
        case operation::LogicalOr:
            return std::make_optional(syrec::BinaryExpression::LogicalOr);
        case operation::BitwiseAnd:
            return std::make_optional(syrec::BinaryExpression::BitwiseAnd);
        case operation::BitwiseOr:
            return std::make_optional(syrec::BinaryExpression::BitwiseOr);
        case operation::LessThan:
            return std::make_optional(syrec::BinaryExpression::LessThan);
        case operation::GreaterThan:
            return std::make_optional(syrec::BinaryExpression::GreaterThan);
        case operation::Equals:
            return std::make_optional(syrec::BinaryExpression::Equals);
        case operation::NotEquals:
            return std::make_optional(syrec::BinaryExpression::NotEquals);
        case operation::LessEquals:
            return std::make_optional(syrec::BinaryExpression::LessEquals);
        case operation::GreaterEquals:
            return std::make_optional(syrec::BinaryExpression::GreaterEquals);
        default:
            return std::nullopt;
    }
}

std::optional<unsigned> syrec_operation::tryMapShiftOperationEnumToFlag(const operation shiftOperation) noexcept {
    switch (shiftOperation) {
        case operation::ShiftLeft:
            return std::make_optional(syrec::ShiftExpression::Left);
        case operation::ShiftRight:
            return std::make_optional(syrec::ShiftExpression::Right);
        default:
            return std::nullopt;
    }
}

std::optional<unsigned int> syrec_operation::tryMapAssignmentOperationEnumToFlag(operation assignmentOperation) noexcept {
    switch (assignmentOperation) {
        case operation::AddAssign:
            return std::make_optional(syrec::AssignStatement::Add);
        case operation::MinusAssign:
            return std::make_optional(syrec::AssignStatement::Subtract);
        case operation::XorAssign:
            return std::make_optional(syrec::AssignStatement::Exor);
        case operation::IncrementAssign:
            return std::make_optional(syrec::UnaryStatement::Increment);
        case operation::DecrementAssign:
            return std::make_optional(syrec::UnaryStatement::Decrement);
        case operation::InvertAssign:
            return std::make_optional(syrec::UnaryStatement::Invert);
        default:
            return std::nullopt;
    }
}

std::optional<unsigned int> syrec_operation::tryMapUnaryAssignmentOperationEnumToFlag(operation unaryAssignmentOperation) noexcept {
    switch (unaryAssignmentOperation) {
        case operation::IncrementAssign:
            return std::make_optional(syrec::UnaryStatement::Increment);
        case operation::DecrementAssign:
            return std::make_optional(syrec::UnaryStatement::Decrement);
        case operation::InvertAssign:
            return std::make_optional(syrec::UnaryStatement::Invert);
        default:
            return std::nullopt;
    }
}