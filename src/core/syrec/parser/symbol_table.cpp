#include "core/syrec/parser/symbol_table.hpp"

#include <functional>

namespace syrec {
    bool                              symbol_table::contains(const std::string& literal) const {
        return this->locals.find(literal) != this->locals.end() || (nullptr != outer && outer->contains(literal));
    }

    bool                              symbol_table::contains(const module::ptr& module) const {
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

        return found_module_matching_signature || (nullptr != outer && outer->contains(module));
    }

    [[nodiscard]] std::optional<variable::ptr> symbol_table::get_variable(const std::string& literal) const {
        std::optional<variable::ptr> found_entry;

        if (contains(literal)) {
            found_entry.emplace(this->locals.find(literal)->second);
        }
        else if (nullptr != outer) {
            return outer->get_variable(literal);
        }
        return found_entry;
    }

    [[nodiscard]] std::optional<module::ptr> symbol_table::get_module(const std::string& module_name, const std::size_t number_of_user_supplied_arguments) const {
        std::optional<module::ptr> found_module;

        const auto module_search_result = this->modules.find(module_name);
        if (this->modules.find(module_name) != this->modules.end()) {
            const std::vector<module::ptr> modules_with_matching_name = module_search_result->second;
            bool                           found_module_matching_signature = false;

            for (size_t i = 0; i < modules_with_matching_name.size() && !found_module_matching_signature; i++) {
                if (modules_with_matching_name.at(i)->parameters.size() == number_of_user_supplied_arguments) {
                    found_module_matching_signature = true;
                    found_module.emplace((modules_with_matching_name.at(i)));
                }    
            }
            return found_module;
        } else if (nullptr != outer) {
            return outer->get_module(module_name, number_of_user_supplied_arguments);
        } else {
            return found_module;
        }
    }

    bool                              symbol_table::add_entry(const variable::ptr& local_entry) {
        if (nullptr == local_entry || contains(local_entry->name)) {
            return false;
        }

        const std::pair new_local_entry(local_entry->name, local_entry);
        locals.insert(new_local_entry);
        return true;
    }

    bool                              symbol_table::add_entry(const module::ptr& module) {
        if (nullptr == module) {
            return false;
        }

        if (!contains(module)) {
            modules.insert(std::pair<std::string, std::vector<std::shared_ptr<syrec::module>>>(module->name, std::vector<module::ptr>()));
        }
        
        std::vector<module::ptr> &modules_for_name = modules.find(module->name)->second;
        modules_for_name.emplace_back(module);
        return true;
    }

    void symbol_table::open_scope(symbol_table::ptr &current_scope) {
        // TODO: Implement me
        const symbol_table::ptr new_scope(new symbol_table());
        new_scope->outer = current_scope;
        current_scope    = new_scope;
    }

    void symbol_table::close_scope(symbol_table::ptr &current_scope) {
        if (nullptr != current_scope) {
            current_scope = current_scope->outer;
        } 
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
