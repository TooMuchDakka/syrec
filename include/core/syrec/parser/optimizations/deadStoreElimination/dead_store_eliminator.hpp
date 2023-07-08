#ifndef DEAD_STORE_ELIMINATOR_HPP
#define DEAD_STORE_ELIMINATOR_HPP
#pragma once

#include "dead_store_status_lookup.hpp"

#include "core/syrec/variable.hpp"
#include "core/syrec/parser/symbol_table.hpp"
#include "core/syrec/parser/optimizations/constantPropagation/valueLookup/base_value_lookup.hpp"

#include <vector>
#include "statement_iteration_helper.hpp"
namespace deadStoreElimination {
    class DeadStoreEliminator {
    public:
        struct AssignmentStatementIndexInControlFlowGraph {
            std::vector<StatementIterationHelper::StatementIndexInBlock> relativeStatementIndexPerControlBlock;

            explicit AssignmentStatementIndexInControlFlowGraph(std::vector<StatementIterationHelper::StatementIndexInBlock> relativeIndexInControlFlowGraphOfStatement):
                relativeStatementIndexPerControlBlock(std::move(relativeIndexInControlFlowGraphOfStatement)) {}
        };

        struct PotentiallyDeadAssignmentStatement {
            syrec::VariableAccess::ptr                assignedToSignalParts;
            AssignmentStatementIndexInControlFlowGraph indexInControlFlowGraph;

            PotentiallyDeadAssignmentStatement(syrec::VariableAccess::ptr assignedToSignalParts, std::vector<StatementIterationHelper::StatementIndexInBlock> relativeIndexInControlFlowGraphOfStatement):
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
        void                                                                  removeDeadStoresFrom(syrec::Statement::vec& statementList, const std::vector<AssignmentStatementIndexInControlFlowGraph>& foundDeadStores) const;

    private:
        std::map<std::string, std::vector<PotentiallyDeadAssignmentStatement>> assignmentStmtIndizesPerSignal;
        std::map<std::string, DeadStoreStatusLookup::ptr>                      livenessStatusLookup;
        const parser::SymbolTable::ptr&                                        symbolTable;

        struct LoopStatementInformation {
            std::size_t nestingLevelOfLoop;
            std::size_t numRemainingNonControlFlowStatementsInLoopBody;
            bool        performsMoreThanOneIteration;

            LoopStatementInformation(std::size_t nestingLevelOfLoop, std::size_t numRemainingNonControlFlowStatementsInLoopBody, bool performsOnlyOneIteration):
                nestingLevelOfLoop(nestingLevelOfLoop), numRemainingNonControlFlowStatementsInLoopBody(numRemainingNonControlFlowStatementsInLoopBody), performsMoreThanOneIteration(performsOnlyOneIteration) {}
        };
        std::vector<LoopStatementInformation>                                   remainingNonControlFlowStatementsPerLoopBody;


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
        [[nodiscard]] bool                        doesAssignmentContainPotentiallyUnsafeOperation(const syrec::Statement::ptr& stmt) const;
        [[nodiscard]] bool                        doesExpressionContainPotentiallyUnsafeOperation(const syrec::expression::ptr& expr) const;
        [[nodiscard]] bool                        wasSignalDeclaredAndAreAllIndizesOfSignalConstantsAndWithinRange(const syrec::VariableAccess::ptr& signalAccess) const;
        [[nodiscard]] bool                        isAssignedToSignalAModifiableParameter(const std::string_view& assignedToSignalIdent) const;
        [[nodiscard]] std::optional<unsigned int> tryEvaluateNumber(const syrec::Number::ptr& number) const;

        /*
         * Used to mark signals as live when its value is read in an expression
         *
         */
        void               markAccessedVariablePartsAsLive(const syrec::VariableAccess::ptr& signalAccess);
        [[nodiscard]] bool isAccessedVariablePartLive(const syrec::VariableAccess::ptr& signalAccess) const;

        [[nodiscard]] std::vector<std::optional<unsigned int>>                                transformUserDefinedDimensionAccess(std::size_t numDimensionsOfAccessedSignal, const std::vector<syrec::expression::ptr>& dimensionAccess) const;
        [[nodiscard]] std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess> transformUserDefinedBitRangeAccess(unsigned int accessedSignalBitwidth, const std::optional<std::pair<syrec::Number::ptr, syrec::Number::ptr>>& bitRangeAccess) const;
        [[nodiscard]] std::vector<AssignmentStatementIndexInControlFlowGraph>                 combineAndSortDeadRemainingDeadStores();

        void markAccessedSignalsAsLiveInExpression(const syrec::expression::ptr& expr);
        void markAccessedSignalsAsLiveInCallStatement(const  std::shared_ptr<syrec::CallStatement>& callStmt);
        void insertPotentiallyDeadAssignmentStatement(const syrec::VariableAccess::ptr& assignedToSignalParts, const std::vector<StatementIterationHelper::StatementIndexInBlock>& relativeIndexOfStatementInControlFlowGraph);
        void removeNoLongerDeadStores(const std::string& accessedSignalIdent);
        void markAccessedSignalPartsAsDead(const syrec::VariableAccess::ptr& signalAccess) const;
        void resetInternalData();

        void removeDeadStoresFrom(syrec::Statement::vec& statementList, const std::vector<AssignmentStatementIndexInControlFlowGraph>& foundDeadStores, std::size_t& currDeadStoreIndex, std::size_t nestingLevelOfCurrentBlock) const;

        void decrementReferenceCountOfUsedSignalsInStatement(const syrec::Statement::ptr& statement) const;
        void decrementReferenceCountsOfUsedSignalsInExpression(const syrec::expression::ptr& expr) const;
        void decrementReferenceCountForAccessedSignal(const syrec::VariableAccess::ptr& accessedSignal) const;
        void decrementReferenceCountOfNumber(const syrec::Number::ptr& number) const;

        [[nodiscard]] bool isAssignmentDefinedInLoopPerformingMoreThanOneIteration() const;
        [[nodiscard]] bool doesLoopPerformMoreThanOneIteration(const std::shared_ptr<syrec::ForStatement>& loopStmt) const;
        void               markStatementAsProcessedInLoopBody(const syrec::Statement::ptr& stmt, std::size_t nestingLevelOfStmt);
        void               addInformationAboutLoopWithMoreThanOneStatement(const std::shared_ptr<syrec::ForStatement>& loopStmt, std::size_t nestingLevelOfStmt);
    };
}

#endif