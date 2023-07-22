#include "core/syrec/parser/operation.hpp"

using namespace syrec_operation;

bool syrec_operation::isOperandUsedAsLhsInOperationIdentityElement(operation operation, unsigned operand) noexcept {
    if (isCommutative(operation)) {
        return isOperandUseAsRhsInOperationIdentityElement(operation, operand);
    }

    switch (operation) {
        case operation::addition:
        case operation::shift_left:
        case operation::shift_right:
            return operand == 0;
        case operation::multiplication:
            return operand == 1;
        default:
            return false;
    }
}

bool syrec_operation::isOperandUseAsRhsInOperationIdentityElement(const operation operation, const unsigned int operand) noexcept {
    switch (operation) {
        case operation::addition:
        case operation::add_assign:
        case operation::subtraction:
        case operation::minus_assign:
        case operation::shift_left:
        case operation::shift_right:
            return operand == 0;
        case operation::division:
        case operation::multiplication:
        case operation::modulo:
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
        case operation::addition:
        case operation::add_assign:
            result = leftOperand + rightOperand;
            break;
        case operation::subtraction:
        case operation::minus_assign:
            result = leftOperand - rightOperand;
            break;
        case operation::multiplication:
            // TODO: Implement correctly
        case operation::upper_bits_multiplication:
            result = leftOperand * rightOperand;
            break;
        case operation::division: {
            if (rightOperand == 0) {
                isValidBinaryOperation = false;
            } else {
                result = leftOperand / rightOperand;
            }
            break;   
        }
        case operation::bitwise_and:
            result = leftOperand & rightOperand;
            break;
        case operation::bitwise_or:
            result = leftOperand | rightOperand;
            break;
        case operation::bitwise_xor:
        case operation::xor_assign:
            result = leftOperand ^ rightOperand;
            break;
        case operation::modulo:
            result = leftOperand % rightOperand;
            break;
        case operation::logical_and:
            result = leftOperand && rightOperand;
            break;
        case operation::logical_or:
            result = leftOperand || rightOperand;
            break;
        case operation::less_than:
            result = leftOperand < rightOperand;
            break;
        case operation::greater_than:
            result = leftOperand > rightOperand;
            break;
        case operation::equals:
            result = leftOperand == rightOperand;
            break;
        case operation::not_equals:
            result = leftOperand != rightOperand;
            break;
        case operation::less_equals:
            result = leftOperand <= rightOperand;
            break;
        case operation::greater_equals:
            result = leftOperand >= rightOperand;
            break;
        case operation::shift_left:
            result = leftOperand << rightOperand;
            break;
        case operation::shift_right:
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
        case operation::bitwise_negation:
            result = ~operand;
            break;
        case operation::logical_negation:
            result = operand == 0 ? 1 : 0;
            break;
        case operation::increment_assign:
            result = operand + 1;
            break;
        case operation::decrement_assign:
            result = operand - 1;
            break;
        case operation::invert_assign:
            result = ~operand;
            break;
        default:
            isValidUnaryOperation = false;
    }

    return isValidUnaryOperation ? std::optional(result) : std::nullopt;
}

bool syrec_operation::isCommutative(operation operation) noexcept {
    switch (operation) {
        case operation::addition:
        case operation::multiplication:
        case operation::upper_bits_multiplication:
        case operation::bitwise_and:
        case operation::bitwise_or:
        case operation::bitwise_xor:
        case operation::logical_and:
        case operation::logical_or:
        case operation::equals:
        case operation::not_equals:
            return true;
        default:
            return false;
    }
}

std::optional<operation> syrec_operation::invert(operation operation) noexcept {
    std::optional<syrec_operation::operation> mappedToInverseOperation;
    switch (operation) {
        case operation::add_assign:
            mappedToInverseOperation.emplace(operation::minus_assign);
            break;
        case operation::minus_assign:
            mappedToInverseOperation.emplace(operation::add_assign);
            break;
        case operation::xor_assign:
            mappedToInverseOperation.emplace(operation::xor_assign);
            break;
        case operation::increment_assign:
            mappedToInverseOperation.emplace(operation::decrement_assign);
            break;
        case operation::decrement_assign:
            mappedToInverseOperation.emplace(operation::increment_assign);
            break;
        case operation::invert_assign:
            mappedToInverseOperation.emplace(operation::invert_assign);
            break;

        case operation::addition:
            mappedToInverseOperation.emplace(operation::subtraction);
            break;
        case operation::subtraction:
            mappedToInverseOperation.emplace(operation::addition);
            break;
        case operation::bitwise_xor:
            mappedToInverseOperation.emplace(operation::bitwise_xor);
            break;
        case operation::multiplication:
            mappedToInverseOperation.emplace(operation::division);
            break;
        case operation::division:
            mappedToInverseOperation.emplace(operation::multiplication);
            break;
        case operation::less_than:
            mappedToInverseOperation.emplace(operation::greater_equals);
            break;
        case operation::greater_than:
            mappedToInverseOperation.emplace(operation::less_equals);
            break;
        case operation::less_equals:
            mappedToInverseOperation.emplace(operation::greater_than);
            break;
        case operation::greater_equals:
            mappedToInverseOperation.emplace(operation::less_than);
            break;
        case operation::equals:
            mappedToInverseOperation.emplace(operation::not_equals);
            break;
        case operation::not_equals:
            mappedToInverseOperation.emplace(operation::equals);
            break;
        case operation::shift_left:
            mappedToInverseOperation.emplace(operation::shift_right);
            break;
        case operation::shift_right:
            mappedToInverseOperation.emplace(operation::shift_left);
            break;
        default:
            break;
    }
    return mappedToInverseOperation;
}

std::optional<operation> syrec_operation::getMatchingAssignmentOperationForOperation(operation operation) noexcept {
    switch (operation) {
        case operation::addition:
            return std::make_optional(operation::add_assign);
        case operation::subtraction:
            return std::make_optional(operation::minus_assign);
        case operation::bitwise_xor:
            return std::make_optional(operation::xor_assign);
        default:
            return std::nullopt;
    }
}
