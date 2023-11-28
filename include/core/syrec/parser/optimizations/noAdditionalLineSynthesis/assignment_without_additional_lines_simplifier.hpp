#ifndef ASSIGNMENT_WITHOUT_ADDITIONAL_LINES_SIMPLIFIER_HPP
#define ASSIGNMENT_WITHOUT_ADDITIONAL_LINES_SIMPLIFIER_HPP
#pragma once

#include "expression_to_subassignment_splitter.hpp"
#include "core/syrec/statement.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_traversal_helper.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/temporary_assignments_container.hpp"

#include <functional>

// TODO: General overhaul of enums to scoped enums if possible (see https://en.cppreference.com/w/cpp/language/enum)

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

        [[nodiscard]] std::optional<OwningOperationOperandSimplificationResultReference> handleOperationNode(const ExpressionTraversalHelper::OperationNodeReference& operationNode);
        [[nodiscard]] std::optional<OwningOperationOperandSimplificationResultReference> handleOperationNodeWithNoLeafNodes(const ExpressionTraversalHelper::OperationNodeReference& operationNode);
        [[nodiscard]] std::optional<OwningOperationOperandSimplificationResultReference> handleOperationNodeWithOneLeafNode(const ExpressionTraversalHelper::OperationNodeReference& operationNode);
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

        /**
         * \brief Try to replace assignments of the from  a += ... with a ^= ... if the value of the symbol table entry for 'a' had the value 0.
         * \remark
         * We are assuming that, constant propagation did already perform any substitutions of accessed signals on the rhs of every assignment thus the values of all remaining signal accesses
         * on the rhs of any assignment are assumed to be unknown. Furthermore, for any signal access to be chosen as the assigned to signal in an assignment for any subexpression of the original assignment
         * must have the signal type of either 'wire', 'out' or 'inout' and since constant propagation did not substitute the accessed signal parts implies that they must be not zero.
         * \remark
         * We are processing the given assignment front to back and are assuming that the ordering of the assignment inside of the vector is based on the "insertion" time thus assignments generated further down the expression
         * tree are found at smaller indices in the vector. After an assignment of the form (referred to as assignment 1) a += ... was transformed to a ^= ..., the next application of the same transformation can only happen at the next assignment of the form a += ...
         * if either the rhs of the assignment 1 was 0 or the matching inversion of assignment 1 took place.
         *
         * \param generatedAssignments The assignments to transform
         * \param signalValueLookupCallback The callback used to determine the value of a given signal access
         */
        void               tryReplaceAddWithXorAsAssignmentOperationIfAssignedToSignalIsZero(const syrec::AssignStatement::vec& generatedAssignments, const SignalValueLookupCallback& signalValueLookupCallback) const;
        
        /**
         * \brief Try to simplify the rhs of an assignment by splitting it into sub-assignments if possible. We are assuming that no signal accesses overlapping the lhs operand of the assignment is defined on the rhs.
         * \param assignment The assignment which should be simplified.
         * \return The generated sub-assignments or a vector containing the original assignment
         */
        [[nodiscard]] syrec::AssignStatement::vec performSimplificationOfAssignmentRhs(const syrec::AssignStatement::ptr& assignment) const;

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
        
        [[nodiscard]] static syrec::expression::ptr                                                             fuseExpressions(const syrec::expression::ptr& lhsOperand, syrec_operation::operation op, const syrec::expression::ptr& rhsOperand);
        [[nodiscard]] static std::optional<syrec::AssignStatement::ptr>                                         tryCreateAssignmentForOperationNode(const syrec::VariableAccess::ptr& assignmentLhs, syrec_operation::operation op, const syrec::expression::ptr& assignmentRhs);
        [[nodiscard]] static syrec::expression::ptr                                                             createExpressionFrom(const syrec::expression::ptr& lhsOperand, syrec_operation::operation op, const syrec::expression::ptr& rhsOperand);
        [[nodiscard]] static syrec::expression::ptr                                                             createExpressionFromOperationNode(const ExpressionTraversalHelper::ptr& expressionTraversalHelper, const ExpressionTraversalHelper::OperationNodeReference& operationNode);
        [[nodiscard]] static syrec::expression::ptr                                                             createExpressionFromOperandSimplificationResult(const OperationOperandSimplificationResult& operandSimplificationResult);
        [[nodiscard]] static bool                                                                               doesExpressionDefineNestedSplitableExpr(const syrec::expression& expr);
        [[nodiscard]] static bool                                                                               doesExpressionDefineNumber(const syrec::expression& expr);
        [[nodiscard]] static bool                                                                               doesExprDefineSignalAccess(const syrec::expression& expr);
        [[nodiscard]] static std::optional<syrec::AssignStatement::ptr>                                         tryCreateAssignmentFromOperands(Decision::ChoosenOperand chosenOperandAsAssignedToSignal, const OperationOperandSimplificationResult& simplificationResultOfFirstOperand, syrec_operation::operation operationNodeOperation, const OperationOperandSimplificationResult& simplificationResultOfSecondOperand);
        [[nodiscard]] static std::optional<syrec::expression::ptr>                                              tryCreateExpressionFromOperationNodeOperandSimplifications(const OperationOperandSimplificationResult& simplificationResultOfFirstOperand, syrec_operation::operation operationNodeOperation, const OperationOperandSimplificationResult& simplificationResultOfSecondOperand);
        [[nodiscard]] static bool                                                                               areAssignedToSignalPartsZero(const syrec::VariableAccess& accessedSignalParts, const SignalValueLookupCallback& signalValueLookupCallback);
    };
}

#endif