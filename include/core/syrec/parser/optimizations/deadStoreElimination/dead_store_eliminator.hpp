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

        // TODO: Currently stores without global side effects are not removed (since they are not detected as 'dead' stores)
        // Since the current definition does not apply to them
        // If we would extend this definition to something of the sort of, if for a given list of assignment to some signal x there does not exist a global side effect that overlaps
        // with any of the assignments with local side effect, the assignment could also be considered as dead.

        /**
         * \brief Removes dead stores, i.e. assignments to parts of a signal that are not read by any subsequent statement, in the given list of statements. <br>
         * If an assignment statement does fit this criteria but if also any of the following conditions hold for any of the involved signals or expressions used in the assignment: <br>
         * I.   Division with unknown divisor or division by zero <br>
         * II.  SignalAccess with either unknown accessed value of dimension or bit range <br>
         * III. Assignment to variable with global side effect <br>
         * IV.  Assignment to undeclared variable <br>
         * then the assignment will not be considered as a dead store. <br>
         *
         * <em>NOTE: Noop assignments (i.e. x += 0) are also considered as dead stores.</em>
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
        std::map<std::string, std::vector<PotentiallyDeadAssignmentStatement>> liveStoresWithoutUsageInGlobalSideEffect;
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
        [[nodiscard]] static std::optional<unsigned int>        tryEvaluateNumber(const syrec::Number& number);
        [[nodiscard]] static std::optional<unsigned int> tryEvaluateExprToConstant(const syrec::expression& expr);

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
        
        [[nodiscard]] bool                                    doesAssignmentOverlapAnyAccessedPartOfSignal(const syrec::VariableAccess::ptr& assignedToSignalParts, const syrec::VariableAccess::ptr& accessedSignalParts) const;
        [[nodiscard]] static bool                             doBitRangesOverlap(const optimizations::BitRangeAccessRestriction::BitRangeAccess& assignedToBitRange, const optimizations::BitRangeAccessRestriction::BitRangeAccess& accessedBitRange);
        [[nodiscard]] static bool                             doDimensionAccessesOverlap(const std::vector<std::optional<unsigned>>& assignedToDimensionAccess, const std::vector<std::optional<unsigned int>>& accessedDimensionAccess);
        [[nodiscard]] std::vector<syrec::VariableAccess::ptr> getAccessedLocalSignalsFromExpression(const syrec::expression::ptr& expr) const;

        // TODO: Renaming of all calls below of this comment
        void                                                  markAccessedLocalSignalsInExpressionAsUsedInNonLocalAssignment(const syrec::expression::ptr& expr, const AssignmentStatementIndexInControlFlowGraph& indexOfStatementContainingExpression);
        void                                                  markAccessedLocalSignalsInStatementAsUsedInNonLocalAssignment(const syrec::Statement::ptr& stmt, const AssignmentStatementIndexInControlFlowGraph& indexOfStatement);
        void                                                  markAccessedLocalSignalAsUsedInNonLocalAssignment(const syrec::VariableAccess::ptr& accessedSignal, const AssignmentStatementIndexInControlFlowGraph& indexOfStatementDefiningAccess);
        [[nodiscard]] bool                                    isAccessedSignalLocalOfModule(const syrec::VariableAccess::ptr& accessedSignal) const;

        void                                                  markAssignmentsToLocalSignalAsRequiredForGlobalSideEffect(const syrec::VariableAccess::ptr& accessedLocalSignalParts, const AssignmentStatementIndexInControlFlowGraph& indexOfStatementContainingSignalAccess);
        void                                                  markPreviouslyDeadStoreAsLiveWithoutUsageInGlobalSideEffect(const syrec::VariableAccess::ptr& assignedToSignalParts, const AssignmentStatementIndexInControlFlowGraph& indexOfAssignmentStatement);
        void                                                  addAssignmentsUsedOnlyInAssignmentsToLocalSignalsAsDeadStores();
    };
}
#endif