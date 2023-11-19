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
        [[nodiscard]] SimplificationResultReference simplify(const syrec::AssignStatement::ptr& assignmentStatement, const syrec::AssignStatement::vec& assignmentsDefiningSignalPartsBlockedFromEvaluation);

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

        [[nodiscard]] bool                             doesAssignmentToSignalLeadToConflict(const syrec::VariableAccess& assignedToSignal) const;
        [[nodiscard]] std::optional<unsigned int>      tryFetchValueOfSignal(const syrec::VariableAccess& assignedToSignal) const;
        [[nodiscard]] std::optional<DecisionReference> tryGetDecisionForOperationNode(const std::size_t& operationNodeId) const;
        [[nodiscard]] bool                             isValueEvaluationOfAccessedSignalPartsBlocked(const syrec::VariableAccess& accessedSignalParts) const;

        ///**
        // * \brief Determine for an operation node with two leaf nodes whether and which leaf to pick.
        // *
        // * - If an existing decision for the given operation node already exists and a leaf node was selected by the previous decision, try to either choose
        // * the other leaf node or if the right operand was already chosen by the last decision and thus all options exhausted, opt to instead select the whole expression.
        // * - If no decision for the given decision exists, try to select the lhs operand.
        // * 
        // * \param operationNode The operation node on which the decision should be made
        // * \return If any leaf node is selected, return whether the lhs operand was selected
        // */
        //[[nodiscard]] virtual Decision::ChoosenOperand chooseBetweenPotentialOperands(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const std::optional<Decision::ChoosenOperand>& previouslyChosenOperand) const;

        [[nodiscard]] DecisionReference                makeDecision(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const std::pair<std::reference_wrapper<const OperationOperandSimplificationResult>, std::reference_wrapper<const OperationOperandSimplificationResult>>& potentialChoices, bool& didConflictPreventChoice);

        /**
         * \brief Determine whether another choice for a previously made decision could be made. This calls excludes the last made decision under the assumption that this call is made after a conflict was derived at the current operation node
         * \return Whether another choice at any previous decision could be made
         */
        [[nodiscard]] bool                             couldAnotherChoiceBeMadeAtPreviousDecision() const;
        //[[maybe_unused]] bool                          redecide(const ExpressionTraversalHelper::OperationNodeReference& operationNode, const DecisionReference& previousDecisionForOperationNode) const;

        void                                           removeDecisionFor(std::size_t operationNodeId);
        [[nodiscard]] std::optional<DecisionReference> tryGetLastDecision() const;
        [[nodiscard]] std::optional<DecisionReference> tryGetSecondToLastDecision() const;
        void                                           backtrackToLastDecision();
        virtual void                                   resetInternals();

        [[nodiscard]] static syrec::expression::ptr                              fuseExpressions(const syrec::expression::ptr& lhsOperand, syrec_operation::operation op, const syrec::expression::ptr& rhsOperand);
        [[nodiscard]] static std::optional<syrec::AssignStatement::ptr>          tryCreateAssignmentForOperationNode(const syrec::VariableAccess::ptr& assignmentLhs, syrec_operation::operation op, const syrec::expression::ptr& assignmentRhs);
        [[nodiscard]] static syrec::expression::ptr                              createExpressionFrom(const syrec::expression::ptr& lhsOperand, syrec_operation::operation op, const syrec::expression::ptr& rhsOperand);
        [[nodiscard]] static syrec::expression::ptr                              createExpressionFromOperationNode(const ExpressionTraversalHelper::ptr& expressionTraversalHelper, const ExpressionTraversalHelper::OperationNodeReference& operationNode);
        [[nodiscard]] static syrec::expression::ptr                              createExpressionFromOperandSimplificationResult(const OperationOperandSimplificationResult& operandSimplificationResult);
        [[nodiscard]] static bool                                                doesExpressionDefineNestedSplitableExpr(const syrec::expression& expr);

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


//#ifndef ASSIGNMENT_WITHOUT_ADDITIONAL_LINES_SIMPLIFIER_HPP
//#define ASSIGNMENT_WITHOUT_ADDITIONAL_LINES_SIMPLIFIER_HPP
//#pragma once
//
//#include "expression_traversal_helper.hpp"
//#include "core/syrec/parser/symbol_table.hpp"
//
//#include <unordered_map>
//
//namespace noAdditionalLineSynthesis {
//    class AssignmentWithoutAdditionalLinesSimplifier {
//    public:
//        using DataflowAnalysisResultLookup = std::unordered_map<std::string, std::vector<syrec::VariableAccess::ptr>>;
//        using DataflowAnalysisResultLookupReference = std::shared_ptr<DataflowAnalysisResultLookup>;
//
//        [[nodiscars]] std::vector<syrec::AssignStatement::ptr> simplify(const syrec::AssignStatement& assignmentStmtToSimplify, const DataflowAnalysisResultLookupReference& blockedAssignmentsLookup);
//
//        explicit AssignmentWithoutAdditionalLinesSimplifier(parser::SymbolTable::ptr symbolTableReference, ExpressionTraversalHelper::ptr expressionTraversalHelper):
//            symbolTable(std::move(symbolTableReference)), expressionTraversalHelper(std::move(expressionTraversalHelper)) {}
//
//        virtual ~AssignmentWithoutAdditionalLinesSimplifier() = 0;
//    protected:
//        using LocallyBlockedSignalMap = std::unordered_map<std::string, std::vector<syrec::VariableAccess::ptr>>;
//        using LocallyBlockedSignalMapReference = std::unique_ptr<LocallyBlockedSignalMap>;
//
//        const parser::SymbolTable::ptr        symbolTable;
//        DataflowAnalysisResultLookupReference valueLookupsBlockedByDataflowAnalysis;
//        LocallyBlockedSignalMapReference      locallyBlockedSignals;
//        ExpressionTraversalHelper::ptr        expressionTraversalHelper;
//        syrec::AssignStatement::vec           generatedAssignments;
//
//        struct ExpressionSimplificationResult {
//            /**
//             * \brief Expression requiring fixup by either fusing them together or using the as the rhs operand in an assignment. This vector should contain at most two elements.
//             */
//            syrec::expression::vec      expressionsRequiringFixup;
//
//            void fuseGeneratedExpressions(syrec_operation::operation operationOfFusedExpr);
//            void copySimplificationResult(const ExpressionSimplificationResult& other);
//        };
//        using ExpressionSimplificationResultReference = std::unique_ptr<ExpressionSimplificationResult>;
//
//        struct Decision {
//            //std::size_t            operationNodeId;
//            /**
//             * \brief Used to determine whether the current decision choose the lhs operand. If no choice was made (indicated by std::nullopt), all previous possibility were exhausted.
//             */
//            std::optional<bool>    chooseLhsOperand;
//            std::size_t            numberOfExistingAssignments;
//            syrec::expression::vec currentExpressionsRequiringFixup;
//        };
//        std::vector<Decision> madeDecisions;
//
//        [[nodiscard]] ExpressionSimplificationResultReference handleOperationNode(const ExpressionTraversalHelper::OperationNodeReference& operationNode);
//        [[nodiscard]] ExpressionSimplificationResultReference handleOperationNodeWithTwoLeafNodes(const ExpressionTraversalHelper::OperationNodeReference& operationNodeWithTwoLeafNodes);
//        [[nodiscard]] ExpressionSimplificationResultReference handleOperationNodeWithOneLeafNode(const ExpressionTraversalHelper::OperationNodeReference& operationNodeWithOneLeafNode);
//        [[nodiscard]] ExpressionSimplificationResultReference handleOperationNodeWithNoLeafNodes(const ExpressionTraversalHelper::OperationNodeReference& operationNodeWithoutLeafNodes);
//        [[nodiscard]] virtual bool                            checkPrecondition(const syrec::AssignStatement& assignmentStatement) const;
//
//        /**
//         * \brief Determine for an operation node with two leaf nodes whether and which leaf to pick.
//         *
//         * - If an existing decision for the given operation node already exists and a leaf node was selected by the previous decision, try to either choose
//         * the other leaf node or if the right operand was already chosen by the last decision and thus all options exhausted, opt to instead select the whole expression.
//         * - If no decision for the given decision exists, try to select the lhs operand.
//         * 
//         * \param operationNode The operation node on which the decision should be made
//         * \return If any leaf node is selected, return whether the lhs operand was selected
//         */
//        [[nodiscard]] virtual std::optional<bool> chooseBetweenPotentialOperands(const ExpressionTraversalHelper::OperationNodeReference& operationNode);
//        virtual void                              makeDecision(const ExpressionTraversalHelper::OperationNodeReference& operationNode, syrec::expression::vec& currentExpressionsRequiringFixup);
//
//        /**
//         * \brief Discard the generated assignments since the last decision and reset to the operation node of the last decision
//         */
//        virtual void                              rollbackToLastDecision();
//
//        [[nodiscard]] std::optional<unsigned int>   determineValueOfAssignedToSignal(const syrec::VariableAccess& assignedToSignal) const;
//        [[nodiscard]] static bool                   isOriginallyAssignedToSignalAccessNotUsedInRhs(const syrec::VariableAccess& originalAssignedToSignal, const syrec::expression& originalAssignmentRhs, const parser::SymbolTable& symbolTable);
//        [[nodiscard]] static syrec::expression::ptr createExpressionFor(const syrec::expression::ptr& lhsOperand, syrec_operation::operation operation, const syrec::expression::ptr& rhsOperand);
//        [[nodiscard]] static bool                   existsMappingForOperationOfExpr(const syrec::expression& expr);
//        virtual void                                resetInternals();
//    };
//}
//#endif