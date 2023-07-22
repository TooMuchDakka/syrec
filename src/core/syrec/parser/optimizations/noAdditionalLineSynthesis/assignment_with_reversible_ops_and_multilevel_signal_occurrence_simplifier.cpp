#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_reversible_ops_and_multilevel_signal_occurrence_simplifier.hpp"

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

syrec::Statement::vec AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence::simplifyWithoutPreconditionCheck(const syrec::AssignStatement::ptr& expr) {
    return {};
}

bool AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence::isXorOperationOnlyDefinedForLeaveNodesInAST(const syrec::expression::ptr& expr) {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        return exprAsBinaryExpr->op == syrec::BinaryExpression::Exor ? doesExprDefineNestedExpr(exprAsBinaryExpr->lhs) && doesExprDefineNestedExpr(exprAsBinaryExpr->rhs) : true;
    }
    return true;
}
