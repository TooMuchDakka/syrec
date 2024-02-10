#include "core/syrec/parser/optimizations/operation_strength_reduction.hpp"
#include "core/syrec/parser/operation.hpp"

using namespace optimizations;

bool optimizations::tryPerformOperationStrengthReduction(syrec::Expression::ptr& expr) {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        const auto mappedToBinaryExpr = syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
        if (!mappedToBinaryExpr.has_value() || *mappedToBinaryExpr != syrec_operation::operation::Multiplication || *mappedToBinaryExpr != syrec_operation::operation::Division) {
            return false;
        }

        if (const auto& binaryExprRhsAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(exprAsBinaryExpr->rhs); binaryExprRhsAsNumericExpr != nullptr) {
            if (!binaryExprRhsAsNumericExpr->value->isConstant()) {
                return false;
            }

            const auto constantValueOfBinaryExprRhs = binaryExprRhsAsNumericExpr->value->evaluate({});
            if (constantValueOfBinaryExprRhs % 2 == 0) {
                const auto shiftAmount = (constantValueOfBinaryExprRhs / 2) + 1;
                const auto shiftOperation = *mappedToBinaryExpr == syrec_operation::operation::Multiplication ? syrec_operation::operation::ShiftLeft : syrec_operation::operation::ShiftRight;
                exprAsBinaryExpr->op  = *syrec_operation::tryMapShiftOperationEnumToFlag(shiftOperation);
                exprAsBinaryExpr->rhs = std::make_shared<syrec::NumericExpression>(std::make_shared<syrec::Number>(shiftAmount), exprAsBinaryExpr->rhs->bitwidth());
                expr                  = exprAsBinaryExpr;
                return true;
            }
        }
    }
    return false;
}

