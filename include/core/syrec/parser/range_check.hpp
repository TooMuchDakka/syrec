#ifndef RANGE_CHECK_HPP
#define RANGE_CHECK_HPP

#include "../variable.hpp"

namespace range_check {
    struct valid_index_access_range {
        std::size_t min_valid_value;
        std::size_t max_valid_value;

        explicit valid_index_access_range(const std::size_t min_valid_value, const std::size_t max_valid_value):
            min_valid_value(min_valid_value), max_valid_value(max_valid_value)
        {
        }
    };

    [[nodiscard]] bool is_valid_dimension_access(const syrec::variable::ptr & variable, std::size_t dimension_idx, std::size_t dimension_value, bool zero_based_indexing = true);
    [[nodiscard]] bool is_valid_bit_access(const syrec::variable::ptr& variable, std::size_t bit_access_index, bool zero_based_indexing = true);
    [[nodiscard]] bool is_valid_range_access(const syrec::variable::ptr& variable, std::size_t range_start, std::size_t range_end, bool zero_based_indexing = true);

    [[nodiscard]] valid_index_access_range get_valid_dimension_access_range(const syrec::variable::ptr& variable, bool zero_based_indexing = true);
    [[nodiscard]] valid_index_access_range get_valid_range_access_range(const syrec::variable::ptr& variable, bool zero_based_indexing = true);
    [[nodiscard]] valid_index_access_range get_valid_bit_access_range(const syrec::variable::ptr& variable, bool zero_based_indexing = true);

}

#endif