#include "core/syrec/parser/symbol_table.hpp"

#include <functional>

namespace syrec {
    bool                              symbol_table::contains(const std::string& literal) const {
        return this->locals.find(literal) != this->locals.end() || (nullptr != outer && outer->contains(literal));
    }

    bool                              symbol_table::contains(const Module::ptr& module) const {
        if (nullptr == module) {
            return false;
        }

        bool       found_module_matching_signature = false;
        const auto module_search_result = this->modules.find(module->name);
        if (module_search_result != this->modules.end()) {
            const std::vector<Module::ptr> modules_with_matching_name = module_search_result->second;

            // TODO: Could replace with std::find_if(...)
            for (size_t i = 0; i < modules_with_matching_name.size() && !found_module_matching_signature; i++) {
                found_module_matching_signature = do_module_signatures_match(modules_with_matching_name.at(i), module);
            }
        }

        return found_module_matching_signature || (nullptr != outer && outer->contains(module));
    }

    [[nodiscard]] std::optional<std::variant<Variable::ptr, Number::ptr>> symbol_table::get_variable(const std::string& literal) const {
        std::optional<std::variant<Variable::ptr, Number::ptr>> found_entry;

        if (this->locals.find(literal) != this->locals.end()) {
            found_entry.emplace(this->locals.find(literal)->second);
        }
        else if (nullptr != outer) {
            return outer->get_variable(literal);
        }
        return found_entry;
    }

    [[nodiscard]] std::optional<std::vector<Module::ptr>> symbol_table::get_matching_modules_for_name(const std::string& module_name) const {
        /*
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
        */
        std::optional<std::vector<Module::ptr>> found_modules;
        if (this->modules.find(module_name) != this->modules.end()) {
            found_modules.emplace(this->modules.find(module_name)->second);
        } else if (nullptr != outer) {
            found_modules = outer->get_matching_modules_for_name(module_name);
        }
        return found_modules;
    }

    bool                              symbol_table::add_entry(const Variable::ptr& local_entry) {
        if (nullptr == local_entry || contains(local_entry->name)) {
            return false;
        }

        const std::variant<Variable::ptr, Number::ptr> local_entry_value (local_entry);
        const std::pair new_local_entry(local_entry->name, local_entry_value);
        locals.insert(new_local_entry);
        return true;
    }

    bool symbol_table::add_entry(const Number::ptr& local_entry) {
        if (nullptr == local_entry || !local_entry->isLoopVariable() || contains(local_entry->variableName())) {
            return false;
        }

        const std::variant<Variable::ptr, Number::ptr> local_entry_value(local_entry);
        const std::pair                                new_local_entry(local_entry->variableName(), local_entry_value);
        locals.insert(new_local_entry);
        return true;
    }

    bool                              symbol_table::add_entry(const Module::ptr& module) {
        if (nullptr == module) {
            return false;
        }

        if (!contains(module)) {
            modules.insert(std::pair<std::string, std::vector<std::shared_ptr<Module>>>(module->name, std::vector<Module::ptr>()));
        }
        
        std::vector<Module::ptr> &modules_for_name = modules.find(module->name)->second;
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

    [[nodiscard]] bool symbol_table::do_module_signatures_match(const Module::ptr& module, const Module::ptr& other_module) {
        bool signatures_match = module->parameters.size() == other_module->parameters.size();
        for (size_t param_idx = 0; param_idx < module->parameters.size() && signatures_match; param_idx++) {
            const Variable::ptr &parameter_first_module = module->parameters.at(param_idx);
            const Variable::ptr &parameter_other_module = other_module->parameters.at(param_idx);
            signatures_match                             = parameter_first_module->name == parameter_other_module->name
                && can_assign_to(parameter_first_module, parameter_other_module, true);
        }
        return signatures_match;
    }
    
    [[nodiscard]] bool symbol_table::can_assign_to(const Variable::ptr& formal_parameter, const Variable::ptr& actual_parameter, const bool parameter_types_must_match) {
        return (parameter_types_must_match ? formal_parameter->type == actual_parameter->type : true) 
            && formal_parameter->dimensions == actual_parameter->dimensions
            && formal_parameter->bitwidth == actual_parameter->bitwidth;
    
    }
}
