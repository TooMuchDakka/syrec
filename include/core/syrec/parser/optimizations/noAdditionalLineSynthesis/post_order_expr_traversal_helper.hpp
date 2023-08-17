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
            syrec::expression::ptr     assignmentRhsOperand;

            explicit GeneratedSubAssignment(syrec_operation::operation assignmentOperation, syrec::expression::ptr expr):
                assignmentOperation(assignmentOperation), assignmentRhsOperand(std::move(expr)) {}
        };

        [[nodiscard]] std::optional<GeneratedSubAssignment> tryGetNextElement();
        [[nodiscard]] std::vector<GeneratedSubAssignment>   getAll();
        void                                                buildPostOrderQueue(syrec_operation::operation assignmentOperation, const syrec::expression::ptr expr);
    private:
        std::queue<syrec_operation::operation> nextSubExpressionInitialAssignmentOperation;
        std::vector<GeneratedSubAssignment>    queueOfLeafNodesInPostOrder;
        bool                                   shouldCurrentOperationBeInverted;
        std::size_t                            postOrderContainerIdx;


        [[nodiscard]] static bool doesExprDefineNestedExpr(const syrec::expression::ptr& expr);
        [[nodiscard]] static bool canHandleAssignmentOperation(syrec_operation::operation assignentOperation);
        void                      buildPostOrderTraversalQueueFor(const syrec::expression::ptr& expr);
        void                      buildPostOrderTraversalQueueForSubExpr(const syrec::expression::ptr& expr);
        void                      resetInternalData();
    };
}
#endif