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
        explicit DeadStoreEliminator(const parser::SymbolTable::ptr& symbolTable):
            symbolTable(symbolTable) {}

        /**
         * \brief Removes dead stores, i.e. assignments to parts of a signal that are not read by any subsequent statement, in the given list of statements. <br>
         * If an assignment statement does fit this criteria but if also any of the following conditions hold for any of the involved signals or expressions used in the assignment: <br>
         * I.   Division with unknown divisor or division by zero <br>
         * II.  SignalAccess with either unknown accessed value of dimension or bit range <br>
         * III. Assignment to variable with global side effect <br>
         * IV.  Assignment to undeclared variable <br>
         * then the assignment will not be considered as a dead store. <br>
         *
         * <em>NOTE: Noop assignments (i.e. x += 0) or similar 'dead' stores are not removed by this optimization.</em>
         * \param statementList The list of statements from which dead stores shall be removed
         */
        void removeDeadStoresFrom(syrec::Statement::vec& statementList);
    private:
        /*
         * BEGIN: Internal helper data structures
         */
        struct AssignmentStatementIndexInControlFlowGraph {
            std::vector<StatementIterationHelper::StatementIndexInBlock> relativeStatementIndexPerControlBlock;

            explicit AssignmentStatementIndexInControlFlowGraph(std::vector<StatementIterationHelper::StatementIndexInBlock> relativeIndexInControlFlowGraphOfStatement):
                relativeStatementIndexPerControlBlock(std::move(relativeIndexInControlFlowGraphOfStatement)) {}
        };

        struct PotentiallyDeadAssignmentStatement {
            syrec::VariableAccess::ptr                 assignedToSignalParts;
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

        // Could also be renamed to liveness status lookup conditional scope since a new scope is only opened for a branch in an if statement
        struct LivenessStatusLookupScope {
            std::map<std::string, DeadStoreStatusLookup::ptr> livenessStatusLookup;
        };

            
        struct LoopStatementInformation {
            std::size_t nestingLevelOfLoop;
            std::size_t numRemainingNonControlFlowStatementsInLoopBody;
            bool        performsMoreThanOneIteration;

            LoopStatementInformation(std::size_t nestingLevelOfLoop, std::size_t numRemainingNonControlFlowStatementsInLoopBody, bool performsOnlyOneIteration):
                nestingLevelOfLoop(nestingLevelOfLoop), numRemainingNonControlFlowStatementsInLoopBody(numRemainingNonControlFlowStatementsInLoopBody), performsMoreThanOneIteration(performsOnlyOneIteration) {}
        };
        /*
         * END: Internal helper data structures
         */

        std::map<std::string, std::vector<PotentiallyDeadAssignmentStatement>> assignmentStmtIndizesPerSignal;
        std::optional<AssignmentStatementIndexInControlFlowGraph>              indexOfLastProcessedStatementInControlFlowGraph;
        const parser::SymbolTable::ptr&                                        symbolTable;
        std::vector<std::shared_ptr<LivenessStatusLookupScope>>                livenessStatusScopes;
        std::vector<LoopStatementInformation>                                  remainingNonControlFlowStatementsPerLoopBody;

        [[nodiscard]] bool                     tryCreateCopyOfLivenessStatusForSignalInCurrentScope(const std::string& signalIdent);
        void                                   mergeLivenessStatusOfCurrentScopeWithParent();
        void                                   createNewLivenessStatusScope();
        void                                   destroyCurrentLivenessStatusScope();

        [[nodiscard]] std::vector<AssignmentStatementIndexInControlFlowGraph> findDeadStores(const syrec::Statement::vec& statementList);

        /*
         * Either create new scopes if the nesting level in comparison to the last statement increased
         * Or remove and merge scopes if the nesting level decreased
         */
        void updateLivenessStatusScopeAccordingToNestingLevelOfStatement(const std::optional<AssignmentStatementIndexInControlFlowGraph>& indexOfLastProcessedStatement, const AssignmentStatementIndexInControlFlowGraph& indexOfCurrentStatement);


        [[nodiscard]] std::optional<std::size_t>                                findScopeContainingEntryForSignal(const std::string& signalIdent) const;
        [[nodiscard]] bool isReachableInReverseControlFlowGraph(const AssignmentStatementIndexInControlFlowGraph& assignmentStatement, const AssignmentStatementIndexInControlFlowGraph& usageOfAssignedToSignal) const;
        [[nodiscard]] std::size_t determineNestingLevelMeasuredForIfStatements(const AssignmentStatementIndexInControlFlowGraph& statementIndexInControlFlowGraph) const;
        [[nodiscard]] std::optional<std::shared_ptr<LivenessStatusLookupScope>> getLivenessStatusLookupForCurrentScope() const;
    

        /*
         * I.   Division with unknown divisor
         * II.  SignalAccess with either unknown accessed value of dimension or bit range
         * III. Assignment to variable with global side effect
         * IV.  Assignment to undeclared variable
         *
         */
        [[nodiscard]] bool                        doesAssignmentContainPotentiallyUnsafeOperation(const syrec::Statement::ptr& stmt) const;
        [[nodiscard]] bool                        doesExpressionContainPotentiallyUnsafeOperation(const syrec::expression::ptr& expr) const;
        [[nodiscard]] bool                        wasSignalDeclaredAndAreAllIndizesOfSignalConstantsAndWithinRange(const syrec::VariableAccess::ptr& signalAccess) const;
        [[nodiscard]] bool                        isAssignedToSignalAModifiableParameter(const std::string_view& assignedToSignalIdent) const;
        [[nodiscard]] std::optional<unsigned int> tryEvaluateNumber(const syrec::Number::ptr& number) const;

        [[nodiscard]] bool isNextDeadStoreInSameBranch(std::size_t currentDeadStoreIndex, const std::vector<AssignmentStatementIndexInControlFlowGraph>& foundDeadStores, std::size_t currentNestingLevel) const;

        /*
         * Used to mark signals as live when its value is read in an expression
         *
         */
        void               markAccessedVariablePartsAsLive(const syrec::VariableAccess::ptr& signalAccess, const AssignmentStatementIndexInControlFlowGraph& indexOfStatementContainingSignalAccess);
        [[nodiscard]] bool isAccessedVariablePartLive(const syrec::VariableAccess::ptr& signalAccess) const;

        [[nodiscard]] std::vector<std::optional<unsigned int>>                                transformUserDefinedDimensionAccess(std::size_t numDimensionsOfAccessedSignal, const std::vector<syrec::expression::ptr>& dimensionAccess) const;
        [[nodiscard]] std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess> transformUserDefinedBitRangeAccess(unsigned int accessedSignalBitwidth, const std::optional<std::pair<syrec::Number::ptr, syrec::Number::ptr>>& bitRangeAccess) const;
        [[nodiscard]] std::vector<AssignmentStatementIndexInControlFlowGraph>                 combineAndSortDeadRemainingDeadStores();

        [[nodiscard]] static std::size_t getBlockTypePrecedence(StatementIterationHelper::BlockType blockType);

        void markAccessedSignalsAsLiveInExpression(const syrec::expression::ptr& expr, const AssignmentStatementIndexInControlFlowGraph& indexOfStatementContainingExpression);
        void markAccessedSignalsAsLiveInCallStatement(const std::shared_ptr<syrec::CallStatement>& callStmt, const AssignmentStatementIndexInControlFlowGraph& indexOfCallStmt);
        void insertPotentiallyDeadAssignmentStatement(const syrec::VariableAccess::ptr& assignedToSignalParts, const std::vector<StatementIterationHelper::StatementIndexInBlock>& relativeIndexOfStatementInControlFlowGraph);
        void removeNoLongerDeadStores(const std::string& accessedSignalIdent, const AssignmentStatementIndexInControlFlowGraph& indexOfStatementContainingSignalAccess);
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