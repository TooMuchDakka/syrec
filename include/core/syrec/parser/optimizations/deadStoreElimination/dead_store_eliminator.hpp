#ifndef DEAD_STORE_ELIMINATOR_HPP
#define DEAD_STORE_ELIMINATOR_HPP
#pragma once

#include "dead_store_status_lookup.hpp"

#include "core/syrec/variable.hpp"
#include "core/syrec/parser/symbol_table.hpp"
#include "core/syrec/parser/optimizations/constantPropagation/valueLookup/base_value_lookup.hpp"

#include <vector>
namespace deadStoreElimination {
    class DeadStoreEliminator {
    public:
        struct AssignmentStatementIndexInControlFlowGraph {
            std::vector<std::size_t> relativeStatementIndexPerControlBlock;

            explicit AssignmentStatementIndexInControlFlowGraph(std::vector<std::size_t> relativeIndexInControlFlowGraphOfStatement):
                relativeStatementIndexPerControlBlock(std::move(relativeIndexInControlFlowGraphOfStatement)) {}
        };

        struct PotentiallyDeadAssignmentStatement {
            syrec::VariableAccess::ptr                assignedToSignalParts;
            AssignmentStatementIndexInControlFlowGraph indexInControlFlowGraph;

            PotentiallyDeadAssignmentStatement(syrec::VariableAccess::ptr assignedToSignalParts, std::vector<std::size_t> relativeIndexInControlFlowGraphOfStatement):
                assignedToSignalParts(std::move(assignedToSignalParts)),
                indexInControlFlowGraph(std::move(relativeIndexInControlFlowGraphOfStatement)) {}

            PotentiallyDeadAssignmentStatement(PotentiallyDeadAssignmentStatement&&) = default;
            PotentiallyDeadAssignmentStatement& operator=(PotentiallyDeadAssignmentStatement&& other) noexcept {
                std::swap(assignedToSignalParts, other.assignedToSignalParts);
                std::swap(indexInControlFlowGraph, other.indexInControlFlowGraph);
                return *this;
            }
        };

        explicit DeadStoreEliminator(const parser::SymbolTable::ptr& symbolTable):
            symbolTable(symbolTable) {}

        /*
         * Removes dead stores from the given list of statements and returns the former
         *
         */
        [[nodiscard]] std::vector<AssignmentStatementIndexInControlFlowGraph> findDeadStores(const syrec::Statement::vec& statementList);

    private:
        std::map<std::string, std::vector<PotentiallyDeadAssignmentStatement>> assignmentStmtIndizesPerSignal;
        std::map<std::string, DeadStoreStatusLookup::ptr>                      livenessStatusLookup;
        const parser::SymbolTable::ptr&                                        symbolTable;

        /*
         * Can be used to determine if an IF or LOOP statement can be removed if its branch/body does only contain dead stores
         * or module calls without side effects
         *
         */
        //[[nodiscard]] bool containsOnlyDeadStores(const syrec::Statement::vec& statementList);

        /*
         *  I.      Module does not contain critical operations
         *  II.     Liveness status of modifiable parameters is not changed 
         */
        //[[nodiscard]] bool moduleCallDoesNotModifyLivenessStatusOfParameters(const syrec::Module::ptr& module);

        /*
         * I.   Division with unknown divisor
         * II.  SignalAccess with either unknown accessed value of dimension or bit range 
         *
         */
        [[nodiscard]] bool                        doesAssignmentContainPotentiallyUnsafeOperation(const syrec::Statement::ptr& stmt);
        [[nodiscard]] bool                        doesExpressionContainPotentiallyUnsafeOperation(const syrec::expression::ptr& expr);
        [[nodiscard]] bool                        wasSignalDeclaredAndAreAllIndizesOfSignalConstantsAndWithinRange(const syrec::VariableAccess::ptr& signalAccess);
        [[nodiscard]] std::optional<unsigned int> tryEvaluateNumber(const syrec::Number::ptr& number) const;

        /*
         * Used to mark signals as live when its value is read in an expression
         *
         */
        void               markAccessedVariablePartsAsLive(const syrec::VariableAccess::ptr& signalAccess);
        [[nodiscard]] bool isAccessedVariablePartLive(const syrec::VariableAccess::ptr& signalAccess);

        [[nodiscard]] std::vector<std::optional<unsigned int>>                                transformUserDefinedDimensionAccess(std::size_t numDimensionsOfAccessedSignal, const std::vector<syrec::expression::ptr>& dimensionAccess);
        [[nodiscard]] std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess> transformUserDefinedBitRangeAccess(unsigned int accessedSignalBitwidth, const std::optional<std::pair<syrec::Number::ptr, syrec::Number::ptr>>& bitRangeAccess);
        [[nodiscard]] std::vector<AssignmentStatementIndexInControlFlowGraph>                 combineAndSortDeadRemainingDeadStores();

        void markAccessedSignalsAsLiveInExpression(const syrec::expression::ptr& expr);
        void markAccessedSignalsAsLiveInCallStatement(const  std::shared_ptr<syrec::CallStatement>& callStmt);
        void insertPotentiallyDeadAssignmentStatement(const syrec::VariableAccess::ptr& assignedToSignalParts, const std::vector<std::size_t>& relativeIndexOfStatementInControlFlowGraph);
        void removeNoLongerDeadStores(const std::string& accessedSignalIdent);
        void markAccessedSignalPartsAsDead(const syrec::VariableAccess::ptr& signalAccess);
        void resetInternalData();
    };
}

#endif