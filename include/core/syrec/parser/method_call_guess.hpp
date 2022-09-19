#ifndef METHOD_CALL_GUESS_HPP
#define METHOD_CALL_GUESS_HPP

#include <optional>
#include <string>
#include <vector>

#include "core/syrec/module.hpp"
#include "core/syrec/parser/symbol_table.hpp"


namespace syrec {
    class method_call_guess {
    public:
        typedef std::shared_ptr<method_call_guess> ptr;

        explicit method_call_guess(const symbol_table::ptr& symbol_table_scope, const std::string &method_ident):
            methods_matching_signature({}), symbol_table_scope(symbol_table_scope) {
            const std::optional<std::vector<Module::ptr>> &matching_modules = this->symbol_table_scope->get_matching_modules_for_name(method_ident);
            if (matching_modules.has_value()) {
                methods_matching_signature = matching_modules.value();
            }
            formal_parameter_idx = 0;
        }

        /**
         * \brief 
         * \param parameter 
         * \return
         * \throws std::invalid_argument
         */
        bool refine(const std::string& parameter);
        bool matches_some_options() const;
        [[nodiscard]] std::optional<Module::ptr> get_remaining_guess() const;

    private:
        std::vector<Module::ptr> methods_matching_signature;
        const symbol_table::ptr& symbol_table_scope;
        std::size_t              formal_parameter_idx;

        void discard_guesses_with_less_parameters(const std::size_t &num_formal_parameters);
        void discard_not_matching_alternatives(const Variable::ptr& actual_parameter);
    };
}

#endif