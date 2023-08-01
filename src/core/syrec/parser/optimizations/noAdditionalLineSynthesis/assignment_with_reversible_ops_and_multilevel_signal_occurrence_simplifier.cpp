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
        *syrec_operation::tryMapAssignmentOperationFlagToEnum(assignmentStmtCasted->op), 
        assignmentStmtCasted->rhs
    );
    
    const syrec_operation::operation                                         initialAssignmentOperation       = *syrec_operation::tryMapAssignmentOperationFlagToEnum(assignmentStmtCasted->op);
    std::optional<PostOrderExprTraversalHelper::PostOrderTraversalNode::ptr> potentialNextSignalAccess        = exprTraversalHelper->getNextElement();
    bool                                                                     wasFirstExprInPostOrderProcessed = false;

    // TODO: Since this case should not happen, do we need this check ?
    if (!potentialNextSignalAccess.has_value()) {
        return {};
    }

    do {
        const auto& nextSignalAccessInPostOrder = *potentialNextSignalAccess;
        auto assignmentOperationToUse    = initialAssignmentOperation;

        syrec::expression::ptr assignmentRhsExpr;
        if (exprTraversalHelper->areBothOperandsOfOperationNodeLeaves(nextSignalAccessInPostOrder)) {
            const auto& rhsOperand = exprTraversalHelper->getNextElement();
            assignmentRhsExpr             = std::make_shared<syrec::BinaryExpression>(
                std::make_shared<syrec::VariableExpression>(nextSignalAccessInPostOrder->signalAccess),
                //TODO: check mapAssignmentOperationEnumValueToFlag(*syrec_operation::getMatchingAssignmentOperationForOperation(exprTraversalHelper->getOperationOfParent(nextSignalAccessInPostOrder))),
                *syrec_operation::tryMapBinaryOperationEnumToFlag(exprTraversalHelper->getOperationOfParent(nextSignalAccessInPostOrder)),
                std::make_shared<syrec::VariableExpression>(rhsOperand.value()->signalAccess));

            if (const auto& requiredAssignmentOperationForSubexpression = exprTraversalHelper->tryGetRequiredAssignmentOperation(*potentialNextSignalAccess); requiredAssignmentOperationForSubexpression.has_value()) {
                assignmentOperationToUse = *requiredAssignmentOperationForSubexpression;
            } else {
                if (const auto& grandParentOperation = exprTraversalHelper->getOperationOfGrandParent(nextSignalAccessInPostOrder); grandParentOperation.has_value()) {
                    assignmentOperationToUse = *syrec_operation::getMatchingAssignmentOperationForOperation(*grandParentOperation);
                }
            }
        } else {
            assignmentRhsExpr = std::make_shared<syrec::VariableExpression>(nextSignalAccessInPostOrder->signalAccess);
            if (const auto& requiredAssignmentOperationForSubexpression = exprTraversalHelper->tryGetRequiredAssignmentOperation(*potentialNextSignalAccess); requiredAssignmentOperationForSubexpression.has_value()) {
                assignmentOperationToUse = *requiredAssignmentOperationForSubexpression;
            } else {
                assignmentOperationToUse = *syrec_operation::getMatchingAssignmentOperationForOperation(exprTraversalHelper->getOperationOfParent(nextSignalAccessInPostOrder));
            }
        }

        // When we are adding the top most expression of the rhs expr of the initial assignment statement we need to reuse the assignment operation of the assignment statement
        if (!wasFirstExprInPostOrderProcessed) {
            assignmentOperationToUse = initialAssignmentOperation;
            wasFirstExprInPostOrderProcessed = true;
        } else {
            /*
             * When the assignment operation was defined as '-=', the value of the current operation node (with binary operation b_op) will get inverted and thus the assignment statement: assignment_lhs b_op^(-1)= leaf will be created
             */
            assignmentOperationToUse = initialAssignmentOperation == syrec_operation::operation::MinusAssign
                ? *syrec_operation::invert(assignmentOperationToUse)
                : assignmentOperationToUse;
        }

        const auto& generatedAssignmentStmt = std::make_shared<syrec::AssignStatement>(
                assignmentStmtCasted->lhs,
                *syrec_operation::tryMapAssignmentOperationEnumToFlag(assignmentOperationToUse),
                assignmentRhsExpr);
        generatedAssignments.emplace_back(generatedAssignmentStmt);

        potentialNextSignalAccess        = exprTraversalHelper->getNextElement();
    } while (potentialNextSignalAccess.has_value());
    return generatedAssignments;
}

bool AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence::isXorOperationOnlyDefinedForLeaveNodesInAST(const syrec::expression::ptr& expr) {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        return exprAsBinaryExpr->op == syrec::BinaryExpression::Exor ? !doesExprDefineNestedExpr(exprAsBinaryExpr->lhs) && !doesExprDefineNestedExpr(exprAsBinaryExpr->rhs) : true;
    }
    return true;
}