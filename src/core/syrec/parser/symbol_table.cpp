#include "core/syrec/parser/symbol_table.hpp"

#include <functional>

namespace syrec {
    bool                              symbol_table::contains(const std::string& literal) {
        return this->locals.find(literal) != this->locals.end();
    }

    bool                              symbol_table::contains(const module::ptr& module) {
        if (nullptr == module) {
            return false;
        }

        bool       found_module_matching_signature = false;
        const auto module_search_result = this->modules.find(module->name);
        if (module_search_result != this->modules.end()) {
            const std::vector<module::ptr> modules_with_matching_name = module_search_result->second;

            // TODO: Could replace with std::find_if(...)
            for (size_t i = 0; i < modules_with_matching_name.size() && !found_module_matching_signature; i++) {
                found_module_matching_signature = do_module_signatures_match(modules_with_matching_name.at(i), module);
            }
        }

        return found_module_matching_signature;
    }

    [[nodiscard]] variable::ptr symbol_table::get_variable(const std::string& literal) {

        if (contains(literal)) {
            return this->locals.find(literal)->second;
        } else if (nullptr != outer) {
            return outer->get_variable(literal);
        } else {
            return nullptr;
        }
    }

    void                              symbol_table::add_entry(const variable::ptr& local_entry) {
        // TODO: Implement me
    }

    void                              symbol_table::add_entry(const module::ptr& module) {
        // TODO: Implement me
    }

    void symbol_table::open_scope(symbol_table::ptr &current_scope) {
        // TODO: Implement me
        const symbol_table::ptr new_scope(new symbol_table());
        new_scope->outer = current_scope;
        current_scope    = new_scope;
    }

    void symbol_table::close_scope(symbol_table::ptr &current_scope) {
        const symbol_table::ptr &outer_scope = current_scope->outer;
        if (nullptr != outer_scope) {
            current_scope->outer = nullptr;
        }
        current_scope = outer_scope;
    }

    [[nodiscard]] bool symbol_table::do_module_signatures_match(const module::ptr& module, const module::ptr& other_module) {
        bool signatures_match = module->parameters.size() == other_module->parameters.size();
        for (size_t param_idx = 0; param_idx < module->parameters.size() && signatures_match; param_idx++) {
            const variable::ptr &parameter_first_module = module->parameters.at(param_idx);
            const variable::ptr &parameter_other_module = other_module->parameters.at(param_idx);
            signatures_match                             = are_parameters_equal(parameter_first_module, parameter_other_module);
        }
        return signatures_match;
    }
    
    [[nodiscard]] bool symbol_table::are_parameters_equal(const variable::ptr& parameter_one, const variable::ptr& parameter_two) {
        return parameter_one->type == parameter_two->type
            && parameter_one->name == parameter_two->name
            && parameter_one->dimensions == parameter_two->dimensions
            && parameter_one->bitwidth == parameter_two->bitwidth;
    
    }
}
