#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/post_order_expr_traversal_helper.hpp"

using namespace noAdditionalLineSynthesis;

/*
    Simplify operations of the form <subExpr> - (op_1 - op 2) to <subExpr> + (b - 2)
    Reorder commutative operation nodes with two leaf nodes with one being a signal access and the other being a constant or loop variable to (signalAccess op other)
    Could perform split of top most expression at '^' if one operand is constant, i.e. a += (2 ^ <subExpr>) = a ^= <subExpr>; a ^= 2 iff a = 0

    (2 - ((2 - b) - (2 - c))) = (2 - ((c - 2) + (2 - b)))
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

void PostOrderExprTraversalHelper::buildPostOrderQueue(syrec_operation::operation assignmentOperation, const syrec::expression::ptr expr, bool isValueOfAssignedToSignalZeroPriorToAssignment) {
    if (!canHandleAssignmentOperation(assignmentOperation) || (!isValueOfAssignedToSignalZeroPriorToAssignment && assignmentOperation == syrec_operation::operation::XorAssign)) {
        return;
    }

    resetInternalData();
    nextSubExpressionInitialAssignmentOperation.emplace(assignmentOperation);
    shouldCurrentOperationBeInverted = assignmentOperation == syrec_operation::operation::MinusAssign;
    buildPostOrderTraversalQueueFor(expr);
}

bool PostOrderExprTraversalHelper::doesExprDefineNestedExpr(const syrec::expression::ptr& expr) {
    return std::dynamic_pointer_cast<syrec::BinaryExpression>(expr) != nullptr;
}

bool PostOrderExprTraversalHelper::doesExprDefinedVariableAccess(const syrec::expression::ptr& expr) {
    return std::dynamic_pointer_cast<syrec::VariableExpression>(expr) != nullptr;
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

void PostOrderExprTraversalHelper::trySwitchSignalAccessOperandToLhs(syrec::expression::ptr& lhsOperand, syrec_operation::operation operationToBeApplied, syrec::expression::ptr& rhsOperand) {
    if (doesExprDefineNestedExpr(lhsOperand) || doesExprDefinedVariableAccess(lhsOperand) || !doesExprDefinedVariableAccess(rhsOperand) || !syrec_operation::isCommutative(operationToBeApplied)) {
        return;
    }
    switchOperands(lhsOperand, rhsOperand);

   /* const auto isCurrentOperationCommutative = syrec_operation::isCommutative(operationToBeApplied);
    if (!isCurrentOperationCommutative && !shouldOperationBeInverted) {
        return;
    }

    if (isCurrentOperationCommutative || (!isCurrentOperationCommutative && shouldOperationBeInverted && syrec_operation::isCommutative(*syrec_operation::invert(operationToBeApplied)))) {
        const auto backOfLhsOperand = lhsOperand;
        lhsOperand                  = rhsOperand;
        rhsOperand                  = backOfLhsOperand;
    }*/
}

void PostOrderExprTraversalHelper::trySwitchNestedExprOnRhsToLhsIfLatterIsConstantOrLoopVariable(syrec::expression::ptr& lhsOperand, syrec_operation::operation operationToBeApplied, syrec::expression::ptr& rhsOperand) {
    if (doesExprDefinedVariableAccess(lhsOperand) || doesExprDefineNestedExpr(lhsOperand) || !doesExprDefineNestedExpr(rhsOperand) || !syrec_operation::isCommutative(operationToBeApplied)) {
        return;
    }
    switchOperands(lhsOperand, rhsOperand);

    /*const auto isCurrentOperationCommutative = syrec_operation::isCommutative(operationToBeApplied);
    if (!isCurrentOperationCommutative && !shouldOperationBeInverted) {
        return;
    }

    if (isCurrentOperationCommutative || (!isCurrentOperationCommutative && shouldOperationBeInverted && syrec_operation::isCommutative(*syrec_operation::invert(operationToBeApplied)))) {
        const auto backOfLhsOperand = lhsOperand;
        lhsOperand                  = rhsOperand;
        rhsOperand                  = backOfLhsOperand;
    }*/
}

bool PostOrderExprTraversalHelper::trySwitchOperandsForXorOperationIfRhsIsNestedExprWithLhsBeingNotNested(syrec::expression::ptr& lhsOperand, syrec_operation::operation operationToBeApplied, syrec::expression::ptr& rhsOperand) {
    if (operationToBeApplied != syrec_operation::operation::BitwiseXor || doesExprDefineNestedExpr(lhsOperand) == doesExprDefineNestedExpr(rhsOperand)) {
        return false;
    }

    if (doesExprDefineNestedExpr(lhsOperand)) {
        return true;
    }
    switchOperands(lhsOperand, rhsOperand);
    return true;
}

bool PostOrderExprTraversalHelper::tryTransformExpressionIfOperationShouldBeInverted(syrec::expression::ptr& lhsOperand, syrec_operation::operation operationToBeApplied, syrec::expression::ptr& rhsOperand, bool shouldOperationBeInverted) {
    if (shouldOperationBeInverted && operationToBeApplied == syrec_operation::operation::Subtraction) {
        switchOperands(lhsOperand, rhsOperand);
        return true;
    }
    return false;
}

void PostOrderExprTraversalHelper::switchOperands(syrec::expression::ptr& lhsOperand, syrec::expression::ptr& rhsOperand) {
    const auto backupOfLhsOperand = lhsOperand;
    lhsOperand                    = rhsOperand;
    rhsOperand                    = backupOfLhsOperand;
}

void PostOrderExprTraversalHelper::buildPostOrderTraversalQueueFor(const syrec::expression::ptr& expr) {
    if (!doesExprDefineNestedExpr(expr)) {
        queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(nextSubExpressionInitialAssignmentOperation.front(), expr));
        return;
    }
    buildPostOrderTraversalQueueForSubExpr(expr);
}

void PostOrderExprTraversalHelper::buildPostOrderTraversalQueueForSubExpr(const syrec::expression::ptr& expr) {
    const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr);
    if (exprAsBinaryExpr == nullptr) {
        return;
    }

    auto&      lhsOperand                             = exprAsBinaryExpr->lhs;
    auto&      rhsOperand                             = exprAsBinaryExpr->rhs;
    auto       mappedToOperationEnumValue             = *syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
    const auto ignoreInversionFlagForCurrentOperation = tryTransformExpressionIfOperationShouldBeInverted(lhsOperand, mappedToOperationEnumValue, rhsOperand, processedAnySubExpression && shouldCurrentOperationBeInverted);
    auto shouldInversionOfCurrentOperationBeApplied = !ignoreInversionFlagForCurrentOperation ? shouldCurrentOperationBeInverted : false;

    if (processedAnySubExpression && ignoreInversionFlagForCurrentOperation && nextSubExpressionInitialAssignmentOperation.front() == syrec_operation::operation::MinusAssign) {
        nextSubExpressionInitialAssignmentOperation.back() = *syrec_operation::invert(nextSubExpressionInitialAssignmentOperation.back());
       /* auto backupOfLhsOperand                            = lhsOperand;
        lhsOperand                                         = rhsOperand;
        rhsOperand                                         = backupOfLhsOperand;*/
    } else {
        // Perform an operand switch for a bitwise xor operation in case that the rhs operand is a nested expression while the lhs is not (this could lead to a simplification of the rhs expression that would otherwise be lost)
        if (!trySwitchOperandsForXorOperationIfRhsIsNestedExprWithLhsBeingNotNested(lhsOperand, mappedToOperationEnumValue, rhsOperand)) {
            // Try perform a switch of the operands if the rhs operand defines a signal access and either the current operation or its inversion (if inversion is required) is commutative
            trySwitchSignalAccessOperandToLhs(lhsOperand, mappedToOperationEnumValue, rhsOperand);
        }
        // Try perform a switch of the operands if the lhs operand does not define a signal access or nested expr while the rhs operand does define a nested expr
        trySwitchNestedExprOnRhsToLhsIfLatterIsConstantOrLoopVariable(lhsOperand, mappedToOperationEnumValue, rhsOperand);
    }

    const bool doesLhsDefinedNestedExpr   = doesExprDefineNestedExpr(lhsOperand);
    const bool doesRhsDefinedNestedExpr   = doesExprDefineNestedExpr(rhsOperand);

    if (!doesLhsDefinedNestedExpr && !doesRhsDefinedNestedExpr) {
        auto firstLeafNodeInPostOrder  = lhsOperand;
        auto secondLeafNodeInPostOrder = rhsOperand;

        trySwitchSignalAccessOperandToLhs(firstLeafNodeInPostOrder, mappedToOperationEnumValue, secondLeafNodeInPostOrder);
        queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(nextSubExpressionInitialAssignmentOperation.front(), firstLeafNodeInPostOrder));
        nextSubExpressionInitialAssignmentOperation.pop();
        processedAnySubExpression = true;

        const auto  mappedtoAssignmentOperationForCurrentOperation = *syrec_operation::getMatchingAssignmentOperationForOperation(mappedToOperationEnumValue);
        const auto& assignmentOperationToUseForRhs                 = shouldInversionOfCurrentOperationBeApplied ? *syrec_operation::invert(mappedtoAssignmentOperationForCurrentOperation) : mappedtoAssignmentOperationForCurrentOperation;

        queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(assignmentOperationToUseForRhs, secondLeafNodeInPostOrder));
    } else if (doesLhsDefinedNestedExpr && doesRhsDefinedNestedExpr) {
        auto backupOfCurrentInversionFlagValue = shouldInversionOfCurrentOperationBeApplied;
        shouldCurrentOperationBeInverted       = shouldInversionOfCurrentOperationBeApplied;
        buildPostOrderTraversalQueueForSubExpr(lhsOperand);
        shouldCurrentOperationBeInverted        = backupOfCurrentInversionFlagValue;

        const auto  mappedtoAssignmentOperation = *syrec_operation::getMatchingAssignmentOperationForOperation(mappedToOperationEnumValue);
        const auto& assignmentOperationToUse    = shouldInversionOfCurrentOperationBeApplied ? *syrec_operation::invert(mappedtoAssignmentOperation) : mappedtoAssignmentOperation;

        // Operation nodes with the '^' operation and the rhs operand defining a nested expression cannot be further simplified and thus the whole rhs expression will be used in the generated subassignments
        if (assignmentOperationToUse == syrec_operation::operation::XorAssign && doesExprDefineNestedExpr(exprAsBinaryExpr->rhs)) {
            queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(assignmentOperationToUse, exprAsBinaryExpr->rhs));
            return;
        }

        backupOfCurrentInversionFlagValue = shouldInversionOfCurrentOperationBeApplied;
        shouldCurrentOperationBeInverted             = assignmentOperationToUse == syrec_operation::operation::MinusAssign;
        nextSubExpressionInitialAssignmentOperation.push(assignmentOperationToUse);

        buildPostOrderTraversalQueueForSubExpr(rhsOperand);
        shouldCurrentOperationBeInverted = backupOfCurrentInversionFlagValue;
    } else {
        /*
                Handle the two cases:
                I. (<signalAccess> op <subExpr>)
                II. (<subExpr> op <signalAccess>)
            */
        auto firstOperandInPostOrder  = lhsOperand;
        auto secondOperandInPostOrder = rhsOperand;

        if (doesExprDefineNestedExpr(firstOperandInPostOrder)) {
            const auto backupOfCurrentInversionFlagValue = shouldInversionOfCurrentOperationBeApplied;
            buildPostOrderTraversalQueueForSubExpr(firstOperandInPostOrder);
            shouldCurrentOperationBeInverted = backupOfCurrentInversionFlagValue;

            const auto& mappedToAssignmentOperationForCurrentOperation = *syrec_operation::getMatchingAssignmentOperationForOperation(mappedToOperationEnumValue);
            const auto& assignmentOperationForRhsOperand               = shouldInversionOfCurrentOperationBeApplied ? *syrec_operation::invert(mappedToAssignmentOperationForCurrentOperation) : mappedToAssignmentOperationForCurrentOperation;
            queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(assignmentOperationForRhsOperand, secondOperandInPostOrder));
        } else {
            queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(nextSubExpressionInitialAssignmentOperation.front(), firstOperandInPostOrder));
            nextSubExpressionInitialAssignmentOperation.pop();
            processedAnySubExpression = true;

            const auto& mappedToAssignmentOperationForCurrentOperation = *syrec_operation::getMatchingAssignmentOperationForOperation(mappedToOperationEnumValue);
            const auto& assignmentOperationForRhsOperand               = shouldInversionOfCurrentOperationBeApplied ? *syrec_operation::invert(mappedToAssignmentOperationForCurrentOperation) : mappedToAssignmentOperationForCurrentOperation;
            // Operation nodes with the '^' operation and the rhs operand defining a nested expression cannot be further simplified and thus the whole rhs expression will be used in the generated subassignments
            if (assignmentOperationForRhsOperand == syrec_operation::operation::XorAssign) {
                queueOfLeafNodesInPostOrder.emplace_back(GeneratedSubAssignment(assignmentOperationForRhsOperand, secondOperandInPostOrder));
                return;
            }

            const auto backupOfCurrentInversionFlagValue = shouldInversionOfCurrentOperationBeApplied;
            shouldCurrentOperationBeInverted             = assignmentOperationForRhsOperand == syrec_operation::operation::MinusAssign;
            nextSubExpressionInitialAssignmentOperation.push(assignmentOperationForRhsOperand);

            buildPostOrderTraversalQueueForSubExpr(secondOperandInPostOrder);
            shouldCurrentOperationBeInverted = backupOfCurrentInversionFlagValue;
        }
    }
}

void PostOrderExprTraversalHelper::resetInternalData() {
    queueOfLeafNodesInPostOrder.clear();
    postOrderContainerIdx = 0;
    processedAnySubExpression = false;
    shouldCurrentOperationBeInverted = false;
}