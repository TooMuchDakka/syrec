#ifndef ASSIGNMENT_WITHOUT_ADDITIONAL_LINES_SIMPLIFIER_HPP
#define ASSIGNMENT_WITHOUT_ADDITIONAL_LINES_SIMPLIFIER_HPP
#pragma once

#include "assignment_transformer.hpp"
#include "expression_to_subassignment_splitter.hpp"
#include "core/syrec/statement.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_traversal_helper.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/temporary_assignments_container.hpp"

#include <functional>

// TODO: General overhaul of enums to scoped enums if possible (see https://en.cppreference.com/w/cpp/language/enum)
// TODO: Support for unary expressions if SyReC IR support such expressions
namespace noAdditionalLineSynthesis {
    class AssignmentWithoutAdditionalLineSimplifier {
    public:
        struct SimplificationResult {
            syrec::AssignStatement::vec generatedAssignments;
        };
        using SignalValueLookupCallback = std::function<std::optional<unsigned int>(const syrec::VariableAccess&)>;
        using SimplificationResultReference = std::unique_ptr<SimplificationResult>;
        [[nodiscard]] SimplificationResultReference simplify(const syrec::AssignStatement& assignmentStatement, const SignalValueLookupCallback& signalValueLookupCallback);

         virtual ~AssignmentWithoutAdditionalLineSimplifier() = default;
        AssignmentWithoutAdditionalLineSimplifier(const parser::SymbolTable::ptr& symbolTableReference) {
            generatedAssignmentsContainer                     = std::make_shared<TemporaryAssignmentsContainer>();
            expressionTraversalHelper                         = std::make_shared<ExpressionTraversalHelper>();
            this->symbolTableReference                        = symbolTableReference;
            expressionToSubAssignmentSplitterReference        = std::make_unique<ExpressionToSubAssignmentSplitter>();
            learnedConflictsLookup                            = std::make_unique<LearnedConflictsLookup>();
            disabledValueLookupToggle                         = false;
            assignmentTransformer                             = std::make_unique<AssignmentTransformer>();
        }

    protected:
        struct Decision {
            enum class ChoosenOperand :unsigned char {
                Left  = 0x1,
                Right = 0x2,
                None = 0x3
            };

            std::size_t            operationNodeId;
            ChoosenOperand         choosenOperand;
            std::size_t            numExistingAssignmentsPriorToDecision;
            syrec::expression::ptr backupOfExpressionRequiringFixup;
        };
        using DecisionReference = std::shared_ptr<Decision>;
        std::vector<DecisionReference>         pastDecisions;
        TemporaryAssignmentsContainer::ptr     generatedAssignmentsContainer;
        ExpressionTraversalHelper::ptr         expressionTraversalHelper;
        parser::SymbolTable::ptr               symbolTableReference;
        ExpressionToSubAssignmentSplitter::ptr expressionToSubAssignmentSplitterReference;
        bool                                   disabledValueLookupToggle;
        AssignmentTransformer::ptr             assignmentTransformer;
        
        using LearnedConflictsLookupKey = std::pair<std::size_t, Decision::ChoosenOperand>;
        struct LearnedConflictsLookupKeyHasher {
            std::size_t operator()(const LearnedConflictsLookupKey& key) const {
                return std::hash<std::size_t>()(key.first) ^ std::hash<Decision::ChoosenOperand>()(key.second);
            }
        };

        // Since the std::unordered_set does not work with std::pair out of the box, we need to provide a custom hasher function
        using LearnedConflictsLookup = std::unordered_set<LearnedConflictsLookupKey, LearnedConflictsLookupKeyHasher>;
        using LearnedConflictsLookupReference = std::unique_ptr<LearnedConflictsLookup>;
        LearnedConflictsLookupReference learnedConflictsLookup;

        std::optional<std::size_t> operationNodeCausingConflictAndBacktrack;
        class OperationOperandSimplificationResult {
        public:
            [[nodiscard]] std::optional<syrec::VariableAccess::ptr> getAssignedToSignalOfAssignment() const {
                return std::holds_alternative<syrec::VariableAccess::ptr>(data) ? std::make_optional(std::get<syrec::VariableAccess::ptr>(data)) : std::nullopt;
            }

            [[nodiscard]] std::optional<syrec::expression::ptr> getGeneratedExpr() const {
                return std::holds_alternative<syrec::expression::ptr>(data) ? std::make_optional(std::get<syrec::expression::ptr>(data)) : std::nullopt;
            }

            [[nodiscard]] std::size_t getNumberOfCreatedAssignments() const {
                return numGeneratedAssignments;
            }

            explicit OperationOperandSimplificationResult(std::size_t numGeneratedAssignments, std::variant<syrec::VariableAccess::ptr, syrec::expression::ptr> data)
                : numGeneratedAssignments(numGeneratedAssignments), data(std::move(data)) {}
        private:
            std::size_t                                                      numGeneratedAssignments;
            std::variant<syrec::VariableAccess::ptr, syrec::expression::ptr> data;
        };
        using OwningOperationOperandSimplificationResultReference = std::unique_ptr<OperationOperandSimplificationResult>;

        [[nodiscard]] std::optional<OwningOperationOperandSimplificationResultReference> handleOperationNode(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const SignalValueLookupCallback& signalValueLookupCallback);
        [[nodiscard]] std::optional<OwningOperationOperandSimplificationResultReference> handleOperationNodeWithNoLeafNodes(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const SignalValueLookupCallback& signalValueLookupCallback);
        [[nodiscard]] std::optional<OwningOperationOperandSimplificationResultReference> handleOperationNodeWithOneLeafNode(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const SignalValueLookupCallback& signalValueLookupCallback);
        [[nodiscard]] std::optional<OwningOperationOperandSimplificationResultReference> handleOperationNodeWithOnlyLeafNodes(const ExpressionTraversalHelper::OperationNodeReference& operationNode);

        // This function should probably be refactored since the usage of a signal access can only determine a conflict and not the choice when trying to create an assignments
        // Only active assignments contribute to the set of potential conflicts.
        // Determining whether a conflict exists would happen when parsing a signal access used in an expression by checking whether an active assignment for any of accessed signal parts exists.
        // If so rollback to the conflicting decision and try again now excluding the previous choice (or creating an replacement for it [i.e. the choice for the assignment if the size of the replacement can be determined])

        // Backtrack to last overlapping decision, add additional field backtrackingToNode to determine whether a conflict or a backtrack is in progress when std::nullopt is returned during handling of operation node
        // Add lookup for learned "clauses" i.e. previous conflicts that can be erased during backtracking
        [[nodiscard]] bool                             doesAssignmentToSignalLeadToConflict(const syrec::VariableAccess& assignedToSignal) const;
        [[nodiscard]] std::optional<DecisionReference> tryGetDecisionForOperationNode(const std::size_t& operationNodeId) const;
        
        [[nodiscard]] DecisionReference           makeDecision(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const std::pair<std::reference_wrapper<const OperationOperandSimplificationResult>, std::reference_wrapper<const OperationOperandSimplificationResult>>& potentialChoices);
        [[nodiscard]] std::optional<std::size_t>  determineOperationNodeIdCausingConflict(const syrec::VariableAccess& choiceOfAssignedToSignalTriggeringSearchForCauseOfConflict) const;
        [[nodiscard]] bool                        isOperationNodeSourceOfConflict(std::size_t operationNodeId) const;
        void                                      markOperationNodeAsSourceOfConflict(std::size_t operationNodeId);
        void                                      markSourceOfConflictReached();
        [[nodiscard]] bool                        shouldBacktrackDueToConflict() const;

        // This call should be responsible to determine conflicts, during a decision their should not arise any conflicts
        // The check for a conflict should take place in the operations nodes with either one or two leaf nodes by using this call before making any decisions
        // If a conflict is detected, the first decision starting from the initial one shall be our backtrack destination and all overlapping assignments for the found first decision
        // shall be recorded as learned conflicts.
        [[nodiscard]] bool                                                                                                 wereAccessedSignalPartsModifiedByActiveAssignment(const syrec::VariableAccess& accessedSignalParts) const;
        [[nodiscard]] std::vector<DecisionReference>                                                                       determineDecisionsOverlappingAccessedSignalPartsOmittingAlreadyRecordedOnes(const syrec::VariableAccess& accessedSignalParts) const;
        void                                                                                                               handleConflict(std::size_t associatedOperationNodeIdOfAccessedSignalPartsOperand, const syrec::VariableAccess& accessedSignalPartsUsedInCheckForConflict);
        [[nodiscard]] std::size_t                                                                                          determineEarliestSharedParentOperationNodeIdBetweenCurrentAndConflictOperationNodeId(std::size_t currentOperationNodeId, std::size_t conflictOperationNodeId) const;

        /**
         * \brief Determine whether another choice for a previously made decision could be made. This calls excludes the last made decision under the assumption that this call is made after a conflict was derived at the current operation node
         * \return Whether another choice at any previous decision could be made
         */
        [[nodiscard]] bool                             couldAnotherChoiceBeMadeAtPreviousDecision(const std::optional<std::size_t>& pastDecisionForOperationNodeWithIdToExclude) const;
        
        void                                                                                             removeDecisionFor(std::size_t operationNodeId);
        [[nodiscard]] std::optional<DecisionReference>                                                   tryGetLastDecision() const;
        [[nodiscard]] std::optional<DecisionReference>                                                   tryGetSecondToLastDecision() const;
        virtual void                                                                                     resetInternals();
        [[nodiscard]] virtual std::unique_ptr<syrec::AssignStatement>                                    transformAssignmentPriorToSimplification(const syrec::AssignStatement& assignmentToSimplify) const;
        void                                                                                             transformExpressionPriorToSimplification(syrec::expression& expr) const;
        void                                                                                             rememberConflict(std::size_t operationNodeId, Decision::ChoosenOperand chosenOperandAtOperationNode) const;
        [[nodiscard]] bool                                                                               didPreviousDecisionMatchingChoiceCauseConflict(const LearnedConflictsLookupKey& lookupKeyRepresentingSearchedForPreviousDecision) const;
        void                                                                                             disableValueLookup();
        void                                                                                             enabledValueLookup();
        [[nodiscard]] bool                                                                               canAlternativeByChosenInOperationNode(std::size_t operationNodeId, Decision::ChoosenOperand toBeChosenAlternative, const syrec::VariableAccess& signalAccess, const syrec::expression& alternativeToCheckAsExpr, const parser::SymbolTable& symbolTable) const;
        [[nodiscard]] bool                                                                               canAlternativeByChosenInOperationNode(std::size_t operationNodeId, Decision::ChoosenOperand toBeChosenAlternative, const syrec::VariableAccess& signalAccess, const syrec::VariableAccess& alternativeToCheckAsSignalAccess, const parser::SymbolTable& symbolTable) const;

        [[nodiscard]] static syrec::expression::ptr                      fuseExpressions(const syrec::expression::ptr& lhsOperand, syrec_operation::operation op, const syrec::expression::ptr& rhsOperand);
        [[nodiscard]] static std::optional<syrec::AssignStatement::ptr>  tryCreateAssignmentForOperationNode(const syrec::VariableAccess::ptr& assignmentLhs, syrec_operation::operation op, const syrec::expression::ptr& assignmentRhs);
        [[nodiscard]] static syrec::expression::ptr                      createExpressionFrom(const syrec::expression::ptr& lhsOperand, syrec_operation::operation op, const syrec::expression::ptr& rhsOperand);
        [[nodiscard]] static syrec::expression::ptr                      createExpressionFromOperationNode(const ExpressionTraversalHelper::ptr& expressionTraversalHelper, const ExpressionTraversalHelper::OperationNodeReference& operationNode);
        [[nodiscard]] static syrec::expression::ptr                      createExpressionFromOperandSimplificationResult(const OperationOperandSimplificationResult& operandSimplificationResult);
        [[nodiscard]] static bool                                        doesExpressionDefineNestedSplitableExpr(const syrec::expression& expr);
        [[nodiscard]] static bool                                        doesExpressionDefineNumber(const syrec::expression& expr);
        [[nodiscard]] static bool                                        doesExprDefineSignalAccess(const syrec::expression& expr);
        [[nodiscard]] static std::optional<syrec::AssignStatement::ptr>  tryCreateAssignmentFromOperands(Decision::ChoosenOperand chosenOperandAsAssignedToSignal, const OperationOperandSimplificationResult& simplificationResultOfFirstOperand, syrec_operation::operation operationNodeOperation, const OperationOperandSimplificationResult& simplificationResultOfSecondOperand);
        [[nodiscard]] static std::optional<syrec::expression::ptr>       tryCreateExpressionFromOperationNodeOperandSimplifications(const OperationOperandSimplificationResult& simplificationResultOfFirstOperand, syrec_operation::operation operationNodeOperation, const OperationOperandSimplificationResult& simplificationResultOfSecondOperand);
        [[nodiscard]] static bool                                        areAssignedToSignalPartsZero(const syrec::VariableAccess& accessedSignalParts, const SignalValueLookupCallback& signalValueLookupCallback);
        [[nodiscard]] static bool                                        doesExprOnlyDefineReversibleOperationAndXorOnlyInLeafNodes(const syrec::expression& expr);
        static void                                                      tryConvertNumericToBinaryExpr(syrec::expression::ptr& expr);
        [[nodiscard]] static std::optional<syrec::BinaryExpression::ptr> convertNumericExprToBinary(const syrec::NumericExpression& numericExpr);
        [[nodiscard]] static std::optional<syrec_operation::operation>   tryMapCompileTimeConstantExprOperationToBinaryOperation(syrec::Number::CompileTimeConstantExpression::Operation operation);
        [[nodiscard]] static syrec::expression::ptr                      convertNumberToExpr(const syrec::Number::ptr& number, unsigned int expectedBitwidth);
        [[nodiscard]] static std::optional<syrec::BinaryExpression::ptr> convertCompileTimeConstantExprToBinaryExpr(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpr, unsigned int expectedBitwidth);
        [[nodiscard]] static syrec::expression::ptr                      transformExprBeforeProcessing(const syrec::expression::ptr& initialExpr);
        
        [[nodiscard]] static bool                                        doesExprContainOverlappingAccessOnGivenSignalAccess(const syrec::expression& expr, const syrec::VariableAccess& signalAccess, const parser::SymbolTable& symbolTable);
        [[nodiscard]] static bool                                        doesNumberContainOverlappingAccessOnGivenSignalAccess(const syrec::Number& number, const syrec::VariableAccess& signalAccess, const parser::SymbolTable& symbolTable);
        [[nodiscard]] static bool                                        doSignalAccessesOverlap(const syrec::VariableAccess& firstSignalAccess, const syrec::VariableAccess& otherSignalAccess, const parser::SymbolTable& symbolTable);

        ///**
        // * \brief Check whether the simplified split of assignment right-hand side expression is allowed.
        // *
        // * \remarks This is allowed when the following conditions hold (when applicable): <br>
        // * I.    The assignment operation is NOT equal to '^=' and the rhs expr does not contain any non-reversible operations or bitwise xor operation. <br>
        // * II.   The assignment operation is equal to '^=' while the value of the assigned to signal is equal to zero and the following holds for the rhs expr: <br>
        // * II.I. The rhs expr does not contain any non-reversible operations <br>
        // * II.I. Starting from the first operation node of the expression tree when traversing the rhs expr in post-order traversal only the first operation node can define the bitwise
        // *       xor operation on an operation node with two leaf nodes, all other operation nodes defining the bitwise xor operation and reachable by the first operation can only contain
        // *       a leaf node as the "second" operand of the operation node while the "first" operand must be the expr reachable from the first operation node (when traversing the expression tree
        // *       bottom-up) <br>
        // * \param assignedToSignalParts The assigned to signal parts by the assignment
        // * \param operation The defined assignment operation
        // * \param topmostAssignmentRhsExpr The right-hand side expression of the assignment
        // * \param signalValueLookupCallback The callback used to determine the value of the assigned to signal parts
        // * \return Whether the simplified split "algorithm" can be applied.
        // */
        //[[nodiscard]] static bool isSplitOfAssignmentRhsIntoSubexpressionsAllowed(const syrec::VariableAccess& assignedToSignalParts, syrec_operation::operation operation, const syrec::expression& topmostAssignmentRhsExpr, const SignalValueLookupCallback& signalValueLookupCallback);

        ///**
        // * \brief Check whether the given \p expr does not defined any operations without a matching assignment operation counterpart. <br>
        // * \remarks This function assumes that numeric expressions that defined compile time constant expression where already converted to binary expressions
        // * \param expr The expression to check
        // * \return Whether all defined operations of the expression had a matching assignment operation counterpart defined
        // */
        //[[nodiscard]] static bool doesExprNotContainAnyOperationWithoutAssignmentCounterpart(const syrec::expression& expr);
    };
}

#endif