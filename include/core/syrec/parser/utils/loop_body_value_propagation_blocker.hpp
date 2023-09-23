#ifndef LOOP_BODY_VALUE_PROPAGATION_BLOCKER_HPP
#define LOOP_BODY_VALUE_PROPAGATION_BLOCKER_HPP
#pragma once

#include "core/syrec/statement.hpp"
#include "core/syrec/parser/symbol_table.hpp"
#include "core/syrec/parser/optimizations/constantPropagation/valueLookup/signal_value_lookup.hpp"

#include <stack>

namespace optimizations {
    class LoopBodyValuePropagationBlocker {
    public:
        explicit LoopBodyValuePropagationBlocker(const parser::SymbolTable::ptr& symbolTableReference)
            : symbolTableReference(symbolTableReference) {}

        void openNewScopeAndAppendDataDataFlowAnalysisResult(const std::vector<std::reference_wrapper<const syrec::Statement>>& stmtsToAnalyze, bool* wereAnyAssignmentsPerformedInCurrentScope);
        void closeScopeAndDiscardDataFlowAnalysisResult();

        [[nodiscard]] bool isAccessBlockedFor(const syrec::VariableAccess& accessedPartsOfSignal) const;
    private:
        struct ScopeLocalAssignmentParts {
            std::vector<std::optional<unsigned int>>                 dimensionAccess;
            std::optional<BitRangeAccessRestriction::BitRangeAccess> bitRange;
        };
        
        const parser::SymbolTable::ptr&                                           symbolTableReference;
        std::map<std::string, valueLookup::SignalValueLookup::ptr>                restrictionStatusPerSignal;
        std::stack<std::map<std::string, std::vector<ScopeLocalAssignmentParts>>> uniqueAssignmentsPerScope;

        void                                                                           handleStatements(const std::vector<std::reference_wrapper<const syrec::Statement>>& stmtsToAnalyze, bool* wasUniqueAssignmentDefined);
        void                                                                           handleStatement(const syrec::Statement& stmt, bool* wasUniqueAssignmentDefined);
        [[nodiscard]] std::optional<std::pair<std::string, ScopeLocalAssignmentParts>> handleAssignment(const syrec::VariableAccess& assignedToSignalParts) const;
        [[nodiscard]] std::optional<ScopeLocalAssignmentParts>                         determineScopeLocalAssignmentFrom(const syrec::VariableAccess& assignedToSignal) const;

        [[nodiscard]] static std::vector<std::optional<unsigned int>>                 transformUserDefinedDimensionAccess(const syrec::VariableAccess& accessedPartsOfSignal);
        [[nodiscard]] static std::optional<BitRangeAccessRestriction::BitRangeAccess> transformUserDefinedBitRangeAccess(const syrec::VariableAccess& accessedPartsOfSignal);

        template<typename T>
        [[nodiscard]] static const T* stmtCastedAs(const syrec::Statement& stmt) {
            return dynamic_cast<const T*>(&stmt);
        }
        [[nodiscard]] static std::vector<std::reference_wrapper<const syrec::Statement>> transformCollectionOfSharedPointersToReferences(const syrec::Statement::vec& statements);
        [[nodiscard]] static std::optional<unsigned int>                                 tryEvaluateNumberToConstant(const syrec::Number& number);
        void                                                                             storeUniqueAssignmentForCurrentScope(const std::string& assignedToSignalIdent, const ScopeLocalAssignmentParts& uniqueAssignment);
        void                                                                             restrictAccessTo(const syrec::VariableAccess& assignedToSignalParts);
        void                                                                             restrictAccessTo(const syrec::Variable& assignedToSignal);
    };
}
#endif