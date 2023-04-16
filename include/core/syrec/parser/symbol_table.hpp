#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <memory>
#include <set>

#include "core/syrec/module.hpp"
#include "core/syrec/variable.hpp"
#include "optimizations/constantPropagation/valueLookup/signal_value_lookup.hpp"

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

        bool                                                                                contains(const std::string_view& literalIdent) const;
        bool                                                                                contains(const syrec::Module::ptr& module) const;
        [[nodiscard]] std::optional<std::variant<syrec::Variable::ptr, syrec::Number::ptr>> getVariable(const std::string_view& literalIdent) const;
        [[nodiscard]] std::optional<syrec::Module::vec>                              getMatchingModulesForName(const std::string_view& moduleName) const;
        bool                                                                         addEntry(const syrec::Variable::ptr& variable);
        bool                                                                         addEntry(const syrec::Number::ptr& number, const unsigned int bitsRequiredToStoreMaximumValue, const std::optional<unsigned int>& defaultValue);
        bool                                                                         addEntry(const syrec::Module::ptr& module);                     

        // BEGIN
        // TODO: UNUSED_REFERENCE - Marked as used
        void markLiteralAsUsed(const std::string_view& literalIdent) const;
        void markModulesMatchingSignatureAsUsed(const syrec::Module::ptr& module) const;

        [[nodiscard]] std::set<std::string> getUnusedLiterals() const;
        [[nodiscard]] std::vector<bool>     determineIfModuleWasUsed(const syrec::Module::vec& modules) const;
        // END

        static void openScope(SymbolTable::ptr& currentScope);
        static void closeScope(SymbolTable::ptr& currentScope);
        static bool isAssignableTo(const syrec::Variable::ptr& formalParameter, const syrec::Variable::ptr& actualParameter, bool mustParameterTypesMatch);

    private:
        struct VariableSymbolTableEntry {
            std::variant<syrec::Variable::ptr, syrec::Number::ptr> variableInformation;
            std::optional<valueLookup::SignalValueLookup::ptr>     optionalValueLookup;
            bool                                                   isUnused;

            VariableSymbolTableEntry(const syrec::Variable::ptr& variable) {
                variableInformation = std::variant<syrec::Variable::ptr, syrec::Number::ptr>(variable);
                isUnused            = true;
                
                switch (variable->type) {
                    case syrec::Variable::Types::Out:
                    case syrec::Variable::Types::Wire:
                    case syrec::Variable::Types::Inout: {
                        valueLookup::SignalValueLookup::ptr valueLookup = std::make_shared<valueLookup::SignalValueLookup>(valueLookup::SignalValueLookup(variable->bitwidth, variable->dimensions, 0));
                        if (variable->type == syrec::Variable::Types::Inout) {
                            valueLookup->invalidateAllStoredValuesForSignal();
                        }
                        optionalValueLookup = valueLookup;
                        break;   
                    }
                    default:
                        break;
                }

            }
            VariableSymbolTableEntry(const syrec::Number::ptr& number, const unsigned int bitsRequiredToStoreMaximumValue, const std::optional<unsigned int>& defaultLoopVariableValue) {
                variableInformation = std::variant<syrec::Variable::ptr, syrec::Number::ptr>(number);
                isUnused            = true;

                valueLookup::SignalValueLookup::ptr valueLookup;
                if (defaultLoopVariableValue.has_value()) {
                    valueLookup = std::make_shared<valueLookup::SignalValueLookup>(valueLookup::SignalValueLookup(bitsRequiredToStoreMaximumValue, {1}, *defaultLoopVariableValue));
                } else {
                    // TODO: We are currently assuming that we can only store values that required at most 32 bits to store its value
                    valueLookup = std::make_shared<valueLookup::SignalValueLookup>(valueLookup::SignalValueLookup(32, {1}, 0));
                    valueLookup->invalidateAllStoredValuesForSignal();
                }
                optionalValueLookup = valueLookup;
            }
        };

        struct ModuleSymbolTableEntry {
            syrec::Module::vec matchingModules;
            std::vector<bool>  isUnusedFlagPerModule;

            ModuleSymbolTableEntry(const syrec::Module::ptr& module):
                matchingModules({module}), isUnusedFlagPerModule({true}){}
        };

        std::map<std::string, std::shared_ptr<VariableSymbolTableEntry>, std::less<void>> locals;
        std::map<std::string, std::shared_ptr<ModuleSymbolTableEntry>, std::less<void>>   modules;
        SymbolTable::ptr outer;

        
        [[nodiscard]] ModuleSymbolTableEntry* getEntryForModulesWithMatchingName(const std::string_view& moduleIdent) const;
        [[nodiscard]] VariableSymbolTableEntry* getEntryForVariable(const std::string_view& literalIdent) const;
        static bool doModuleSignaturesMatch(const syrec::Module::ptr& module, const syrec::Module::ptr& otherModule);
    };    
}

#endif