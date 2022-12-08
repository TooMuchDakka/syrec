#ifndef OPERATION_HPP
#define OPERATION_HPP
#include <optional>

namespace syrec_operation {
    enum class operation {
        // Binary expression operations
        // BEGIN: Number operations
        addition,
        subtraction,
        multiplication,
        division,
        // END: Number operations
        modulo,
        upper_bits_multiplication,
        bitwise_xor,
        logical_and,
        logical_or,
        bitwise_and,
        bitwise_or,
        less_than,
        greater_than,
        equals,
        not_equals,
        less_equals,
        greater_equals,
        // Unary statement operations
        increment_assign,
        decrement_assign,
        invert_assign,
        //Assign statement operations
        add_assign,
        minus_assign,
        xor_assign,
        // Unary expression operations
        bitwise_negation,
        logical_negation,
        // Shift expression operations
        shift_left,
        shift_right
    };

    [[nodiscard]] std::optional<unsigned int> apply(operation operation, unsigned int leftOperand, unsigned int rightOperand) noexcept;
    [[nodiscard]] std::optional<unsigned int> apply(operation operation, unsigned int operand) noexcept;
};

#endif