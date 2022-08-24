#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <memory>

#include "core/syrec/module.hpp"
#include "core/syrec/variable.hpp"

namespace syrec {
    class symbol_table {
    public:
        symbol_table() {
            declared_modules = {};
            outer            = nullptr;
        }

        ~symbol_table() = default;

        bool                              contains(const std::string_view &literal);
        [[nodiscard]] const variable::ptr get_variable(const std::string_view &literal);
        [[nodiscard]] const module::ptr   get_module(const std::string_view &moduleName);

        void open_scope();
        void close_scope();

    private:
        std::map<std::string, module::ptr> declared_modules;
        std::shared_ptr<symbol_table>      outer;
    };    
}

#endif