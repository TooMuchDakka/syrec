#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP
#pragma once

#include "core/syrec/module.hpp"
#include "core/syrec/variable.hpp"
#include "optimizations/constantPropagation/valueLookup/signal_value_lookup.hpp"

#include <memory>
#include <unordered_set>

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

        struct ModuleCallSignature {
            std::string moduleIdent;
            syrec::Variable::vec parameters;
        };

        struct ModuleIdentifier {
            std::string_view moduleIdent;
            std::size_t      internalModuleId;
        };

        struct DeclaredModuleSignature {
            std::size_t              internalModuleId;
            syrec::Variable::vec     declaredParameters;
            std::vector<std::size_t> indicesOfOptimizedAwayParameters;

            [[maybe_unused]] syrec::Variable::vec determineOptimizedCallSignature(std::unordered_set<std::size_t>* indicesOfRemainingParameters) const;
        };

        [[nodiscard]] std::optional<DeclaredModuleSignature>        getMatchingModuleForIdentifier(const ModuleIdentifier& moduleIdentifier) const;
        [[nodiscard]] std::vector<DeclaredModuleSignature>          getMatchingModuleSignaturesForName(const std::string_view& moduleName) const;
        [[nodiscard]] std::optional<DeclaredModuleSignature>        tryGetOptimizedSignatureForModuleCall(const std::string_view& moduleName, const std::vector<std::string>& calleeArguments, bool areCalleeArgumentsBasedOnOptimizedSignature) const;
        [[nodiscard]] std::optional<DeclaredModuleSignature>        tryGetOptimizedSignatureForModuleCall(const syrec::Module& callTarget) const;
        [[nodiscard]] std::optional<std::unique_ptr<syrec::Module>> getFullDeclaredModuleInformation(const ModuleIdentifier& moduleIdentifier) const;

        [[maybe_unused]] bool addEntry(const syrec::Variable& variable);
        [[maybe_unused]] bool addEntry(const syrec::Number& number, const unsigned int bitsRequiredToStoreMaximumValue, const std::optional<unsigned int>& defaultValue);
        [[maybe_unused]] bool addEntry(const syrec::Module& module, std::size_t* internalModuleId);
        void removeVariable(const std::string& literalIdent);

        enum ReferenceCountUpdate {
            Increment,
            Decrement
        };

        void                  markModuleVariableAsUnused(const ModuleIdentifier& moduleIdentifier, const std::string_view& literalIdent, bool doesIdentReferenceModuleParameter) const;
        void                  updateReferenceCountOfLiteral(const std::string_view& literalIdent, ReferenceCountUpdate typeOfUpdate) const;
        void                  updateReferenceCountOfModulesMatchingSignature(const std::vector<ModuleIdentifier>& moduleIdentifiers, ReferenceCountUpdate typeOfUpdate) const;
        [[maybe_unused]] bool changeStatementsOfModule(const ModuleIdentifier& moduleIdentifier, const syrec::Statement::vec& updatedModuleBodyStatements) const;
        [[nodiscard]] bool    isVariableUsedAnywhereBasedOnReferenceCount(const std::string_view& literalIdent) const;

        // TODO: CONSTANT_PROPAGATION
        [[nodiscard]] std::optional<unsigned int> tryFetchValueForLiteral(const syrec::VariableAccess::ptr& assignedToSignalParts) const;
        [[nodiscard]] std::optional<unsigned int> tryFetchValueOfLoopVariable(const std::string_view& loopVariableIdent) const;
        void                                      invalidateStoredValuesFor(const syrec::VariableAccess::ptr& assignedToSignalParts) const;
        void                                      invalidateStoredValueForLoopVariable(const std::string_view& loopVariableIdent) const;
        void                                      updateStoredValueFor(const syrec::VariableAccess::ptr& assignedToSignalParts, unsigned int newValue) const;
        void                                      updateStoredValueForLoopVariable(const std::string_view& loopVariableIdent, unsigned int newValue) const;

        void updateViaSignalAssignment(const syrec::VariableAccess::ptr& assignmentLhsOperand, const syrec::VariableAccess::ptr& assignmentRhsOperand) const;
        void swap(const syrec::VariableAccess::ptr& swapLhsOperand, const syrec::VariableAccess::ptr& swapRhsOperand) const;

        [[nodiscard]] std::unordered_set<std::string>   getUnusedLiterals() const;
        [[nodiscard]] std::unordered_set<std::size_t>   updateOptimizedModuleSignatureByMarkingAndReturningIndicesOfUnusedParameters(const ModuleIdentifier& moduleIdentifier) const;
        [[nodiscard]] std::unordered_set<std::size_t>   fetchUnusedLocalModuleVariableIndicesAndRemoveFromSymbolTable(const ModuleIdentifier& moduleIdentifier) const;

        [[nodiscard]] std::optional<bool>                                canSignalBeAssignedTo(const std::string_view& literalIdent) const;
        [[nodiscard]] std::optional<bool>                                determineIfModuleWasUnused(const ModuleCallSignature& moduleSignatureToCheck) const;
        [[nodiscard]] std::optional<valueLookup::SignalValueLookup::ptr> createBackupOfValueOfSignal(const std::string_view& literalIdent) const;
        void                                                             restoreValuesFromBackup(const std::string_view& literalIdent, const valueLookup::SignalValueLookup::ptr& newValues) const;
        // END

        static void openScope(SymbolTable::ptr& currentScope);
        static void closeScope(SymbolTable::ptr& currentScope);
        static bool isAssignableTo(const syrec::Variable& formalParameter, const syrec::Variable& actualParameter, bool mustParameterTypesMatch);

    private:
        struct VariableSymbolTableEntry {
            std::variant<syrec::Variable::ptr, syrec::Number::ptr> variableInformation;
            std::optional<valueLookup::SignalValueLookup::ptr>     optionalValueLookup;
            std::size_t                                            referenceCount;
            
            VariableSymbolTableEntry(const syrec::Variable& variable): referenceCount(0) {
                const auto& sharedPointerToVariable = std::make_shared<syrec::Variable>(variable);
                variableInformation = std::variant<syrec::Variable::ptr, syrec::Number::ptr>(sharedPointerToVariable);

                valueLookup::SignalValueLookup::ptr valueLookup = std::make_shared<valueLookup::SignalValueLookup>(valueLookup::SignalValueLookup(variable.bitwidth, variable.dimensions, 0));
                if (variable.type == syrec::Variable::Types::In || variable.type == syrec::Variable::Types::Inout) {
                    valueLookup->invalidateAllStoredValuesForSignal();
                }
                optionalValueLookup = valueLookup;
            }
            VariableSymbolTableEntry(const syrec::Number& number, const unsigned int bitsRequiredToStoreMaximumValue, const std::optional<unsigned int>& defaultLoopVariableValue): referenceCount(0) {
                const auto& sharedPointerToNumber = std::make_shared<syrec::Number>(number);
                variableInformation = std::variant<syrec::Variable::ptr, syrec::Number::ptr>(sharedPointerToNumber);

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
            struct InternalModuleHelperData {
                const std::size_t              internalId;
                const syrec::Module::ptr       declaredModule;
                std::size_t                    referenceCount;
                std::vector<std::size_t>       indicesOfDroppedParametersInOptimizedVersion;
            };

            std::map<std::size_t, InternalModuleHelperData> internalDataLookup;

            [[nodiscard]] std::size_t addModule(const syrec::Module& module) {
                const auto& sharedPointerToModule = std::make_shared<syrec::Module>(module);
                const auto internalIdForModule = internalDataLookup.size();
                internalDataLookup.insert(std::make_pair(internalIdForModule, InternalModuleHelperData({internalIdForModule, sharedPointerToModule, 0, {}})));
                return internalIdForModule;
            }
        };

        std::map<std::string, std::shared_ptr<VariableSymbolTableEntry>, std::less<void>> locals;
        std::map<std::string, std::shared_ptr<ModuleSymbolTableEntry>, std::less<void>>   modules;
        SymbolTable::ptr outer;

        
        [[nodiscard]] ModuleSymbolTableEntry*   getEntryForModulesWithMatchingName(const std::string_view& moduleIdent) const;
        [[nodiscard]] VariableSymbolTableEntry* getEntryForVariable(const std::string_view& literalIdent) const;
        [[nodiscard]] static bool               doModuleSignaturesMatch(const syrec::Module::ptr& module, const syrec::Module::ptr& otherModule);

        [[nodiscard]] bool                                                                             doesVariableAccessAllowValueLookup(const VariableSymbolTableEntry* symbolTableEntryForVariable, const syrec::VariableAccess::ptr& variableAccess) const;
        [[nodiscard]] std::optional<DeclaredModuleSignature>                                           tryGetOptimizedSignatureForModuleCall(const std::string_view& moduleName, std::size_t& internalModuleId) const;
        [[nodiscard]] static std::optional<::optimizations::BitRangeAccessRestriction::BitRangeAccess> tryTransformAccessedBitRange(const syrec::VariableAccess::ptr& accessedSignalParts, bool considerUnknownBitRangeStartAndEndAsWholeSignalAccess = true);
        [[nodiscard]] static std::vector<std::optional<unsigned int>>                                  tryTransformAccessedDimensions(const syrec::VariableAccess::ptr& accessedSignalParts, bool isAccessedSignalLoopVariable);
        static void                                                                                    performReferenceCountUpdate(std::size_t& currentReferenceCount, ReferenceCountUpdate typeOfUpdate);
        static void                                                                                    addIndexOfDroppedParameterToOptimizedModule(ModuleSymbolTableEntry::InternalModuleHelperData& optimizedModuleData, std::size_t indexOfOptimizedAwayParameter);
        [[nodiscard]] static bool                                                                      doesVariableTypeAllowAssignment(syrec::Variable::Types variableTypeFlag);
        [[nodiscard]] static std::optional<syrec::Variable::Types>                                     convertVariableTypeFlagToEnum(unsigned int variableTypeFlag);
    };    
}

#endif