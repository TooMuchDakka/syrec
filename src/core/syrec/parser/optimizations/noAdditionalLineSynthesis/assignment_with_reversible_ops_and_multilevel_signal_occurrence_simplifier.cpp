#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_reversible_ops_and_multilevel_signal_occurrence_simplifier.hpp"

#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/post_order_expr_traversal_helper.hpp"

using namespace noAdditionalLineSynthesis;

bool AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence::simplificationPrecondition(const syrec::AssignStatement::ptr& assignmentStmt) {
    // Check whether given pointer can be casted to assignment statement is already done in base class
    const auto& assignStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);

    // Currently the simplification of an XOR assignment statement and of non-binary expressions on the rhs of the assignment are not supported
    if (assignStmtCasted->op == syrec::AssignStatement::Exor || std::dynamic_pointer_cast<syrec::BinaryExpression>(assignStmtCasted->rhs) == nullptr) {
        return false;
    }

    if (assignStmtCasted->op == syrec::AssignStatement::Subtract) {
        return isXorOperationOnlyDefinedForLeaveNodesInAST(assignStmtCasted->rhs);
    }
    if (assignStmtCasted->op == syrec::AssignStatement::Add) {
        return true;
    }

    return false;
}

syrec::Statement::vec AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence::simplifyWithoutPreconditionCheck(const syrec::AssignStatement::ptr& assignmentStmt) {
    syrec::Statement::vec generatedAssignments;

    const auto& assignmentStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    const auto& exprTraversalHelper = std::make_unique<PostOrderExprTraversalHelper>(
        *tryMapAssignmentOperationFlagToEnum(assignmentStmtCasted->op), 
        assignmentStmtCasted->rhs
    );
    
    const syrec_operation::operation                                         topMostAssignmentStatement       = *tryMapAssignmentOperationFlagToEnum(assignmentStmtCasted->op);
    std::optional<PostOrderExprTraversalHelper::PostOrderTraversalNode::ptr> potentialNextSignalAccess        = exprTraversalHelper->getNextElement();
    //bool                                                                     wasFirstExprInPostOrderProcessed = false;

    // TODO: Since this case should not happen, do we need this check ?
    if (!potentialNextSignalAccess.has_value()) {
        return {};
    }

    do {
        const auto& nextSignalAccessInPostOrder = *potentialNextSignalAccess;
        auto assignmentOperationToUse    = topMostAssignmentStatement;

        syrec::expression::ptr assignmentRhsExpr;
        if (exprTraversalHelper->areBothOperandsOfOperationNodeLeaves(nextSignalAccessInPostOrder)) {
            const auto& rhsOperand = exprTraversalHelper->getNextElement();
            assignmentRhsExpr             = std::make_shared<syrec::BinaryExpression>(
                std::make_shared<syrec::VariableExpression>(nextSignalAccessInPostOrder->signalAccess),
                    mapAssignmentOperationEnumValueToFlag(exprTraversalHelper->getOperationOfParent(nextSignalAccessInPostOrder)),
                std::make_shared<syrec::VariableExpression>(rhsOperand.value()->signalAccess));

            if (const auto& requiredAssignmentOperationForSubexpression = exprTraversalHelper->tryGetRequiredAssignmentOperation(*potentialNextSignalAccess); requiredAssignmentOperationForSubexpression.has_value()) {
                assignmentOperationToUse = *requiredAssignmentOperationForSubexpression;
            } else {
                if (const auto& grandParentOperation = exprTraversalHelper->getOperationOfGrandParent(nextSignalAccessInPostOrder); grandParentOperation.has_value()) {
                    assignmentOperationToUse = *syrec_operation::getMatchingAssignmentOperationForOperation(*grandParentOperation);
                } else {
                    assignmentOperationToUse = topMostAssignmentStatement;
                }
            }

            //assignmentOperationToUse = *exprTraversalHelper->tryGetRequiredAssignmentOperation(*potentialNextSignalAccess);
            //assignmentOperationToUse = *syrec_operation::getMatchingAssignmentOperationForOperation(*exprTraversalHelper->getOperationOfGrandParent(nextSignalAccessInPostOrder));

        } else {
            assignmentRhsExpr = std::make_shared<syrec::VariableExpression>(nextSignalAccessInPostOrder->signalAccess);
            if (const auto& requiredAssignmentOperationForSubexpression = exprTraversalHelper->tryGetRequiredAssignmentOperation(*potentialNextSignalAccess); requiredAssignmentOperationForSubexpression.has_value()) {
                assignmentOperationToUse = *requiredAssignmentOperationForSubexpression;
            } else {
                /*
                 * When the assignment operation was defined as '-=', the leaf of an operation nodes (with binary operation b_op) with only one leaf as its operands
                 * will get added as the assignment statement assignment_lhs b_op^(-1)= leaf
                 */
                /*assignmentOperationToUse = *syrec_operation::getMatchingAssignmentOperationForOperation(
                    topMostAssignmentStatement == syrec_operation::operation::minus_assign
                    ? *syrec_operation::invert(exprTraversalHelper->getOperationOfParent(nextSignalAccessInPostOrder))
                    : exprTraversalHelper->getOperationOfParent(nextSignalAccessInPostOrder)
                );*/
                assignmentOperationToUse = *syrec_operation::getMatchingAssignmentOperationForOperation(exprTraversalHelper->getOperationOfParent(nextSignalAccessInPostOrder));

            }
        }

        const auto& generatedAssignmentStmt = std::make_shared<syrec::AssignStatement>(
                assignmentStmtCasted->lhs,
                mapAssignmentOperationEnumValueToFlag(assignmentOperationToUse),
                assignmentRhsExpr);

        /*const auto& generatedAssignmentStmt = std::make_shared<syrec::AssignStatement>(
            assignmentStmtCasted->lhs,
                mapAssignmentOperationEnumValueToFlag(wasFirstExprInPostOrderProcessed ? determineRequiredAssignmentOperation(topMostAssignmentStatement, assignmentOperationToUse) : assignmentOperationToUse),
            assignmentRhsExpr
        );*/
        generatedAssignments.emplace_back(generatedAssignmentStmt);

        potentialNextSignalAccess        = exprTraversalHelper->getNextElement();
        //wasFirstExprInPostOrderProcessed = true;
    } while (potentialNextSignalAccess.has_value());
    return generatedAssignments;
}

bool AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence::isXorOperationOnlyDefinedForLeaveNodesInAST(const syrec::expression::ptr& expr) {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        return exprAsBinaryExpr->op == syrec::BinaryExpression::Exor ? !doesExprDefineNestedExpr(exprAsBinaryExpr->lhs) && !doesExprDefineNestedExpr(exprAsBinaryExpr->rhs) : true;
    }
    return true;
}

//syrec_operation::operation AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence::determineRequiredAssignmentOperation(syrec_operation::operation topMostAssignmentOperation, syrec_operation::operation operationNodeOperation) {
//    // We are assuming that the assignment to simplify consists of only reversible operations and thus the invert operation should return a valid operation
//    return topMostAssignmentOperation == syrec_operation::operation::minus_assign ? *syrec_operation::invert(operationNodeOperation) : operationNodeOperation;
//}

unsigned int AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence::mapAssignmentOperationEnumValueToFlag(syrec_operation::operation assignmentOperation) {
    // We are assuming that the assignment to simplify consists of only reversible operations
    switch (assignmentOperation) {
        case syrec_operation::operation::add_assign:
            return syrec::AssignStatement::Add;
        case syrec_operation::operation::minus_assign:
            return syrec::AssignStatement::Subtract;
        default:
            return syrec::AssignStatement::Exor;
    }
}

std::optional<syrec_operation::operation> AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence::tryMapBinaryOperationFlagToCorrespondingAssignmentOperationEnumValue(unsigned operationFlag) {
    // We are assuming that the assignment to simplify consists of only reversible operations
    switch (operationFlag) {
        case syrec::BinaryExpression::Add:
            return std::make_optional(syrec_operation::operation::add_assign);
        case syrec::BinaryExpression::Subtract:
            return std::make_optional(syrec_operation::operation::minus_assign);
        case syrec::BinaryExpression::Exor:
            return std::make_optional(syrec_operation::operation::xor_assign);
        default:
            return std::nullopt;
    }
}

std::optional<syrec_operation::operation> AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence::tryMapAssignmentOperationFlagToEnum(unsigned int operationFlag) {
    // We are assuming that the assignment to simplify consists of only reversible operations
    switch (operationFlag) {
        case syrec::AssignStatement::Add: 
            return std::make_optional(syrec_operation::operation::add_assign);
        case syrec::AssignStatement::Subtract: 
            return std::make_optional(syrec_operation::operation::minus_assign);
        default:
            return std::make_optional(syrec_operation::operation::xor_assign);
    }
}