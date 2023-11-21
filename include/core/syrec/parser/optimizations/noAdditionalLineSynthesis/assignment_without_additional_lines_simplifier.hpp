#ifndef ASSIGNMENT_WITHOUT_ADDITIONAL_LINES_SIMPLIFIER_HPP
#define ASSIGNMENT_WITHOUT_ADDITIONAL_LINES_SIMPLIFIER_HPP
#pragma once

#include "core/syrec/statement.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_traversal_helper.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/temporary_assignments_container.hpp"

// TODO: General overhaul of enums to scoped enums if possible (see https://en.cppreference.com/w/cpp/language/enum)

namespace noAdditionalLineSynthesis {
    class AssignmentWithoutAdditionalLineSimplifier {
    public:
        struct SimplificationResult {
            syrec::AssignStatement::vec generatedAssignments;
        };
        using SimplificationResultReference = std::unique_ptr<SimplificationResult>;
        [[nodiscard]] SimplificationResultReference simplify(const syrec::AssignStatement& assignmentStatement, const syrec::AssignStatement::vec& assignmentsDefiningSignalPartsBlockedFromEvaluation);

         virtual ~AssignmentWithoutAdditionalLineSimplifier() = default;
        AssignmentWithoutAdditionalLineSimplifier(const parser::SymbolTable::ptr& symbolTableReference) {
            generatedAssignmentsContainer = std::make_shared<TemporaryAssignmentsContainer>();
            expressionTraversalHelper     = std::make_shared<ExpressionTraversalHelper>();
            this->symbolTableReference    = symbolTableReference;
            signalPartsBlockedFromEvaluationLookup = std::make_unique<SignalPartsBlockedFromEvaluationLookup>();
        }

    protected:
        struct Decision {
            enum class ChoosenOperand :unsigned char {
                Left  = 0x1,
                Right = 0x2,
                None = 0x3
            };

            friend constexpr ChoosenOperand operator| (const ChoosenOperand first, const ChoosenOperand second) {
                return static_cast<ChoosenOperand>(static_cast<unsigned char>(first) | static_cast<unsigned char>(second));
            }

            constexpr bool wasOperandAlreadyChoosen(const ChoosenOperand aggregateOfPreviousDecisions, const ChoosenOperand operandToCheck) {
                return static_cast<bool>(static_cast<unsigned char>(aggregateOfPreviousDecisions) & static_cast<unsigned char>(operandToCheck));
            }

            std::size_t            operationNodeId;
            ChoosenOperand         choosenOperand;
            std::size_t            numExistingAssignmentsPriorToDecision;
            syrec::expression::ptr backupOfExpressionRequiringFixup;
        };
        using DecisionReference = std::shared_ptr<Decision>;
        std::vector<DecisionReference>     pastDecisions;
        TemporaryAssignmentsContainer::ptr generatedAssignmentsContainer;
        ExpressionTraversalHelper::ptr     expressionTraversalHelper;
        parser::SymbolTable::ptr           symbolTableReference;

        using SignalPartsBlockedFromEvaluationLookup = std::map<std::string, std::vector<syrec::VariableAccess::ptr>, std::less<>>;
        using SignalPartsBlockedFromEvaluationLookupReference = std::unique_ptr<SignalPartsBlockedFromEvaluationLookup>;
        SignalPartsBlockedFromEvaluationLookupReference signalPartsBlockedFromEvaluationLookup;

        class OperationOperandSimplificationResult {
        public:
            enum class SimplificationMergeResult : unsigned char {
                OnlyExpression = 0x1,
                OnlyAssignment = 0x2,
                AssignmentAndExpression = 0x3
            };

            friend constexpr SimplificationMergeResult operator|(const SimplificationMergeResult first, const SimplificationMergeResult second) {
                return static_cast<SimplificationMergeResult>(static_cast<unsigned char>(first) | static_cast<unsigned char>(second));
            }

            [[nodiscard]] std::optional<syrec::VariableAccess::ptr> getAssignedToSignalOfAssignment() const {
                return std::holds_alternative<syrec::VariableAccess::ptr>(data) ? std::make_optional(std::get<syrec::VariableAccess::ptr>(data)) : std::nullopt;
            }

            [[nodiscard]] std::optional<syrec::expression::ptr> getGeneratedExpr() const {
                return std::holds_alternative<syrec::expression::ptr>(data) ? std::make_optional(std::get<syrec::expression::ptr>(data)) : std::nullopt;
            }

            [[nodiscard]] std::size_t getNumberOfCreatedAssignments() const {
                return numGeneratedAssignments;
            }

            explicit OperationOperandSimplificationResult(std::size_t numGeneratedAssignments, const std::variant<syrec::VariableAccess::ptr, syrec::expression::ptr> data)
                : numGeneratedAssignments(numGeneratedAssignments), data(data) {}
        private:
            std::size_t numGeneratedAssignments;
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
        [[nodiscard]] std::optional<unsigned int>      tryFetchValueOfSignal(const syrec::VariableAccess& assignedToSignal) const;
        [[nodiscard]] std::optional<DecisionReference> tryGetDecisionForOperationNode(const std::size_t& operationNodeId) const;
        [[nodiscard]] bool                             isValueEvaluationOfAccessedSignalPartsBlocked(const syrec::VariableAccess& accessedSignalParts) const;
        [[nodiscard]] DecisionReference                makeDecision(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const std::pair<std::reference_wrapper<const OperationOperandSimplificationResult>, std::reference_wrapper<const OperationOperandSimplificationResult>>& potentialChoices, bool& didConflictPreventChoice);

        /**
         * \brief Determine whether another choice for a previously made decision could be made. This calls excludes the last made decision under the assumption that this call is made after a conflict was derived at the current operation node
         * \return Whether another choice at any previous decision could be made
         */
        [[nodiscard]] bool                             couldAnotherChoiceBeMadeAtPreviousDecision(const std::optional<std::size_t>& pastDecisionForOperationNodeWithIdToExclude) const;
        
        void                                                                                             removeDecisionFor(std::size_t operationNodeId);
        [[nodiscard]] std::optional<DecisionReference>                                                   tryGetLastDecision() const;
        [[nodiscard]] std::optional<DecisionReference>                                                   tryGetSecondToLastDecision() const;
        void                                                                                             backtrackToLastDecision();
        virtual void                                                                                     resetInternals();
        [[nodiscard]] virtual std::unique_ptr<syrec::AssignStatement>                                    transformAssignmentPriorToSimplification(const syrec::AssignStatement& assignmentToSimplify) const;
        void                                                                                             transformExpressionPriorToSimplification(syrec::expression& expr) const;
        [[nodiscard]] std::optional<std::pair<syrec::AssignStatement::ptr, syrec::AssignStatement::ptr>> trySplitAssignmentRhs(const syrec::VariableAccess::ptr& assignedToSignal, syrec_operation::operation assignmentOperation, const syrec::expression::ptr& assignmentRhsExpr) const;

        [[nodiscard]] static syrec::expression::ptr                                                             fuseExpressions(const syrec::expression::ptr& lhsOperand, syrec_operation::operation op, const syrec::expression::ptr& rhsOperand);
        [[nodiscard]] static std::optional<syrec::AssignStatement::ptr>                                         tryCreateAssignmentForOperationNode(const syrec::VariableAccess::ptr& assignmentLhs, syrec_operation::operation op, const syrec::expression::ptr& assignmentRhs);
        [[nodiscard]] static syrec::expression::ptr                                                             createExpressionFrom(const syrec::expression::ptr& lhsOperand, syrec_operation::operation op, const syrec::expression::ptr& rhsOperand);
        [[nodiscard]] static syrec::expression::ptr                                                             createExpressionFromOperationNode(const ExpressionTraversalHelper::ptr& expressionTraversalHelper, const ExpressionTraversalHelper::OperationNodeReference& operationNode);
        [[nodiscard]] static syrec::expression::ptr                                                             createExpressionFromOperandSimplificationResult(const OperationOperandSimplificationResult& operandSimplificationResult);
        [[nodiscard]] static bool                                                                               doesExpressionDefineNestedSplitableExpr(const syrec::expression& expr);
        [[nodiscard]] static constexpr unsigned int                                                             combineAssignmentAndBinaryOperation(syrec_operation::operation assignmentOperation, syrec_operation::operation binaryOperation) {
            return mapRelevantOperationsToNumericValue(assignmentOperation) + mapRelevantOperationsToNumericValue(binaryOperation);
        }
        [[nodiscard]] static constexpr unsigned int                                                             mapRelevantOperationsToNumericValue(syrec_operation::operation operation) {
            constexpr unsigned int baseValue = 1;
            switch (operation) {
                case syrec_operation::operation::AddAssign:
                    return baseValue << 0;
                case syrec_operation::operation::Addition:
                    return baseValue << 1;
                case syrec_operation::operation::MinusAssign:
                    return baseValue << 2;
                case syrec_operation::operation::Subtraction:
                    return baseValue << 3;
                case syrec_operation::operation::XorAssign:
                    return baseValue << 4;
                case syrec_operation::operation::BitwiseXor:
                    return baseValue << 5;
                default:
                    return 0;
            }
        }

        [[nodiscard]] static std::optional<syrec::AssignStatement::ptr> tryCreateAssignmentFromOperands(Decision::ChoosenOperand chosenOperandAsAssignedToSignal, const OperationOperandSimplificationResult& simplificationResultOfFirstOperand, syrec_operation::operation operationNodeOperation, const OperationOperandSimplificationResult& simplificationResultOfSecondOperand);
        [[nodiscard]] static std::optional<syrec::expression::ptr>      tryCreateExpressionFromOperationNodeOperandSimplifications(const OperationOperandSimplificationResult& simplificationResultOfFirstOperand, syrec_operation::operation operationNodeOperation, const OperationOperandSimplificationResult& simplificationResultOfSecondOperand);

        /**
         * \brief Build the lookup defining which signal parts are blocked from value evaluation
         *
         * - Since heterogeneous lookup for containers was added in the C++ 2as working draft (https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0919r2.html) but C++ 17 is used in this project, we need to "fallback" to ordered
         * containers that allow the usage of heterogeneous lookup that was added with C++ 14 (https://stackoverflow.com/a/35525806)
         *
         * \param assignmentsDefiningSignalPartsBlockedFromEvaluation The assignments defining the signal parts blocked from value evaluation (i.e. the assigned to signal on the left hand side of the assignment)
         * \param symbolTableReference The symbol table usable to evaluate any non-constant expressions used to define the assigned to signal of an assignment statement
         * \return A lookup container used to define the signal parts blocked from value lookup
         */
        [[nodiscard]] static SignalPartsBlockedFromEvaluationLookupReference buildLookupOfSignalsPartsBlockedFromEvaluation(const syrec::AssignStatement::vec& assignmentsDefiningSignalPartsBlockedFromEvaluation, const parser::SymbolTable& symbolTableReference);
    };
}

#endif