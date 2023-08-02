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
        [[nodiscard]] syrec::Statement::vec simplifyWithoutPreconditionCheck(const syrec::AssignStatement::ptr& assignmentStmt) override;
        [[nodiscard]] bool                  simplificationPrecondition(const syrec::AssignStatement::ptr& assignmentStmt) override;
    private:
        syrec::Statement::vec globalCreatedAssignmentContainer;

        struct SimplificationScope {
            std::vector<syrec::AssignStatement::ptr>              generatedAssignments;
            std::vector<syrec::expression::ptr>                   expressionsRequiringFixup;

            /*explicit SimplificationScope(
                    std::vector<syrec::AssignStatement::ptr> generatedAssignments,
                    std::vector<std::shared_ptr<syrec::AssignStatement>> assignmentsRequiringFixup,
                    std::vector<std::shared_ptr<syrec::BinaryExpression>> expressionsRequiringFixup):
                    generatedAssignments(std::move(generatedAssignments)),
                    assignmentsRequiringFixup(std::move(assignmentsRequiringFixup)),
                    expressionsRequiringFixup(std::move(expressionsRequiringFixup)) {}*/
        };

        using SimplificationScopeReference = std::unique_ptr<SimplificationScope>;

        [[nodiscard]] SimplificationScopeReference simplifyExpressionSubtree(const std::shared_ptr<InorderOperationNodeTraversalHelper>& operationNodeTraversalHelper);
        [[nodiscard]] SimplificationScopeReference handleOperationNodeWithTwoLeafNodes(const InorderOperationNodeTraversalHelper::OperationNodeReference& operationNode, const std::shared_ptr<InorderOperationNodeTraversalHelper>& operationNodeTraversalHelper);
        [[nodiscard]] SimplificationScopeReference handleOperationNodeWithTwoNonLeafNodes(const InorderOperationNodeTraversalHelper::OperationNodeReference& operationNode, const std::shared_ptr<InorderOperationNodeTraversalHelper>& operationNodeTraversalHelper);
        [[nodiscard]] SimplificationScopeReference handleOperationNodeWithOneLeafNode(const InorderOperationNodeTraversalHelper::OperationNodeReference& operationNode, const std::shared_ptr<InorderOperationNodeTraversalHelper>& operationNodeTraversalHelper);

        //static void mergeSimplificationScopes(const SimplificationScopeReference& parentScope, const SimplificationScopeReference& childScope);
        static void                                 fuseGeneratedExpressionsOf(const SimplificationScopeReference& scope, syrec_operation::operation operation);
        static void                                 copyGeneratedAssignmentsAndExpressionsTo(const SimplificationScopeReference& copiedToScope, const SimplificationScopeReference& copiedFromScope);
        [[nodiscard]] static syrec::expression::ptr createExpressionFor(const syrec::expression::ptr& lhsOperand, syrec_operation::operation operation, const syrec::expression::ptr& rhsOperand);
        [[nodiscard]] static bool                   isMinusAndXorOperationOnlyDefinedForLeaveNodesInAST(const syrec::expression::ptr& exprToCheck);
        [[nodiscard]] static bool                   checkNoConsecutiveNonReversibleOperationsDefinedInExpressionAST(const syrec::expression::ptr& exprToCheck, const std::optional<syrec_operation::operation>& parentNodeOperation);
        [[nodiscard]] static bool                   isOperationAdditionSubtractionOrXor(syrec_operation::operation operation);
    };
}
#endif