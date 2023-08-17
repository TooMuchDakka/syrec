#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/post_order_expr_traversal_helper.hpp"

using namespace noAdditionalLineSynthesis;

std::optional<PostOrderExprTraversalHelper::GeneratedSubAssignment> PostOrderExprTraversalHelper::tryGetNextElement() {
    if (postOrderContainerIdx < queueOfLeafNodesInPostOrder.size()) {
        return std::make_optional(queueOfLeafNodesInPostOrder.at(postOrderContainerIdx++));
    }
    return std::nullopt;
}

std::vector<PostOrderExprTraversalHelper::GeneratedSubAssignment> PostOrderExprTraversalHelper::getAll() {
    return queueOfLeafNodesInPostOrder;
}

void PostOrderExprTraversalHelper::buildPostOrderQueue(syrec_operation::operation assignmentOperation, const syrec::expression::ptr expr) {
    if (!canHandleAssignmentOperation(assignmentOperation)) {
        return;
    }

    resetInternalData();
    if (assignmentOperation == syrec_operation::operation::MinusAssign) {
        nextSubExpressionInitialAssignmentOperation.emplace(*syrec_operation::invert(assignmentOperation));
        shouldCurrentOperationBeInverted = true;
    } else {
        nextSubExpressionInitialAssignmentOperation.emplace(assignmentOperation);
    }
    buildPostOrderTraversalQueueFor(expr);
}

bool PostOrderExprTraversalHelper::doesExprDefineNestedExpr(const syrec::expression::ptr& expr) {
    return std::dynamic_pointer_cast<syrec::BinaryExpression>(expr) != nullptr;
}

bool PostOrderExprTraversalHelper::canHandleAssignmentOperation(syrec_operation::operation assignentOperation) {
    switch (assignentOperation) {
        case syrec_operation::operation::AddAssign:
        case syrec_operation::operation::MinusAssign:
        case syrec_operation::operation::XorAssign:
            return true;
        default:
            return false;
    }
}

void PostOrderExprTraversalHelper::buildPostOrderTraversalQueueFor(const syrec::expression::ptr& expr) {
    if (!doesExprDefineNestedExpr(expr)) {
        const auto& assignmentOperationToUse = shouldCurrentOperationBeInverted ? *syrec_operation::invert(nextSubExpressionInitialAssignmentOperation.front()) : nextSubExpressionInitialAssignmentOperation.front();
        queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(assignmentOperationToUse, expr));
    }
    buildPostOrderTraversalQueueForSubExpr(expr);
}

void PostOrderExprTraversalHelper::buildPostOrderTraversalQueueForSubExpr(const syrec::expression::ptr& expr) {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        const bool doesLhsDefinedNestedExpr = doesExprDefineNestedExpr(exprAsBinaryExpr->lhs);
        const bool doesRhsDefinedNestedExpr = doesExprDefineNestedExpr(exprAsBinaryExpr->rhs);
        const auto mappedToOperationEnumValue = *syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);

        if (!doesLhsDefinedNestedExpr && !doesRhsDefinedNestedExpr) {
            const auto& assignmentOperationToUseForLhs = shouldCurrentOperationBeInverted ? *syrec_operation::invert(nextSubExpressionInitialAssignmentOperation.front()) : nextSubExpressionInitialAssignmentOperation.front();
            queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(assignmentOperationToUseForLhs, exprAsBinaryExpr->lhs));
            nextSubExpressionInitialAssignmentOperation.pop();

            const auto  mappedtoAssignmentOperationForCurrentOperation = *syrec_operation::getMatchingAssignmentOperationForOperation(mappedToOperationEnumValue);
            const auto& assignmentOperationToUseForRhs                 = shouldCurrentOperationBeInverted ? *syrec_operation::invert(mappedtoAssignmentOperationForCurrentOperation) : mappedtoAssignmentOperationForCurrentOperation;
            queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(assignmentOperationToUseForRhs, exprAsBinaryExpr->rhs));
        } else if (doesLhsDefinedNestedExpr && doesRhsDefinedNestedExpr) {
            buildPostOrderTraversalQueueForSubExpr(exprAsBinaryExpr->lhs);

            const auto mappedtoAssignmentOperation = *syrec_operation::getMatchingAssignmentOperationForOperation(mappedToOperationEnumValue);
            const auto& assignmentOperationToUse    = shouldCurrentOperationBeInverted ? *syrec_operation::invert(mappedtoAssignmentOperation) : mappedtoAssignmentOperation;

            // Operation nodes with the '^' operation and the rhs operand defining a nested expression cannot be further simplified and thus the whole rhs expression will be used in the generated subassignments
            if (assignmentOperationToUse == syrec_operation::operation::XorAssign && doesExprDefineNestedExpr(exprAsBinaryExpr->rhs)) {
                queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(assignmentOperationToUse, exprAsBinaryExpr->rhs));
                return;
            }
            nextSubExpressionInitialAssignmentOperation.push(assignmentOperationToUse);

            const auto currentInversionFlagValue = shouldCurrentOperationBeInverted;
            shouldCurrentOperationBeInverted     = assignmentOperationToUse == syrec_operation::operation::MinusAssign;
            buildPostOrderTraversalQueueForSubExpr(exprAsBinaryExpr->rhs);
            shouldCurrentOperationBeInverted = currentInversionFlagValue;
        } else {
            const auto& leafNode    = doesLhsDefinedNestedExpr ? exprAsBinaryExpr->rhs : exprAsBinaryExpr->lhs;
            const auto& nonLeafNode = doesLhsDefinedNestedExpr ? exprAsBinaryExpr->lhs : exprAsBinaryExpr->rhs;

            const auto& assignmentOperationToUse = shouldCurrentOperationBeInverted ? *syrec_operation::invert(nextSubExpressionInitialAssignmentOperation.front()) : nextSubExpressionInitialAssignmentOperation.front();
            nextSubExpressionInitialAssignmentOperation.pop();
            queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(assignmentOperationToUse, leafNode));

            // Operation nodes with the '^' operation and the rhs operand defining a nested expression cannot be further simplified and thus the whole rhs expression will be used in the generated subassignments
            const auto& assignmentOperationForNonLeafNode = *syrec_operation::getMatchingAssignmentOperationForOperation(mappedToOperationEnumValue);
            if (assignmentOperationForNonLeafNode == syrec_operation::operation::XorAssign && doesExprDefineNestedExpr(nonLeafNode)) {
                queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(assignmentOperationToUse, nonLeafNode));
                return;
            }

            nextSubExpressionInitialAssignmentOperation.push(assignmentOperationForNonLeafNode);
            const auto currentInversionFlagValue = shouldCurrentOperationBeInverted;
            shouldCurrentOperationBeInverted     = assignmentOperationToUse == syrec_operation::operation::MinusAssign;
            buildPostOrderTraversalQueueForSubExpr(nonLeafNode);
            shouldCurrentOperationBeInverted = currentInversionFlagValue;
        }
    }
}

void PostOrderExprTraversalHelper::resetInternalData() {
    queueOfLeafNodesInPostOrder.clear();
    postOrderContainerIdx = 0;
}