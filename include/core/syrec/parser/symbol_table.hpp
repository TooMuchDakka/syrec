#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <memory>

#include "core/syrec/module.hpp"
#include "core/syrec/variable.hpp"

namespace syrec {
    class symbol_table {
    public:
        symbol_table() {
            locals = {};
            outer            = nullptr;
        }

        ~symbol_table() = default;

        bool                              contains(const std::string_view &literal);
        bool                              contains(const module::ptr& module);
        [[nodiscard]] const variable::ptr get_variable(const std::string_view &literal);
        [[nodiscard]] const module::ptr   get_module(const std::string_view &moduleName);
        void                              add_entry(const variable::ptr& local_entry);
        void                              add_entry(const module::ptr& module);                     

        void open_scope();
        void close_scope();

    private:
        std::map<std::string, variable::ptr> locals;
        std::shared_ptr<symbol_table>      outer;
    };    
}

#endif