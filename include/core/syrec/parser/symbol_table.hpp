#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <memory>

#include "core/syrec/module.hpp"
#include "core/syrec/variable.hpp"

namespace syrec {
    class symbol_table {
    public:

        typedef std::shared_ptr<symbol_table> ptr;

        symbol_table() {
            locals = {};
            modules = {};
            outer            = nullptr;
        }

        ~symbol_table() = default;

        bool                              contains(const std::string &literal) const;
        bool                              contains(const Module::ptr& module) const;
        [[nodiscard]] std::optional<std::variant<Variable::ptr, Number::ptr>> get_variable(const std::string &literal) const;
        [[nodiscard]] std::optional<std::vector<Module::ptr>>   get_matching_modules_for_name(const std::string& module_name) const;
        bool                              add_entry(const Variable::ptr& local_entry);
        bool                              add_entry(const Number::ptr& local_entry);
        bool                              add_entry(const Module::ptr& module);                     

        static void open_scope(symbol_table::ptr &current_scope);
        static void close_scope(symbol_table::ptr& current_scope);
        static bool can_assign_to(const Variable::ptr& formal_parameter, const Variable::ptr& actual_parameter, bool parameter_types_must_match);

    private:
        std::map<std::string, std::variant<Variable::ptr, Number::ptr>> locals;
        std::map<std::string, std::vector<Module::ptr>> modules; 
        symbol_table::ptr      outer;

        static bool do_module_signatures_match(const Module::ptr& module, const Module::ptr& other_module);
    };    
}

#endif