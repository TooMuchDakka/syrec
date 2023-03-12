#include "core/syrec/parser/optimizations/no_additional_line_assignment.hpp"
#include "core/syrec/parser/operation.hpp"

using namespace optimizations;

std::optional<syrec_operation::operation> mapOperation(const unsigned int binaryExpressionOp) {
    std::optional<syrec_operation::operation> mappedToOperation;
    switch (binaryExpressionOp) {
        case syrec::BinaryExpression::Add:
            mappedToOperation.emplace(syrec_operation::operation::addition);
            break;
        case syrec::BinaryExpression::Subtract:
            mappedToOperation.emplace(syrec_operation::operation::subtraction);
            break;
        case syrec::BinaryExpression::Exor:
            mappedToOperation.emplace(syrec_operation::operation::bitwise_xor);
            break;
        default:
            break;
    }
    return mappedToOperation;
}

bool doesAssignmentRhsOnlyContainReversibleOperations(const syrec::BinaryExpression& assignmentStatementRhs) {
    if (!mapOperation(assignmentStatementRhs.op).has_value()) {
        return false;
    }
    
    if (const auto lhsExprAsBinayExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(assignmentStatementRhs.lhs); lhsExprAsBinayExpr != nullptr) {
        if (!mapOperation(lhsExprAsBinayExpr->op).has_value()) {
            return false;
        }
        if (const auto rhsExprAsBinayExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(assignmentStatementRhs.rhs); rhsExprAsBinayExpr != nullptr) {
            return mapOperation(rhsExprAsBinayExpr->op).has_value();
        }
    }
    return true;
}

std::optional<syrec_operation::operation> tryMapOperation(const unsigned int binaryExpressionOp) {
    std::optional<syrec_operation::operation> mappedToOperation;
    switch (binaryExpressionOp) {
        case syrec::BinaryExpression::Add:
            mappedToOperation.emplace(syrec_operation::operation::addition);
            break;
        case syrec::BinaryExpression::Subtract:
            mappedToOperation.emplace(syrec_operation::operation::subtraction);
            break;
        case syrec::BinaryExpression::Exor:
            mappedToOperation.emplace(syrec_operation::operation::bitwise_xor);
            break;
        default:
            break;
    }
    return mappedToOperation;
}

std::optional<syrec_operation::operation> tryGetOperationFromExpression(const syrec::expression::ptr& expression) {
    if (const auto exprAsBinayExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expression); exprAsBinayExpr != nullptr) {
        return tryMapOperation(exprAsBinayExpr->op);
    }
    return std::nullopt;
}


LineAwareOptimizationResult optimizations::optimize(const syrec::AssignStatement& assignmentStatement) {
    
}
