#include "core/syrec/parser/operation.hpp"

#include <stdexcept>

namespace syrec_operation {

    static unsigned int apply(const operation operation, const unsigned int left_operand_value, const unsigned int right_operand_value)
    {
        unsigned int result;
        switch (operation) {
            case syrec_operation::operation::addition:
            case syrec_operation::operation::add_assign:
                result = left_operand_value + right_operand_value;
                break;
            case syrec_operation::operation::subtraction:
            case syrec_operation::operation::minus_assign:
                result = left_operand_value - right_operand_value;
                break;
            case syrec_operation::operation::multiplication:
                // TODO: Implement correctly
            case syrec_operation::operation::upper_bits_multiplication:
                result = left_operand_value * right_operand_value;
                break;
            case syrec_operation::operation::division:
                result = right_operand_value != 0 ? (left_operand_value / right_operand_value) : 0;
                break;
            case syrec_operation::operation::bitwise_and:
                result = left_operand_value & right_operand_value;
                break;
            case syrec_operation::operation::bitwise_or:
                result = left_operand_value | right_operand_value;
                break;
            case syrec_operation::operation::bitwise_xor:
            case syrec_operation::operation::xor_assign:
                result = left_operand_value ^ right_operand_value;
                break;
            case syrec_operation::operation::modulo:
                result = left_operand_value % right_operand_value;
                break;
            // TODO: Implement correctly
            case syrec_operation::operation::logical_and:
                result = left_operand_value && right_operand_value;
                break;
            // TODO: Implement correctly
            case syrec_operation::operation::logical_or:
                result = left_operand_value || right_operand_value;
                break;
            case syrec_operation::operation::less_than:
                result = left_operand_value < right_operand_value;
                break;
            case syrec_operation::operation::greater_than:
                result = left_operand_value > right_operand_value;
                break;
            case syrec_operation::operation::equals:
                result = left_operand_value == right_operand_value;
                break;
            case syrec_operation::operation::not_equals:
                result = left_operand_value != right_operand_value;
                break;
            case syrec_operation::operation::less_equals:
                result = left_operand_value <= right_operand_value;
                break;
            case syrec_operation::operation::greater_equals:
                result = left_operand_value >= right_operand_value;
                break;
            case syrec_operation::operation::shift_left:
                result = left_operand_value << right_operand_value;
                break;
            case syrec_operation::operation::shift_right:
                result = left_operand_value >> right_operand_value;
                break;
            default:
                // TODO:
                throw std::invalid_argument("TODO:");
        }
        return result;
    }

    static unsigned int         apply(const operation operation, const unsigned int left_operand_value) {
        unsigned int result = 0;
        switch (operation) {
            case syrec_operation::operation::bitwise_negation:
                result = ~left_operand_value;
                break;
            // TODO: Implement correctly
            case syrec_operation::operation::logical_negation:
                break;
            case syrec_operation::operation::increment_assign:
                result = left_operand_value + 1;
                break;
            case syrec_operation::operation::decrement_assign:
                result = left_operand_value - 1;
                break;
            // TODO: Implement correctly
            case syrec_operation::operation::negate_assign:
                result = ~left_operand_value;
                break;

            default:
                // TODO:
                throw std::invalid_argument("TODO:");
        }
        return result;
    }
}
