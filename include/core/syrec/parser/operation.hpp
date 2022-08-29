#ifndef OPERATION_HPP
#define OPERATION_HPP

namespace syrec_operation {
    enum class operation {
        addition,
        subtraction,
        multiplication,
        division ,

        increment_assign,
        decrement_assign,
        negate_assign,

        add_assign,
        minus_assign,
        xor_assign
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