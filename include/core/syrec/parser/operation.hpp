#ifndef OPERATION_HPP
#define OPERATION_HPP

namespace syrec_operation {
    enum class operation {
        // Number operations
        addition,
        subtraction,
        multiplication,
        division ,
        // Unary statement operations
        increment_assign,
        decrement_assign,
        negate_assign,
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

    /**
     *  TODO: Add documentation
     * @returns
     * @throws std::overflow_error
     * @throws std::invalid_argument
     **/
    [[nodiscard]] unsigned int                apply(operation operation, unsigned int left_operand_value, unsigned int right_operand_value);

    /**
     * TODO: Add documentation
     * @returns
     * @throws std::overflow_error
     * @throws std::invalid_argument
     **/
    [[nodiscard]] unsigned int apply(operation operation, unsigned int left_operand_value);
};

#endif