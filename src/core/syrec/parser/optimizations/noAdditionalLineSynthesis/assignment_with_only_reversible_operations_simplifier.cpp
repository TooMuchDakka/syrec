#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_only_reversible_operations_simplifier.hpp"

using namespace noAdditionalLineSynthesis;

syrec::Statement::vec AssignmentWithOnlyReversibleOperationsSimplifier::simplifyWithoutPreconditionCheck(const syrec::AssignStatement::ptr& assignmentStmt, bool isValueOfAssignedToSignalBlockedByDataFlowAnalysis) {
    const auto& assignStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    if (auto simplifiedRhsExpr = simplifyWithoutPreconditionCheck(assignStmtCasted->rhs, isValueOfAssignedToSignalBlockedByDataFlowAnalysis); !simplifiedRhsExpr.empty()) {
        const auto& lastAssignedToSignalInRhs = std::dynamic_pointer_cast<syrec::AssignStatement>(simplifiedRhsExpr.back())->lhs;

        const auto topMostAssignment = std::make_shared<syrec::AssignStatement>(
                assignStmtCasted->lhs,
                assignStmtCasted->op,
                std::make_shared<syrec::VariableExpression>(lastAssignedToSignalInRhs));
        simplifiedRhsExpr.emplace_back(topMostAssignment);
        return simplifiedRhsExpr;
    }
    return {};
}

syrec::Statement::vec AssignmentWithOnlyReversibleOperationsSimplifier::simplifyWithoutPreconditionCheck(const syrec::BinaryExpression::ptr& expr, bool isValueOfAssignedToSignalBlockedByDataFlowAnalysis) {
    const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr);
    if (exprAsBinaryExpr == nullptr) {
        return {};
    }

    syrec::Statement::vec generatedAssignments;
    auto&                 lhsOperandOfExpression = exprAsBinaryExpr->lhs;
    auto&                 rhsOperandOfExpression = exprAsBinaryExpr->rhs;

    if (doesExprDefineNestedExpr(exprAsBinaryExpr->lhs)) {
        generatedAssignments       = simplifyWithoutPreconditionCheck(exprAsBinaryExpr->lhs, isValueOfAssignedToSignalBlockedByDataFlowAnalysis);
        bool simplificationOfLhsOK = false;
        if (!generatedAssignments.empty()) {
            if (const auto& finalAssignmentForLhsExpr = std::dynamic_pointer_cast<syrec::AssignStatement>(generatedAssignments.back()); finalAssignmentForLhsExpr != nullptr) {
                lhsOperandOfExpression = std::make_shared<syrec::VariableExpression>(finalAssignmentForLhsExpr->lhs);
                simplificationOfLhsOK  = true;
            }
        }

        if (!simplificationOfLhsOK) {
            return {};
        }
    }

    if (doesExprDefineNestedExpr(exprAsBinaryExpr->rhs)) {
        const auto& generatedAssignmentsForRhsExpr = simplifyWithoutPreconditionCheck(exprAsBinaryExpr->rhs, isValueOfAssignedToSignalBlockedByDataFlowAnalysis);
        bool        simplificationOfRhsOK          = false;
        if (!generatedAssignmentsForRhsExpr.empty()) {
            generatedAssignments.insert(generatedAssignments.end(), generatedAssignmentsForRhsExpr.begin(), generatedAssignmentsForRhsExpr.end());
            if (const auto& finalAssignmentForRhsExpr = std::dynamic_pointer_cast<syrec::AssignStatement>(generatedAssignmentsForRhsExpr.back()); finalAssignmentForRhsExpr != nullptr) {
                rhsOperandOfExpression = std::make_shared<syrec::VariableExpression>(finalAssignmentForRhsExpr->lhs);
                simplificationOfRhsOK  = true;
            }
        }

        if (!simplificationOfRhsOK) {
            return {};
        }
    }

    if (isExprConstantNumber(lhsOperandOfExpression) && doesExprDefineVariableAccess(rhsOperandOfExpression)) {
        const auto backupOfRhsOperand = rhsOperandOfExpression;
        rhsOperandOfExpression        = lhsOperandOfExpression;
        lhsOperandOfExpression        = backupOfRhsOperand;
    } else if (isExprConstantNumber(lhsOperandOfExpression) && isExprConstantNumber(rhsOperandOfExpression)) {
        return {};
    }

    const auto generatedAssignmentStatement = std::make_shared<syrec::AssignStatement>(
            std::dynamic_pointer_cast<syrec::VariableExpression>(lhsOperandOfExpression)->var,
            exprAsBinaryExpr->op,
            rhsOperandOfExpression);
    generatedAssignments.emplace_back(generatedAssignmentStatement);
    return generatedAssignments;
}

bool AssignmentWithOnlyReversibleOperationsSimplifier::simplificationPrecondition(const syrec::AssignStatement::ptr& assignmentStmt) {
    const auto& assignStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    if (assignStmtCasted == nullptr) {
        return {};
    }

    const auto preconditionStatus = isEveryLhsOperandOfAnyBinaryExprDefinedOnceOnEveryLevelOfTheAST(assignStmtCasted->rhs);
    return preconditionStatus.value_or(false);
}