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

        [[nodiscard]] bool                                                                  contains(const std::string_view& literalIdent) const;
        [[nodiscard]] bool                                                                  contains(const syrec::Module::ptr& module) const;
        [[nodiscard]] std::optional<std::variant<syrec::Variable::ptr, syrec::Number::ptr>> getVariable(const std::string_view& literalIdent) const;
        [[nodiscard]] std::optional<syrec::Module::vec>                                     getMatchingModulesForName(const std::string_view& moduleName) const;
        [[nodiscard]] std::optional<std::set<std::size_t>>                                  getPositionOfUnusedParametersForModule(const syrec::Module::ptr& module) const;
        bool                                                                                addEntry(const syrec::Variable::ptr& variable);
        bool                                                                                addEntry(const syrec::Number::ptr& number, const unsigned int bitsRequiredToStoreMaximumValue, const std::optional<unsigned int>& defaultValue);
        bool                                                                                addEntry(const syrec::Module::ptr& module, const std::vector<bool>& isUnusedStatusPerModuleParameter);
        void                                                                                removeVariable(const std::string& literalIdent);

        // BEGIN 
        // TODO: UNUSED_REFERENCE - Marked as used
        void incrementLiteralReferenceCount(const std::string_view& literalIdent) const;
        void decrementLiteralReferenceCount(const std::string_view& literalIdent) const;

        void incrementReferenceCountOfModulesMatchingSignature(const syrec::Module::ptr& module) const;
        void decrementReferenceCountOfModulesMatchingSignature(const syrec::Module::ptr& module) const;

        // TODO: CONSTANT_PROPAGATION
        [[nodiscard]] std::optional<unsigned int> tryFetchValueForLiteral(const syrec::VariableAccess::ptr& assignedToSignalParts) const;
        [[nodiscard]] std::optional<unsigned int> tryFetchValueOfLoopVariable(const std::string_view& loopVariableIdent) const;
        void                                      invalidateStoredValuesFor(const syrec::VariableAccess::ptr& assignedToSignalParts) const;
        void                                      invalidateStoredValueForLoopVariable(const std::string_view& loopVariableIdent) const;
        void                                      updateStoredValueFor(const syrec::VariableAccess::ptr& assignedToSignalParts, unsigned int newValue) const;
        void                                      updateStoredValueForLoopVariable(const std::string_view& loopVariableIdent, unsigned int newValue) const;

        void updateViaSignalAssignment(const syrec::VariableAccess::ptr& assignmentLhsOperand, const syrec::VariableAccess::ptr& assignmentRhsOperand) const;
        void swap(const syrec::VariableAccess::ptr& swapLhsOperand, const syrec::VariableAccess::ptr& swapRhsOperand) const;

        [[nodiscard]] std::set<std::string>                              getUnusedLiterals() const;
        [[nodiscard]] std::vector<bool>                                  determineIfModuleWasUsed(const syrec::Module::vec& modules) const;
        [[nodiscard]] std::optional<valueLookup::SignalValueLookup::ptr> createBackupOfValueOfSignal(const std::string_view& literalIdent) const;
        void                                                             restoreValuesFromBackup(const std::string_view& literalIdent, const valueLookup::SignalValueLookup::ptr& newValues) const;
        // END

        static void openScope(SymbolTable::ptr& currentScope);
        static void closeScope(SymbolTable::ptr& currentScope);
        static bool isAssignableTo(const syrec::Variable::ptr& formalParameter, const syrec::Variable::ptr& actualParameter, bool mustParameterTypesMatch);

    private:
        struct VariableSymbolTableEntry {
            std::variant<syrec::Variable::ptr, syrec::Number::ptr> variableInformation;
            std::optional<valueLookup::SignalValueLookup::ptr>     optionalValueLookup;
            std::size_t                                            referenceCount;
            
            VariableSymbolTableEntry(const syrec::Variable::ptr& variable): referenceCount(0) {
                variableInformation = std::variant<syrec::Variable::ptr, syrec::Number::ptr>(variable);

                valueLookup::SignalValueLookup::ptr valueLookup = std::make_shared<valueLookup::SignalValueLookup>(valueLookup::SignalValueLookup(variable->bitwidth, variable->dimensions, 0));
                if (variable->type == syrec::Variable::Types::In || variable->type == syrec::Variable::Types::Inout) {
                    valueLookup->invalidateAllStoredValuesForSignal();
                }
                optionalValueLookup = valueLookup;
            }
            VariableSymbolTableEntry(const syrec::Number::ptr& number, const unsigned int bitsRequiredToStoreMaximumValue, const std::optional<unsigned int>& defaultLoopVariableValue): referenceCount(0) {
                variableInformation = std::variant<syrec::Variable::ptr, syrec::Number::ptr>(number);

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
            std::vector<std::size_t> referenceCountsPerModule;
            std::vector<std::vector<bool>> optimizedAwayStatusPerParameterOfModulePerModule;

            ModuleSymbolTableEntry(): matchingModules({}), referenceCountsPerModule({}), optimizedAwayStatusPerParameterOfModulePerModule({}) {}

            void addModule(const syrec::Module::ptr& module, const std::vector<bool>& isUnusedStatusPerModuleParameter) {
                matchingModules.emplace_back(module);
                referenceCountsPerModule.emplace_back(0);
                optimizedAwayStatusPerParameterOfModulePerModule.emplace_back(isUnusedStatusPerModuleParameter);
            }
        };

        std::map<std::string, std::shared_ptr<VariableSymbolTableEntry>, std::less<void>> locals;
        std::map<std::string, std::shared_ptr<ModuleSymbolTableEntry>, std::less<void>>   modules;
        SymbolTable::ptr outer;

        
        [[nodiscard]] ModuleSymbolTableEntry*   getEntryForModulesWithMatchingName(const std::string_view& moduleIdent) const;
        [[nodiscard]] VariableSymbolTableEntry* getEntryForVariable(const std::string_view& literalIdent) const;
        static bool                             doModuleSignaturesMatch(const syrec::Module::ptr& module, const syrec::Module::ptr& otherModule);
        void                                    incrementOrDecrementReferenceCountOfModulesMatchingSignature(const syrec::Module::ptr& module, bool incrementReferenceCount) const;

        [[nodiscard]] bool doesVariableAccessAllowValueLookup(const VariableSymbolTableEntry* symbolTableEntryForVariable, const syrec::VariableAccess::ptr& variableAccess) const;
        static std::optional<::optimizations::BitRangeAccessRestriction::BitRangeAccess> tryTransformAccessedBitRange(const syrec::VariableAccess::ptr& accessedSignalParts);
        static std::vector<std::optional<unsigned int>>                                  tryTransformAccessedDimensions(const syrec::VariableAccess::ptr& accessedSignalParts, bool isAccessedSignalLoopVariable);
    };    
}

#endif