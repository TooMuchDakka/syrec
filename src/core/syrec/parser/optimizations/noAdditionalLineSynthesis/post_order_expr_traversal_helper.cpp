#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/post_order_expr_traversal_helper.hpp"

using namespace noAdditionalLineSynthesis;

/*
    Could perform split of top most expression at '^' if one operand is constant, i.e. a += (2 ^ <subExpr>) = a ^= <subExpr>; a ^= 2 iff a = 0
*/
std::optional<PostOrderExprTraversalHelper::GeneratedSubAssignment> PostOrderExprTraversalHelper::tryGetNextElement() {
    if (postOrderContainerIdx < queueOfLeafNodesInPostOrder.size()) {
        return std::make_optional(queueOfLeafNodesInPostOrder.at(postOrderContainerIdx++));
    }
    return std::nullopt;
}

std::vector<PostOrderExprTraversalHelper::GeneratedSubAssignment> PostOrderExprTraversalHelper::getAll() {
    return queueOfLeafNodesInPostOrder;
}

void PostOrderExprTraversalHelper::buildPostOrderQueue(syrec_operation::operation assignmentOperation, const syrec::Expression::ptr& expr, bool isValueOfAssignedToSignalZeroPriorToAssignment) {
    if (!canHandleAssignmentOperation(assignmentOperation) || (!isValueOfAssignedToSignalZeroPriorToAssignment && assignmentOperation == syrec_operation::operation::XorAssign)) {
        return;
    }

    resetInternalData();
    nextSubExpressionInitialAssignmentOperation.emplace(assignmentOperation);
    shouldCurrentOperationBeInverted = assignmentOperation == syrec_operation::operation::MinusAssign;
    isInversionOfAssignmentOperationsNecessary = shouldCurrentOperationBeInverted;
    buildPostOrderTraversalQueueFor(expr);
}

bool PostOrderExprTraversalHelper::doesExprDefineNestedExpr(const syrec::Expression::ptr& expr) {
    return std::dynamic_pointer_cast<syrec::BinaryExpression>(expr) != nullptr;
}

bool PostOrderExprTraversalHelper::doesExprDefinedVariableAccess(const syrec::Expression::ptr& expr) {
    return std::dynamic_pointer_cast<syrec::VariableExpression>(expr) != nullptr;
}

bool PostOrderExprTraversalHelper::canHandleAssignmentOperation(syrec_operation::operation assignmentOperation) {
    switch (assignmentOperation) {
        case syrec_operation::operation::AddAssign:
        case syrec_operation::operation::MinusAssign:
        case syrec_operation::operation::XorAssign:
            return true;
        default:
            return false;
    }
}

void PostOrderExprTraversalHelper::trySwitchSignalAccessOperandToLhs(syrec::Expression::ptr& lhsOperand, syrec_operation::operation operationToBeApplied, syrec::Expression::ptr& rhsOperand) {
    if (doesExprDefineNestedExpr(lhsOperand) || doesExprDefinedVariableAccess(lhsOperand) || !doesExprDefinedVariableAccess(rhsOperand) || !syrec_operation::isCommutative(operationToBeApplied)) {
        return;
    }
    switchOperands(lhsOperand, rhsOperand);
}

void PostOrderExprTraversalHelper::trySwitchNestedExprOnRhsToLhsIfLatterIsConstantOrLoopVariable(syrec::Expression::ptr& lhsOperand, syrec_operation::operation operationToBeApplied, syrec::Expression::ptr& rhsOperand) {
    if (doesExprDefinedVariableAccess(lhsOperand) || doesExprDefineNestedExpr(lhsOperand) || !doesExprDefineNestedExpr(rhsOperand) || !syrec_operation::isCommutative(operationToBeApplied)) {
        return;
    }
    switchOperands(lhsOperand, rhsOperand);
}

bool PostOrderExprTraversalHelper::trySwitchOperandsForXorOperationIfRhsIsNestedExprWithLhsBeingNotNested(syrec::Expression::ptr& lhsOperand, syrec_operation::operation operationToBeApplied, syrec::Expression::ptr& rhsOperand) {
    if (operationToBeApplied != syrec_operation::operation::BitwiseXor || doesExprDefineNestedExpr(lhsOperand) == doesExprDefineNestedExpr(rhsOperand)) {
        return false;
    }

    if (doesExprDefineNestedExpr(lhsOperand)) {
        return true;
    }
    switchOperands(lhsOperand, rhsOperand);
    return true;
}

bool PostOrderExprTraversalHelper::tryTransformExpressionIfOperationShouldBeInverted(syrec::Expression::ptr& lhsOperand, syrec_operation::operation operationToBeApplied, syrec::Expression::ptr& rhsOperand, bool shouldOperationBeInverted) {
    if (shouldOperationBeInverted && operationToBeApplied == syrec_operation::operation::Subtraction) {
        switchOperands(lhsOperand, rhsOperand);
        return true;
    }
    return false;
}

void PostOrderExprTraversalHelper::switchOperands(syrec::Expression::ptr& lhsOperand, syrec::Expression::ptr& rhsOperand) {
    const auto backupOfLhsOperand = lhsOperand;
    lhsOperand                    = rhsOperand;
    rhsOperand                    = backupOfLhsOperand;
}

syrec_operation::operation PostOrderExprTraversalHelper::determineFinalAssignmentOperation(syrec_operation::operation currentAssignmentOperation) const {
    if (!isInversionOfAssignmentOperationsNecessary || !processedAnySubExpression) {
        return currentAssignmentOperation;
    }

    return *syrec_operation::invert(currentAssignmentOperation);
}

void PostOrderExprTraversalHelper::buildPostOrderTraversalQueueFor(const syrec::Expression::ptr& expr) {
    if (!doesExprDefineNestedExpr(expr)) {
        queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(nextSubExpressionInitialAssignmentOperation.front(), expr));
        return;
    }
    buildPostOrderTraversalQueueForSubExpr(expr);
}

void PostOrderExprTraversalHelper::buildPostOrderTraversalQueueForSubExpr(const syrec::Expression::ptr& expr) {
    const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr);
    if (exprAsBinaryExpr == nullptr) {
        return;
    }

    auto&      lhsOperand                             = exprAsBinaryExpr->lhs;
    auto&      rhsOperand                             = exprAsBinaryExpr->rhs;
    const auto mappedToOperationEnumValue                   = *syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
    auto transformedOperationConsideringInversionFlag = processedAnySubExpression && shouldCurrentOperationBeInverted ? *syrec_operation::invert(mappedToOperationEnumValue) : mappedToOperationEnumValue;

    /*
     * Transform an expression of the form ... - (<subExpr_1> - <subExpr_2>) to ... + (<subExpr_2> - <subExpr_1>)
     */
    if (processedAnySubExpression && mappedToOperationEnumValue == syrec_operation::operation::Subtraction && nextSubExpressionInitialAssignmentOperation.front() == syrec_operation::operation::MinusAssign) {
        nextSubExpressionInitialAssignmentOperation.back() = syrec_operation::operation::AddAssign;
        transformedOperationConsideringInversionFlag       = syrec_operation::operation::Subtraction;
        switchOperands(lhsOperand, rhsOperand);
        shouldCurrentOperationBeInverted = false;
    } else {
        // Perform an operand switch for a bitwise xor operation in case that the rhs operand is a nested expression while the lhs is not (this could lead to a simplification of the rhs expression that would otherwise be lost)
        if (!trySwitchOperandsForXorOperationIfRhsIsNestedExprWithLhsBeingNotNested(lhsOperand, transformedOperationConsideringInversionFlag, rhsOperand)) {
            // Try perform a switch of the operands if the rhs operand defines a signal access and either the current operation or its inversion (if inversion is required) is commutative
            trySwitchSignalAccessOperandToLhs(lhsOperand, transformedOperationConsideringInversionFlag, rhsOperand);
        }
        // Try perform a switch of the operands if the lhs operand does not define a signal access or nested expr while the rhs operand does define a nested expr
        trySwitchNestedExprOnRhsToLhsIfLatterIsConstantOrLoopVariable(lhsOperand, transformedOperationConsideringInversionFlag, rhsOperand);
    }

    const bool doesLhsDefinedNestedExpr   = doesExprDefineNestedExpr(lhsOperand);
    const bool doesRhsDefinedNestedExpr   = doesExprDefineNestedExpr(rhsOperand);

    if (!doesLhsDefinedNestedExpr && !doesRhsDefinedNestedExpr) {
        queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(determineFinalAssignmentOperation(nextSubExpressionInitialAssignmentOperation.front()), lhsOperand));
        nextSubExpressionInitialAssignmentOperation.pop();
        processedAnySubExpression = true;

        const auto  mappedToAssignmentOperationForCurrentOperation = *syrec_operation::getMatchingAssignmentOperationForOperation(transformedOperationConsideringInversionFlag);
        queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(determineFinalAssignmentOperation(mappedToAssignmentOperationForCurrentOperation), rhsOperand));
    } else if (doesLhsDefinedNestedExpr && doesRhsDefinedNestedExpr) {
        auto backupOfCurrentInversionFlagValue = shouldCurrentOperationBeInverted;
        buildPostOrderTraversalQueueForSubExpr(lhsOperand);
        shouldCurrentOperationBeInverted        = backupOfCurrentInversionFlagValue;
        
        const auto& assignmentOperationForRhsOperand = *syrec_operation::getMatchingAssignmentOperationForOperation(transformedOperationConsideringInversionFlag);
        // Operation nodes with the '^' operation and the rhs operand defining a nested expression cannot be further simplified and thus the whole rhs expression will be used in the generated subassignments
        if (assignmentOperationForRhsOperand == syrec_operation::operation::XorAssign && doesExprDefineNestedExpr(exprAsBinaryExpr->rhs)) {
            queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(assignmentOperationForRhsOperand, exprAsBinaryExpr->rhs));
            return;
        }

        backupOfCurrentInversionFlagValue = shouldCurrentOperationBeInverted;
        shouldCurrentOperationBeInverted  = assignmentOperationForRhsOperand == syrec_operation::operation::MinusAssign;
        nextSubExpressionInitialAssignmentOperation.push(assignmentOperationForRhsOperand);

        buildPostOrderTraversalQueueForSubExpr(rhsOperand);
        shouldCurrentOperationBeInverted = backupOfCurrentInversionFlagValue;
    } else {
        /*
            Handle the two cases:
            I. (<signalAccess> op <subExpr>)
            II. (<subExpr> op <signalAccess>)
        */

        if (doesExprDefineNestedExpr(lhsOperand)) {
            const auto backupOfCurrentInversionFlagValue = shouldCurrentOperationBeInverted;
            buildPostOrderTraversalQueueForSubExpr(lhsOperand);
            shouldCurrentOperationBeInverted = backupOfCurrentInversionFlagValue;

            const auto& mappedToAssignmentOperationForCurrentOperation = *syrec_operation::getMatchingAssignmentOperationForOperation(transformedOperationConsideringInversionFlag);
            queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(determineFinalAssignmentOperation(mappedToAssignmentOperationForCurrentOperation), rhsOperand));
        } else {
            queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(determineFinalAssignmentOperation(nextSubExpressionInitialAssignmentOperation.front()), lhsOperand));
            nextSubExpressionInitialAssignmentOperation.pop();
            processedAnySubExpression = true;
            
            const auto& assignmentOperationForRhsOperand = *syrec_operation::getMatchingAssignmentOperationForOperation(transformedOperationConsideringInversionFlag);
            // Operation nodes with the '^' operation and the rhs operand defining a nested expression cannot be further simplified and thus the whole rhs expression will be used in the generated subassignments
            if (assignmentOperationForRhsOperand == syrec_operation::operation::XorAssign) {
                queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(assignmentOperationForRhsOperand, rhsOperand));
                return;
            }

            const auto backupOfCurrentInversionFlagValue = shouldCurrentOperationBeInverted;
            shouldCurrentOperationBeInverted             = assignmentOperationForRhsOperand == syrec_operation::operation::MinusAssign;
            nextSubExpressionInitialAssignmentOperation.push(assignmentOperationForRhsOperand);

            buildPostOrderTraversalQueueForSubExpr(rhsOperand);
            shouldCurrentOperationBeInverted = backupOfCurrentInversionFlagValue;
        }
    }
}

void PostOrderExprTraversalHelper::resetInternalData() {
    queueOfLeafNodesInPostOrder.clear();
    postOrderContainerIdx = 0;
    processedAnySubExpression = false;
    shouldCurrentOperationBeInverted = false;
    isInversionOfAssignmentOperationsNecessary = false;
}