#include "core/syrec/parser/optimizations/operation_strength_reduction.hpp"
#include <core/syrec/parser/parser_utilities.hpp>

using namespace optimizations;

bool optimizations::tryPerformOperationStrengthReduction(syrec::expression::ptr& expr) {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        const auto mappedToBinaryExpr = parser::ParserUtilities::mapInternalBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
        if (!mappedToBinaryExpr.has_value() || *mappedToBinaryExpr != syrec_operation::operation::multiplication || *mappedToBinaryExpr != syrec_operation::operation::division) {
            return false;
        }

        if (const auto& binaryExprRhsAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(exprAsBinaryExpr->rhs); binaryExprRhsAsNumericExpr != nullptr) {
            if (!binaryExprRhsAsNumericExpr->value->isConstant()) {
                return false;
            }

            const auto constantValueOfBinaryExprRhs = binaryExprRhsAsNumericExpr->value->evaluate({});
            if (constantValueOfBinaryExprRhs % 2 == 0) {
                const auto shiftAmount = (constantValueOfBinaryExprRhs / 2) + 1;
                const auto shiftOperation = *mappedToBinaryExpr == syrec_operation::operation::multiplication ? syrec_operation::operation::shift_left : syrec_operation::operation::shift_right;
                exprAsBinaryExpr->op  = *parser::ParserUtilities::mapOperationToInternalFlag(shiftOperation);
                exprAsBinaryExpr->rhs = std::make_shared<syrec::NumericExpression>(std::make_shared<syrec::Number>(shiftAmount), exprAsBinaryExpr->rhs->bitwidth());
                expr                  = exprAsBinaryExpr;
                return true;
            }
        }
    }
    return false;
    
}

