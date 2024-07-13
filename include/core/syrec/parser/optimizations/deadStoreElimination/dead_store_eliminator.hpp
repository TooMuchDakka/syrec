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

            bool operator==(const AssignmentStatementIndexInControlFlowGraph& other) const {
                bool areEqual = relativeStatementIndexPerControlBlock.size() == other.relativeStatementIndexPerControlBlock.size();
                for (std::size_t i = 0; i < relativeStatementIndexPerControlBlock.size() && areEqual; ++i) {
                    const StatementIterationHelper::StatementIndexInBlock& thisStatementIndexInBlock  = relativeStatementIndexPerControlBlock.at(i);
                    const StatementIterationHelper::StatementIndexInBlock& otherStatementIndexInBlock = other.relativeStatementIndexPerControlBlock.at(i);

                    const bool isThisStatementIndexInIfCondition  = thisStatementIndexInBlock.blockType == StatementIterationHelper::BlockType::IfConditionTrueBranch || thisStatementIndexInBlock.blockType == StatementIterationHelper::BlockType::IfConditionFalseBranch;
                    const bool isOtherStatementIndexInIfCondition = otherStatementIndexInBlock.blockType == StatementIterationHelper::BlockType::IfConditionTrueBranch || otherStatementIndexInBlock.blockType == StatementIterationHelper::BlockType::IfConditionFalseBranch;

                    areEqual &= isThisStatementIndexInIfCondition && isOtherStatementIndexInIfCondition ? getBlockTypePrecedence(thisStatementIndexInBlock.blockType) == getBlockTypePrecedence(otherStatementIndexInBlock.blockType) : true;
                    areEqual &= thisStatementIndexInBlock.relativeIndexInBlock == otherStatementIndexInBlock.relativeIndexInBlock;
                }
                return areEqual;
            }

            bool operator<(const AssignmentStatementIndexInControlFlowGraph& other) const {
                const std::size_t numElemsToCheck       = std::min(relativeStatementIndexPerControlBlock.size(), other.relativeStatementIndexPerControlBlock.size());
                bool              isCurrentEntrySmaller = false;
                for (std::size_t i = 0; i < numElemsToCheck && !isCurrentEntrySmaller; ++i) {
                    const StatementIterationHelper::StatementIndexInBlock& thisStatementIndexInBlock  = relativeStatementIndexPerControlBlock.at(i);
                    const StatementIterationHelper::StatementIndexInBlock& otherStatementIndexInBlock = other.relativeStatementIndexPerControlBlock.at(i);

                    const bool isThisStatementIndexInIfCondition  = thisStatementIndexInBlock.blockType == StatementIterationHelper::BlockType::IfConditionTrueBranch || thisStatementIndexInBlock.blockType == StatementIterationHelper::BlockType::IfConditionFalseBranch;
                    const bool isOtherStatementIndexInIfCondition = otherStatementIndexInBlock.blockType == StatementIterationHelper::BlockType::IfConditionTrueBranch || otherStatementIndexInBlock.blockType == StatementIterationHelper::BlockType::IfConditionFalseBranch;

                    isCurrentEntrySmaller |= isThisStatementIndexInIfCondition && isOtherStatementIndexInIfCondition ? getBlockTypePrecedence(thisStatementIndexInBlock.blockType) <= getBlockTypePrecedence(otherStatementIndexInBlock.blockType) : true;
                    isCurrentEntrySmaller &= relativeStatementIndexPerControlBlock.at(i).relativeIndexInBlock < other.relativeStatementIndexPerControlBlock.at(i).relativeIndexInBlock;
                }
                return isCurrentEntrySmaller;
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

        class InternalAssignmentData {
        public:
            using ptr = std::shared_ptr<InternalAssignmentData>;
            inline const static std::string NONE_ASSIGNMENT_STATEMENT_IDENT_PLACEHOLDER;

            struct SwapOperands {
                syrec::VariableAccess::ptr lhsOperand;
                syrec::VariableAccess::ptr rhsOperand;

                explicit SwapOperands(syrec::VariableAccess::ptr lhsOperand, syrec::VariableAccess::ptr rhsOperand):
                    lhsOperand(std::move(lhsOperand)), rhsOperand(std::move(rhsOperand)) {}
            };

            std::optional<std::variant<std::shared_ptr<syrec::AssignStatement>, std::shared_ptr<syrec::UnaryStatement>>> optionalReferencedAssignmentData;
            std::optional<SwapOperands>                                                                                  optionalSwapOperandsData;
            AssignmentStatementIndexInControlFlowGraph                                                                   indexInControlFlowGraph;

            InternalAssignmentData(std::variant<std::shared_ptr<syrec::AssignStatement>, std::shared_ptr<syrec::UnaryStatement>> referencedAssignment, AssignmentStatementIndexInControlFlowGraph indexInControlFlowGraph):
                optionalReferencedAssignmentData(std::move(referencedAssignment)), indexInControlFlowGraph(std::move(indexInControlFlowGraph)) {}

            InternalAssignmentData(SwapOperands swapOperandsData, AssignmentStatementIndexInControlFlowGraph indexInControlFlowGraph):
                optionalSwapOperandsData(std::move(swapOperandsData)), indexInControlFlowGraph(std::move(indexInControlFlowGraph)) {}

            InternalAssignmentData(AssignmentStatementIndexInControlFlowGraph indexInControlFlowGraph):
                indexInControlFlowGraph(std::move(indexInControlFlowGraph)) {}

            [[nodiscard]] std::optional<std::string> getAssignedToSignalPartsIdent() const {
                if (const std::optional<syrec::VariableAccess::ptr>& assignedToSignalParts = getAssignedToSignalParts(); assignedToSignalParts.has_value()) {
                    return assignedToSignalParts.value()->var->name;
                }
                return std::nullopt;
            }
            [[nodiscard]] std::optional<syrec::VariableAccess::ptr> getAssignedToSignalParts() const {
                if (!optionalReferencedAssignmentData.has_value()) {
                    return std::nullopt;
                }

                if (std::holds_alternative<std::shared_ptr<syrec::AssignStatement>>(*optionalReferencedAssignmentData)) {
                    return std::get<std::shared_ptr<syrec::AssignStatement>>(*optionalReferencedAssignmentData)->lhs;
                }
                if (std::holds_alternative<std::shared_ptr<syrec::UnaryStatement>>(*optionalReferencedAssignmentData)) {
                    return std::get<std::shared_ptr<syrec::UnaryStatement>>(*optionalReferencedAssignmentData)->var;
                }
                return std::nullopt;
            }

            [[nodiscard]] std::optional<SwapOperands> getSwapOperands() const {
                return optionalSwapOperandsData;
            }

            [[nodiscard]] std::optional<syrec::Expression::ptr> getAssignmentRhsExpr() const {
                if (!optionalReferencedAssignmentData.has_value() || !std::holds_alternative<std::shared_ptr<syrec::AssignStatement>>(*optionalReferencedAssignmentData)) {
                    return std::nullopt;
                }
                return std::get<std::shared_ptr<syrec::AssignStatement>>(*optionalReferencedAssignmentData)->rhs;
            }
        };

        std::map<std::string, std::unordered_set<InternalAssignmentData::ptr>, std::less<>> internalAssignmentData;
        std::map<std::string, std::unordered_set<InternalAssignmentData::ptr>, std::less<>> graveyard;

        [[nodiscard]] std::optional<InternalAssignmentData::ptr> getOrCreateEntryInInternalLookupForAssignment(const std::variant<std::shared_ptr<syrec::AssignStatement>, std::shared_ptr<syrec::UnaryStatement>>& referencedAssignment, const AssignmentStatementIndexInControlFlowGraph& indexOfAssignmentInControlFlowGraph);
        [[nodiscard]] std::optional<InternalAssignmentData::ptr> getOrCreateEntryInInternalLookupForSwapStatement(const InternalAssignmentData::SwapOperands& swapOperands, const AssignmentStatementIndexInControlFlowGraph& indexOfSwapStatementInControlFlowGraph);
        [[nodiscard]] std::optional<InternalAssignmentData::ptr> getEntryByIndexInControlFlowGraph(const std::string_view& assignedToSignalPartsIdent, const AssignmentStatementIndexInControlFlowGraph& indexInControlFlowGraph) const;
        void                                                     createGraveyardEntryForSkipStatement(const AssignmentStatementIndexInControlFlowGraph& indexOfSkipStatementInControlFlowGraph);
        void                                                     insertNewEntryIntoInternalLookup(const std::string_view& assignedToSignalIdent, const InternalAssignmentData::ptr& entry);
        void                                                     insertEntryIntoGraveyard(const std::string_view& assignedToSignalIdent, const InternalAssignmentData::ptr& entry);
        void                                                     removeEntryFromGraveyard(const std::string_view& assignedToSignalIdent, const InternalAssignmentData::ptr& matchingAssignmentEntry);

        void removeDataDependenciesOfAssignmentFromGraveyard(const InternalAssignmentData::ptr& internalContainerForAssignment);
        void removeOverlappingAssignmentsFromGraveyard(const syrec::VariableAccess& assignedToSignalParts, const AssignmentStatementIndexInControlFlowGraph& indexOfAssignmentWhereSignalAccessWasDefined);
        void removeOverlappingAssignmentsForSignalAccessesInExpressionFromGraveyard(const syrec::Expression::ptr& expression, const AssignmentStatementIndexInControlFlowGraph& indexOfAssignmentWhereSignalAccessWasDefined);

        [[nodiscard]] std::vector<InternalAssignmentData::ptr>                determineOverlappingAssignmentsForGivenSignalAccess(const syrec::VariableAccess& signalAccess, const AssignmentStatementIndexInControlFlowGraph& indexOfAssignmentWhereSignalAccessWasDefined) const;
        [[nodiscard]] std::vector<AssignmentStatementIndexInControlFlowGraph> determineDeadStores() const;

        std::optional<AssignmentStatementIndexInControlFlowGraph> indexOfLastProcessedStatementInControlFlowGraph;
        const parser::SymbolTable::ptr&                           symbolTable;
        std::vector<std::shared_ptr<LivenessStatusLookupScope>>   livenessStatusScopes;
        std::vector<LoopStatementInformation>                     remainingNonControlFlowStatementsPerLoopBody;

        [[nodiscard]] bool tryCreateCopyOfLivenessStatusForSignalInCurrentScope(const std::string& signalIdent);
        void               createNewLivenessStatusScope();
        void               destroyCurrentLivenessStatusScope();

        [[nodiscard]] std::vector<AssignmentStatementIndexInControlFlowGraph> findDeadStores(const syrec::Statement::vec& statementList);

        /*
         * Either create new scopes if the nesting level in comparison to the last statement increased
         * Or remove and merge scopes if the nesting level decreased
         */
        void updateLivenessStatusScopeAccordingToNestingLevelOfStatement(const std::optional<AssignmentStatementIndexInControlFlowGraph>& indexOfLastProcessedStatement, const AssignmentStatementIndexInControlFlowGraph& indexOfCurrentStatement);

        [[nodiscard]] std::optional<std::size_t> findScopeContainingEntryForSignal(const std::string& signalIdent) const;
        [[nodiscard]] bool                       isReachableInReverseControlFlowGraph(const AssignmentStatementIndexInControlFlowGraph& assignmentStatement, const AssignmentStatementIndexInControlFlowGraph& usageOfAssignedToSignal) const;
        [[nodiscard]] std::size_t                determineNestingLevelMeasuredForIfStatements(const AssignmentStatementIndexInControlFlowGraph& statementIndexInControlFlowGraph) const;

        /*
         * I.   Division with unknown divisor
         * II.  SignalAccess with either unknown accessed value of dimension or bit range
         * III. Assignment to variable with global side effect
         * IV.  Assignment to undeclared variable
         *
         */
        [[nodiscard]] bool                               doesAssignmentContainPotentiallyUnsafeOperation(const syrec::Statement::ptr& stmt) const;
        [[nodiscard]] bool                               doesExpressionContainPotentiallyUnsafeOperation(const syrec::Expression::ptr& expr) const;
        [[nodiscard]] bool                               wasSignalDeclaredAndAreAllIndizesOfSignalConstantsAndWithinRange(const syrec::VariableAccess::ptr& signalAccess) const;
        [[nodiscard]] bool                               isAssignedToSignalAModifiableParameter(const std::string_view& assignedToSignalIdent) const;
        [[nodiscard]] static std::optional<unsigned int> tryEvaluateNumber(const syrec::Number& number);
        [[nodiscard]] static std::optional<unsigned int> tryEvaluateExprToConstant(const syrec::Expression& expr);

        [[nodiscard]] bool               isNextDeadStoreInSameBranch(std::size_t currentDeadStoreIndex, const std::vector<AssignmentStatementIndexInControlFlowGraph>& foundDeadStores, std::size_t currentNestingLevel) const;
        [[nodiscard]] static std::size_t getBlockTypePrecedence(StatementIterationHelper::BlockType blockType);

        void resetInternalData();
        void removeDeadStoresFrom(syrec::Statement::vec& statementList, const std::vector<AssignmentStatementIndexInControlFlowGraph>& foundDeadStores, std::size_t& currDeadStoreIndex, std::size_t nestingLevelOfCurrentBlock) const;
        void decrementReferenceCountOfUsedSignalsInStatement(const syrec::Statement::ptr& statement) const;
        void decrementReferenceCountsOfUsedSignalsInExpression(const syrec::Expression::ptr& expr) const;
        void decrementReferenceCountForAccessedSignal(const syrec::VariableAccess::ptr& accessedSignal) const;
        void decrementReferenceCountOfNumber(const syrec::Number::ptr& number) const;

        [[nodiscard]] bool isAssignmentDefinedInLoopPerformingMoreThanOneIteration() const;
        [[nodiscard]] bool doesLoopPerformMoreThanOneIteration(const std::shared_ptr<syrec::ForStatement>& loopStmt) const;
        void               markStatementAsProcessedInLoopBody(const syrec::Statement::ptr& stmt, std::size_t nestingLevelOfStmt);
        void               addInformationAboutLoopWithMoreThanOneStatement(const std::shared_ptr<syrec::ForStatement>& loopStmt, std::size_t nestingLevelOfStmt);

        [[nodiscard]] std::vector<syrec::VariableAccess::ptr> getAccessedLocalSignalsFromExpression(const syrec::Expression::ptr& expr) const;
        [[nodiscard]] bool                                    isAccessedSignalLocalOfModule(const syrec::VariableAccess::ptr& accessedSignal) const;
        [[nodiscard]] static bool                             doesStatementListOnlyContainSingleSkipStatement(const syrec::Statement::vec& statementsToCheck);
        [[nodiscard]] static bool                             doesStatementListContainOnlySkipStatements(const syrec::Statement::vec& statementsToCheck);
        [[nodiscard]] static bool                             isNextDeadStoreInFalseBranchOfIfStatement(std::size_t currentDeadStoreIndex, const std::vector<AssignmentStatementIndexInControlFlowGraph>& foundDeadStores);
    };
} // namespace deadStoreElimination
#endif