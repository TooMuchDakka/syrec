#ifndef ASSIGNMENT_WITH_NONE_REVERSIBLE_OPERATIONS_AND_UNIQUE_SIGNAL_OCCURRENCES_SIMPLIFIER_HPP
#define ASSIGNMENT_WITH_NONE_REVERSIBLE_OPERATIONS_AND_UNIQUE_SIGNAL_OCCURRENCES_SIMPLIFIER_HPP
#pragma once

#include "inorder_operation_node_traversal_helper.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/base_assignment_simplifier.hpp"

/*
 * TODO: Can the simplifcation approach be reworked from a top-down to a bottom up traversal (and hopefully simplify the required logic of this class)
 */
namespace noAdditionalLineSynthesis {
    class AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier : public BaseAssignmentSimplifier {
    public:
        explicit AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier(parser::SymbolTable::ptr symbolTable)
            : BaseAssignmentSimplifier(std::move(symbolTable)) {}

        ~AssignmentWithNonReversibleOperationsAndUniqueSignalOccurrencesSimplifier() override = default;

    protected:
        [[nodiscard]] syrec::Statement::vec simplifyWithoutPreconditionCheck(const syrec::AssignStatement::ptr& assignmentStmt, bool isValueOfAssignedToSignalBlockedByDataFlowAnalysis) override;
        [[nodiscard]] bool                  simplificationPrecondition(const syrec::AssignStatement::ptr& assignmentStmt) override;
    private:
        syrec::Statement::vec globalCreatedAssignmentContainer;

        struct SimplificationScope {
            std::vector<syrec::AssignStatement::ptr>              generatedAssignments;
            std::vector<syrec::Expression::ptr>                   expressionsRequiringFixup;
        };

        using SimplificationScopeReference = std::unique_ptr<SimplificationScope>;

        [[nodiscard]] SimplificationScopeReference simplifyExpressionSubtree(const std::shared_ptr<InorderOperationNodeTraversalHelper>& operationNodeTraversalHelper);
        [[nodiscard]] SimplificationScopeReference handleOperationNodeWithTwoLeafNodes(const InorderOperationNodeTraversalHelper::OperationNodeReference& operationNode, const std::shared_ptr<InorderOperationNodeTraversalHelper>& operationNodeTraversalHelper);
        [[nodiscard]] SimplificationScopeReference handleOperationNodeWithTwoNonLeafNodes(const InorderOperationNodeTraversalHelper::OperationNodeReference& operationNode, const std::shared_ptr<InorderOperationNodeTraversalHelper>& operationNodeTraversalHelper);
        [[nodiscard]] SimplificationScopeReference handleOperationNodeWithOneLeafNode(const InorderOperationNodeTraversalHelper::OperationNodeReference& operationNode, const std::shared_ptr<InorderOperationNodeTraversalHelper>& operationNodeTraversalHelper);
        [[nodiscard]] syrec::Statement::vec        createFinalAssignmentFromOptimizedRhsOfInitialAssignment(const syrec::VariableAccess::ptr& initialAssignmentLhs, unsigned int initialAssignmentOperation, const SimplificationScopeReference& optimizedRhsOfInitialAssignment, bool isValueOfAssignedToSignalBlockedByDataFlowAnalysis) const;

        //static void mergeSimplificationScopes(const SimplificationScopeReference& parentScope, const SimplificationScopeReference& childScope);
        static void                                 fuseGeneratedExpressionsOf(const SimplificationScopeReference& scope, syrec_operation::operation operation);
        static void                                 copyGeneratedAssignmentsAndExpressionsTo(const SimplificationScopeReference& copiedToScope, const SimplificationScopeReference& copiedFromScope);
        [[nodiscard]] static syrec::Expression::ptr createExpressionFor(const syrec::Expression::ptr& lhsOperand, syrec_operation::operation operation, const syrec::Expression::ptr& rhsOperand);
        [[nodiscard]] static bool                   isMinusAndXorOperationOnlyDefinedForLeaveNodesInAST(const syrec::Expression::ptr& exprToCheck);
        [[nodiscard]] static bool                   checkNoConsecutiveNonReversibleOperationsDefinedInExpressionAST(const syrec::Expression::ptr& exprToCheck, const std::optional<syrec_operation::operation>& parentNodeOperation);
        [[nodiscard]] static bool                   isOperationAdditionSubtractionOrXor(syrec_operation::operation operation);

        struct SplitAssignmentExprPart {
            syrec_operation::operation mappedToAssignmentOperation;
            syrec::Expression::ptr     assignmentRhsOperand;

            explicit SplitAssignmentExprPart(syrec_operation::operation mappedToAssignmentOperation, syrec::Expression::ptr assignmentRhsOperand):
                mappedToAssignmentOperation(mappedToAssignmentOperation), assignmentRhsOperand(std::move(assignmentRhsOperand)) {}
        };
        using SplitAssignmentExprPartReference = std::shared_ptr<SplitAssignmentExprPart>;

        [[nodiscard]] static std::vector<SplitAssignmentExprPartReference> splitAssignmentRhs(syrec_operation::operation originalAssignmentOperation, const syrec::BinaryExpression::ptr& assignmentRhsBinaryExpr, bool& isAssignedToSignalZeroPriorToAssignment);
        [[nodiscard]] static std::vector<SplitAssignmentExprPartReference> splitAssignmentSubexpression(syrec_operation::operation parentBinaryExprOperation, const syrec::Expression::ptr& subExpr);
        [[nodiscard]] static syrec::AssignStatement::vec                   createAssignmentsForSplitAssignment(const syrec::VariableAccess::ptr& assignedToSignalParts, const std::vector<SplitAssignmentExprPartReference>& splitUpAssignmentRhsParts);
    };
}
#endif