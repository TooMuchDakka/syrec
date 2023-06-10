#ifndef LOOP_BODY_VALUE_PROPAGATION_BLOCKER_HPP
#define LOOP_BODY_VALUE_PROPAGATION_BLOCKER_HPP
#pragma once

#include "core/syrec/statement.hpp"
#include "core/syrec/parser/symbol_table.hpp"
#include "core/syrec/parser/optimizations/constantPropagation/valueLookup/signal_value_lookup.hpp"

namespace optimizations {
    class LoopBodyValuePropagationBlocker {
    public:
        explicit           LoopBodyValuePropagationBlocker(const syrec::Statement::vec& stmtBlock, const parser::SymbolTable::ptr& symbolTable, const std::optional<std::shared_ptr<LoopBodyValuePropagationBlocker>>& aggregateOfExistingLoopBodyValueRestrictions);
        [[nodiscard]] bool isAccessBlockedFor(const syrec::VariableAccess::ptr& accessedPartsOfSignal) const;

    private:
        std::map<std::string, valueLookup::SignalValueLookup::ptr> assignedToSignalsInLoopBody;
        const parser::SymbolTable::ptr&                            symbolTableReference;

        void handleStatement(const syrec::Statement::ptr& stmt);
        void handleAssignment(const syrec::VariableAccess::ptr& assignedToSignalParts);
        [[nodiscard]] std::optional<valueLookup::SignalValueLookup::ptr> fetchAndAddNewAssignedToSignalToLookupIfNoEntryExists(const syrec::VariableAccess::ptr& assignedToSignalParts);

        [[nodiscard]] static std::vector<std::optional<unsigned int>> transformDimensionAccess(const syrec::VariableAccess::ptr& accessedPartsOfSignal);
        [[nodiscard]] static std::optional<BitRangeAccessRestriction::BitRangeAccess> transformAccessedBitRange(const syrec::VariableAccess::ptr& accessedPartsOfSignal);

        template<typename T>
        [[nodiscard]] std::shared_ptr<T> tryConvertStmtToStmtOfOtherType(const syrec::Statement::ptr& stmt) {
            return std::dynamic_pointer_cast<T>(stmt);
        }
    };
}
#endif