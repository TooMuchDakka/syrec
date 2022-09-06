#include "core/syrec/parser/range_check.hpp"

namespace range_check {
    static std::optional<valid_index_access_range> get_valid_dimension_access_range(const syrec::variable::ptr& variable, const std::size_t dimension_idx, const bool zero_based_indexing) {
        std::optional<valid_index_access_range> valid_index_range;

        std::size_t max_valid_dimension_value;
        if (zero_based_indexing) {
            max_valid_dimension_value = variable->dimensions.empty() ? 0 : variable->dimensions.size() - 1;
        } else {
            max_valid_dimension_value = variable->dimensions.empty() ? 1 : variable->dimensions.size();
        }

        if (dimension_idx > max_valid_dimension_value) {
            // TODO: Throw exception
            return valid_index_range;
        }

        const std::size_t min_valid_dimension_value = zero_based_indexing ? 0 : 1;
        if (variable->dimensions.empty()) {
            max_valid_dimension_value = min_valid_dimension_value;
        }
        else {
            if (zero_based_indexing) {
                max_valid_dimension_value = variable->dimensions.at(dimension_idx) - 1;        
            } else {
                max_valid_dimension_value = variable->dimensions.at(dimension_idx);
            }
        }

        valid_index_range.emplace(valid_index_access_range(min_valid_dimension_value, max_valid_dimension_value));
        return valid_index_range;
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
        const std::optional<valid_index_access_range> valid_index_range_for_dimension = get_valid_dimension_access_range(variable, dimension_idx, zero_based_indexing).value();
        if (valid_index_range_for_dimension.has_value()) {
            return dimension_value >= valid_index_range_for_dimension.value().min_valid_value && dimension_value <= valid_index_range_for_dimension.value().max_valid_value;   
        }
        return false;
    }

    static bool is_valid_bit_access(const syrec::variable::ptr& variable, const std::size_t bit_access_index, const bool zero_based_indexing) {
        const valid_index_access_range valid_bit_access_range = get_valid_bit_access_range(variable, zero_based_indexing);
        return bit_access_index >= valid_bit_access_range.min_valid_value && bit_access_index <= valid_bit_access_range.max_valid_value;
    }

    static bool is_valid_range_access(const syrec::variable::ptr& variable, const std::pair<std::size_t, std::size_t>& range, const bool zero_based_indexing) {
        return is_valid_bit_access(variable, range.first, zero_based_indexing) && is_valid_bit_access(variable, range.second, zero_based_indexing);
    }
}