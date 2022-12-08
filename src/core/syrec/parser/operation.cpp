#include "core/syrec/parser/operation.hpp"

namespace syrec_operation {

    static std::optional<unsigned int> apply(const operation operation, const unsigned int leftOperand, const unsigned int rightOperand) noexcept
    {
        unsigned int result;
        bool         isValidBinaryOperation = true;
        switch (operation) {
            case syrec_operation::operation::addition:
            case syrec_operation::operation::add_assign:
                result = leftOperand + rightOperand;
                break;
            case syrec_operation::operation::subtraction:
            case syrec_operation::operation::minus_assign:
                result = leftOperand - rightOperand;
                break;
            case syrec_operation::operation::multiplication:
                // TODO: Implement correctly
            case syrec_operation::operation::upper_bits_multiplication:
                result = leftOperand * rightOperand;
                break;
            case syrec_operation::operation::division:
                if (rightOperand == 0) {
                    isValidBinaryOperation = false;
                }
                else {
                    result = leftOperand / rightOperand;
                }
                break;
            case syrec_operation::operation::bitwise_and:
                result = leftOperand & rightOperand;
                break;
            case syrec_operation::operation::bitwise_or:
                result = leftOperand | rightOperand;
                break;
            case syrec_operation::operation::bitwise_xor:
            case syrec_operation::operation::xor_assign:
                result = leftOperand ^ rightOperand;
                break;
            case syrec_operation::operation::modulo:
                result = leftOperand % rightOperand;
                break;
            // TODO: Implement correctly
            case syrec_operation::operation::logical_and:
                result = leftOperand && rightOperand;
                break;
            // TODO: Implement correctly
            case syrec_operation::operation::logical_or:
                result = leftOperand || rightOperand;
                break;
            case syrec_operation::operation::less_than:
                result = leftOperand < rightOperand;
                break;
            case syrec_operation::operation::greater_than:
                result = leftOperand > rightOperand;
                break;
            case syrec_operation::operation::equals:
                result = leftOperand == rightOperand;
                break;
            case syrec_operation::operation::not_equals:
                result = leftOperand != rightOperand;
                break;
            case syrec_operation::operation::less_equals:
                result = leftOperand <= rightOperand;
                break;
            case syrec_operation::operation::greater_equals:
                result = leftOperand >= rightOperand;
                break;
            case syrec_operation::operation::shift_left:
                result = leftOperand << rightOperand;
                break;
            case syrec_operation::operation::shift_right:
                result = leftOperand >> rightOperand;
                break;
            default:
                isValidBinaryOperation = false;
        }

        return isValidBinaryOperation ? std::optional(result) : std::nullopt;
    }

    static std::optional<unsigned int> apply(const operation operation, const unsigned int operand) noexcept {
        unsigned int result = 0;
        bool isValidUnaryOperation = true;

        switch (operation) {
            case syrec_operation::operation::bitwise_negation:
                result = ~operand;
                break;
            // TODO: Implement correctly
            case syrec_operation::operation::logical_negation:
                break;
            case syrec_operation::operation::increment_assign:
                result = operand + 1;
                break;
            case syrec_operation::operation::decrement_assign:
                result = operand - 1;
                break;
            // TODO: Implement correctly
            case syrec_operation::operation::invert_assign:
                result = ~operand;
                break;
            default:
                isValidUnaryOperation = false;
        }

        return isValidUnaryOperation ? std::optional(result) : std::nullopt;
    }
}
