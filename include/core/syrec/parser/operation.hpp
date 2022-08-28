#ifndef OPERATION_HPP
#define OPERATION_HPP

#include "../number.hpp"

#include <optional>

namespace syrec {
    class syrec_operation {
    public:
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

        [[nodiscard]] static std::optional<unsigned int>                apply(operation operation, const number::ptr &left_operand, const number::ptr &right_operand);
        [[nodiscard]] static std::optional<unsigned int> apply(operation operation, const number::ptr &left_operand);
	};
}

#endif