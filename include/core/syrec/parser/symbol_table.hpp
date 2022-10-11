#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <memory>

#include "core/syrec/module.hpp"
#include "core/syrec/variable.hpp"

namespace parser {
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
        bool                              contains(const syrec::Module::ptr& module) const;
        [[nodiscard]] std::optional<std::variant<syrec::Variable::ptr, syrec::Number::ptr>> getVariable(const std::string_view& literalIdent) const;
        [[nodiscard]] std::optional<syrec::Module::vec>                              getMatchingModulesForName(const std::string_view& moduleName) const;
        bool                                                                         addEntry(const syrec::Variable::ptr& variable);
        bool                                                                         addEntry(const syrec::Number::ptr& number);
        bool                                                                         addEntry(const syrec::Module::ptr& module);                     

        static void openScope(SymbolTable::ptr& currentScope);
        static void closeScope(SymbolTable::ptr& currentScope);
        static bool isAssignableTo(const syrec::Variable::ptr& formalParameter, const syrec::Variable::ptr& actualParameter, bool mustParameterTypesMatch);

    private:
        std::map<std::string, std::variant<syrec::Variable::ptr, syrec::Number::ptr>, std::less<void>> locals;
        std::map<std::string, syrec::Module::vec, std::less<void>>                                     modules; 
        SymbolTable::ptr outer;

        static bool doModuleSignaturesMatch(const syrec::Module::ptr& module, const syrec::Module::ptr& otherModule);
    };    
}

#endif