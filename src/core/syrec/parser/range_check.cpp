#include "core/syrec/parser/range_check.hpp"

namespace range_check {
    static valid_index_access_range get_valid_dimension_access_range(const syrec::variable::ptr& variable, const bool zero_based_indexing) {
        std::size_t min_valid_dimension_value, max_valid_dimension_value;
        if (zero_based_indexing) {
            min_valid_dimension_value = 0;
            max_valid_dimension_value = variable->dimensions.size() - 1;
        } else {
            min_valid_dimension_value = 1;
            max_valid_dimension_value = variable->dimensions.size();
        }
        return valid_index_access_range(min_valid_dimension_value, max_valid_dimension_value);
    }

    static valid_index_access_range get_valid_range_access_range(const syrec::variable::ptr& variable, const bool zero_based_indexing) {
        return get_valid_bit_access_range(variable, zero_based_indexing);
    }

    static valid_index_access_range get_valid_bit_access_range(const syrec::variable::ptr& variable, const bool zero_based_indexing) {
        std::size_t min_valid_bit_index, max_valid_bit_index;
        if (zero_based_indexing) {
            min_valid_bit_index = 0;
            max_valid_bit_index = variable->bitwidth - 1;
        } else {
            min_valid_bit_index = 1;
            max_valid_bit_index = variable->bitwidth;
        }
        return valid_index_access_range(min_valid_bit_index, max_valid_bit_index);
    }

    static bool is_valid_dimension_access(const syrec::variable::ptr& variable, const std::size_t dimension_idx, const std::size_t dimension_value, const bool zero_based_indexing) {
        if (dimension_idx > variable->dimensions.size()) {
            return false;
        }

        const valid_index_access_range valid_dimension_value_range = get_valid_dimension_access_range(variable, zero_based_indexing);
        return dimension_value >= valid_dimension_value_range.min_valid_value && dimension_value <= valid_dimension_value_range.max_valid_value;
    }

    static bool is_valid_bit_access(const syrec::variable::ptr& variable, const std::size_t bit_access_index, const bool zero_based_indexing) {
        const valid_index_access_range valid_bit_access_range = get_valid_bit_access_range(variable, zero_based_indexing);
        return bit_access_index >= valid_bit_access_range.min_valid_value && bit_access_index <= valid_bit_access_range.max_valid_value;
    }

    static bool is_valid_range_access(const syrec::variable::ptr& variable, const std::size_t range_start, const std::size_t range_end, const bool zero_based_indexing) {
        return is_valid_bit_access(variable, range_start, zero_based_indexing) && is_valid_bit_access(variable, range_end, zero_based_indexing);
    }
}