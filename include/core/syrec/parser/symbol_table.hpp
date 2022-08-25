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

        bool                              contains(const std::string &literal);
        bool                              contains(const module::ptr& module);
        [[nodiscard]] variable::ptr get_variable(const std::string &literal);
        void                              add_entry(const variable::ptr& local_entry);
        void                              add_entry(const module::ptr& module);                     

        static void open_scope(symbol_table::ptr &current_scope);
        static void close_scope(symbol_table::ptr &current_scope);

    private:
        std::map<std::string, variable::ptr> locals;
        std::map<std::string, std::vector<module::ptr>> modules; 
        symbol_table::ptr      outer;

        static bool do_module_signatures_match(const module::ptr& module, const module::ptr& other_module);
        static bool are_parameters_equal(const variable::ptr& parameter_one, const variable::ptr& parameter_two);
    };    
}

#endif