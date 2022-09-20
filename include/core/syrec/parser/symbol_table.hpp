#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <memory>

#include "core/syrec/module.hpp"
#include "core/syrec/variable.hpp"

namespace syrec {
    class SymbolTable {
    public:
        typedef std::shared_ptr<SymbolTable>  ptr;

        SymbolTable() {
            locals = {};
            modules = {};
            outer            = nullptr;
        }

        ~SymbolTable() = default;

        bool contains(const std:: string_view& literalIdent) const;
        bool                              contains(const Module::ptr& module) const;
        [[nodiscard]] std::optional<std::variant<Variable::ptr, Number::ptr>> getVariable(const std::string_view& literalIdent) const;
        [[nodiscard]] std::optional<Module::vec>   getMatchingModulesForName(const std::string_view& moduleName) const;
        bool                              addEntry(const Variable::ptr& variable);
        bool                              addEntry(const Number::ptr& number);
        bool                              addEntry(const Module::ptr& module);                     

        static void openScope(SymbolTable::ptr &currentScope);
        static void closeScope(SymbolTable::ptr& currentScope);
        static bool isAssignableTo(const Variable::ptr& formalParameter, const Variable::ptr& actualParameter, bool mustParameterTypesMatch);

    private:
        std::map<std::string, std::variant<Variable::ptr, Number::ptr>, std::less<void>> locals;
        std::map<std::string, Module::vec, std::less<void>> modules; 
        SymbolTable::ptr outer;

        static bool doModuleSignaturesMatch(const Module::ptr& module, const Module::ptr& otherModule);
    };    
}

#endif