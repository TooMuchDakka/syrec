#include "core/syrec/parser/method_call_guess.hpp"

namespace syrec {
    // TODO: When refining the guess sort the options according to the number of formal parameters ascendingly
    bool method_call_guess::refine(const std::string& parameter) {
        bool could_refine_guess = false;

        if (!methods_matching_signature.empty() && this->symbol_table_scope->contains(parameter)) {
            const auto parameter_symbol_table_entry = this->symbol_table_scope->get_variable(parameter).value();
            if (std::holds_alternative<variable::ptr>(parameter_symbol_table_entry)) {
                const variable::ptr parameter_entry = std::get<variable::ptr>(parameter_symbol_table_entry);

                discard_guesses_with_less_parameters(this->formal_parameter_idx + 1);
                discard_not_matching_alternatives(parameter_entry);
                could_refine_guess = matches_some_options();
                this->formal_parameter_idx++;
            }            
        } else {
            if (!methods_matching_signature.empty()) {
                methods_matching_signature.clear();
            }
        }
        return could_refine_guess;
    }

    bool method_call_guess::matches_some_options() const {
        return !methods_matching_signature.empty();
    }

    void method_call_guess::discard_guesses_with_less_parameters(const std::size_t &num_formal_parameters) {
        methods_matching_signature.erase(
                std::remove_if(
                    methods_matching_signature.begin(),
                    methods_matching_signature.end(),
                        [num_formal_parameters](const module::ptr& module) { return module->parameters.size() < num_formal_parameters; }
                ),
                methods_matching_signature.end());
    }

    void method_call_guess::discard_not_matching_alternatives(const variable::ptr& actual_parameter) {
        const std::size_t formal_parameter_idx = this->formal_parameter_idx;
        methods_matching_signature.erase(
                std::remove_if(
                        methods_matching_signature.begin(),
                        methods_matching_signature.end(),
                        [formal_parameter_idx, actual_parameter](const module::ptr& module) {
                            const variable::ptr &formal_parameter = module->parameters.at(formal_parameter_idx);
                            return !symbol_table::can_assign_to(formal_parameter, actual_parameter, false);
                        }),
                methods_matching_signature.end());
    }

    std::optional<module::ptr> method_call_guess::get_remaining_guess() const {
        std::optional<module::ptr> matching_guess;
        if (methods_matching_signature.size() == 1) {
            matching_guess.emplace(methods_matching_signature.at(0));
        }
        return matching_guess;
    }


}