#ifndef POST_ORDER_EXPR_TRAVERSAL_HELPER_HPP
#define POST_ORDER_EXPR_TRAVERSAL_HELPER_HPP
#pragma once

#include "core/syrec/expression.hpp"
#include "core/syrec/parser/operation.hpp"
#include <queue>

namespace noAdditionalLineSynthesis {
    class PostOrderExprTraversalHelper {
    public:
        struct GeneratedSubAssignment {
            syrec_operation::operation assignmentOperation;
            syrec::Expression::ptr     assignmentRhsOperand;

            explicit GeneratedSubAssignment(syrec_operation::operation assignmentOperation, syrec::Expression::ptr expr):
                assignmentOperation(assignmentOperation), assignmentRhsOperand(std::move(expr)) {}
        };

        [[nodiscard]] std::optional<GeneratedSubAssignment> tryGetNextElement();
        [[nodiscard]] std::vector<GeneratedSubAssignment>   getAll();
        void                                                buildPostOrderQueue(syrec_operation::operation assignmentOperation, const syrec::Expression::ptr& expr, bool isValueOfAssignedToSignalZeroPriorToAssignment);

        PostOrderExprTraversalHelper():
            shouldCurrentOperationBeInverted(false), processedAnySubExpression(false), isInversionOfAssignmentOperationsNecessary(false), postOrderContainerIdx(0) {}

    private:
        std::queue<syrec_operation::operation> nextSubExpressionInitialAssignmentOperation;
        std::vector<GeneratedSubAssignment>    queueOfLeafNodesInPostOrder;
        bool                                   shouldCurrentOperationBeInverted;
        bool                                   processedAnySubExpression;
        bool                                   isInversionOfAssignmentOperationsNecessary;
        std::size_t                            postOrderContainerIdx;

        [[nodiscard]] static bool doesExprDefineNestedExpr(const syrec::Expression::ptr& expr);
        [[nodiscard]] static bool doesExprDefinedVariableAccess(const syrec::Expression::ptr& expr);
        [[nodiscard]] static bool canHandleAssignmentOperation(syrec_operation::operation assignmentOperation);
        [[nodiscard]] static bool trySwitchOperandsForXorOperationIfRhsIsNestedExprWithLhsBeingNotNested(syrec::Expression::ptr& lhsOperand, syrec_operation::operation operationToBeApplied, syrec::Expression::ptr& rhsOperand);
        [[nodiscard]] static bool tryTransformExpressionIfOperationShouldBeInverted(syrec::Expression::ptr& lhsOperand, syrec_operation::operation operationToBeApplied, syrec::Expression::ptr& rhsOperand, bool shouldOperationBeInverted);
        
        /*
         * \brief Perform a switch of the operands of a give operation node if: <br>
         * One of the operands is a signal access while the other is not with the additional requirement that either the current operation is commutative <br>
         * Reason for this switch is that performing such a switch of the operands could lead to the optimization that an '+=' operation is transformed to a '^=' <br>
         * \param lhsOperand The lhs operand of the binary expression in post order
         * \param operationToBeApplied The operation of the current operation node
         * \param rhsOperand The rhs operand of the binary expression in post order
         * 
         */
        static void               trySwitchSignalAccessOperandToLhs(syrec::Expression::ptr& lhsOperand, syrec_operation::operation operationToBeApplied, syrec::Expression::ptr& rhsOperand);
        static void               trySwitchNestedExprOnRhsToLhsIfLatterIsConstantOrLoopVariable(syrec::Expression::ptr& lhsOperand, syrec_operation::operation operationToBeApplied, syrec::Expression::ptr& rhsOperand);
        static void               switchOperands(syrec::Expression::ptr& lhsOperand, syrec::Expression::ptr& rhsOperand);

        [[nodiscard]] syrec_operation::operation determineFinalAssignmentOperation(syrec_operation::operation currentAssignmentOperation) const;

        void                      buildPostOrderTraversalQueueFor(const syrec::Expression::ptr& expr);
        void                      buildPostOrderTraversalQueueForSubExpr(const syrec::Expression::ptr& expr);
        void                      resetInternalData();
    };
}
#endif