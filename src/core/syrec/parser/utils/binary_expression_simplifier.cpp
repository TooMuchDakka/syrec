#include "core/syrec/parser/utils/binary_expression_simplifier.hpp"
#include "core/syrec/parser/operation.hpp"
#include <core/syrec/parser/parser_utilities.hpp>

using namespace optimizations;

static BinaryExpressionSimplificationResult trySimplifyOptionallyOnlyTopLevelExpr(const std::variant<std::shared_ptr<syrec::ShiftExpression>, std::shared_ptr<syrec::BinaryExpression>>& exprToOptimize, bool onlyTopLevelExpr) {
    const auto&            isExprBinaryOne = std::holds_alternative<std::shared_ptr<syrec::BinaryExpression>>(exprToOptimize);

    syrec::expression::ptr referenceExpression;
    syrec::expression::ptr exprLhsOperand;
    syrec::expression::ptr exprRhsOperand;
    unsigned int           usedOperation;
    if (isExprBinaryOne) {
        const auto& exprAsBinaryOne = std::get<std::shared_ptr<syrec::BinaryExpression>>(exprToOptimize);
        referenceExpression         = exprAsBinaryOne;
        exprLhsOperand              = exprAsBinaryOne->lhs;
        exprRhsOperand              = exprAsBinaryOne->rhs;
        usedOperation               = exprAsBinaryOne->op;
    } else {
        const auto& exprAsShiftOne = std::get<std::shared_ptr<syrec::ShiftExpression>>(exprToOptimize);
        referenceExpression        = exprAsShiftOne;
        exprLhsOperand             = exprAsShiftOne->lhs;
        exprRhsOperand             = std::make_shared<syrec::NumericExpression>(exprAsShiftOne->rhs, exprAsShiftOne->bitwidth());
        usedOperation              = exprAsShiftOne->op;
    }

    const auto mappedFlagToEnum = parser::ParserUtilities::mapInternalBinaryOperationFlagToEnum(usedOperation);
    if (!mappedFlagToEnum.has_value()) {
        return BinaryExpressionSimplificationResult(false, referenceExpression);
    }

    std::optional<unsigned int> lExprAsConstantValue;
    std::optional<unsigned int> rExprAsConstantValue;

    if (const auto& lOperandAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(exprLhsOperand); lOperandAsNumericExpr != nullptr) {
        if (lOperandAsNumericExpr->value->isConstant()) {
            lExprAsConstantValue = lOperandAsNumericExpr->value->evaluate({});
        }
    }

    if (const auto& rOperandAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(exprRhsOperand); rOperandAsNumericExpr != nullptr) {
        if (rOperandAsNumericExpr->value->isConstant()) {
            rExprAsConstantValue = rOperandAsNumericExpr->value->evaluate({});
        }
    }
    
    if (lExprAsConstantValue.has_value() && rExprAsConstantValue.has_value()) {
        const auto evaluatedResult      = *syrec_operation::apply(*mappedFlagToEnum, *lExprAsConstantValue, *rExprAsConstantValue);
        const auto containerForResult   = std::make_shared<syrec::Number>(evaluatedResult);
        const auto simplificationResult = std::make_shared<syrec::NumericExpression>(containerForResult, referenceExpression->bitwidth());
        return BinaryExpressionSimplificationResult(true, simplificationResult);
    }

    if (lExprAsConstantValue.has_value() || rExprAsConstantValue.has_value()) {
        const bool isLeftOperandConstant = lExprAsConstantValue.has_value();
        if (isLeftOperandConstant && syrec_operation::isOperandUsedAsLhsInOperationIdentityElement(*mappedFlagToEnum, *lExprAsConstantValue)) {
            if (onlyTopLevelExpr) {
                return BinaryExpressionSimplificationResult(true, exprRhsOperand);
            }
            const auto& simplificationResultOfRhs = trySimplify(exprRhsOperand);
            return BinaryExpressionSimplificationResult(true, simplificationResultOfRhs.simplifiedExpression);   
        }
        if (!isLeftOperandConstant && syrec_operation::isOperandUseAsRhsInOperationIdentityElement(*mappedFlagToEnum, *rExprAsConstantValue)) {
            if (onlyTopLevelExpr) {
                return BinaryExpressionSimplificationResult(true, exprLhsOperand);
            }
            const auto& simplificationResultOfLhs = trySimplify(exprLhsOperand);
            return BinaryExpressionSimplificationResult(true, simplificationResultOfLhs.simplifiedExpression);   
        }

        std::optional<unsigned int> operationSpecificSimplificationResult;
        switch (*mappedFlagToEnum) {
            case syrec_operation::operation::division:
                if (isLeftOperandConstant && *lExprAsConstantValue == 0) {
                    operationSpecificSimplificationResult.emplace(0);
                }
                break;
            case syrec_operation::operation::bitwise_and:
            case syrec_operation::operation::logical_and:
            case syrec_operation::operation::multiplication:
                if ((isLeftOperandConstant && *lExprAsConstantValue == 0) 
                    || (!isLeftOperandConstant && *rExprAsConstantValue == 0)) {
                    operationSpecificSimplificationResult.emplace(0);
                }
                break;
            case syrec_operation::operation::shift_left:
            case syrec_operation::operation::shift_right:
                if (!isLeftOperandConstant && *rExprAsConstantValue == 0) {
                    const auto& simplificationResultOfNonConstantOperand = trySimplify(exprLhsOperand);
                    return BinaryExpressionSimplificationResult(true, simplificationResultOfNonConstantOperand.simplifiedExpression);
                }
                if (isLeftOperandConstant && *lExprAsConstantValue == 0) {
                    operationSpecificSimplificationResult.emplace(0);
                }
                break;
            default:
                break;
        }

        if (operationSpecificSimplificationResult.has_value()) {
            const auto containerForResult   = std::make_shared<syrec::Number>(*operationSpecificSimplificationResult);
            const auto simplificationResult = std::make_shared<syrec::NumericExpression>(containerForResult, referenceExpression->bitwidth());
            return BinaryExpressionSimplificationResult(true, simplificationResult);
        }
    }

    if (onlyTopLevelExpr) {
        return BinaryExpressionSimplificationResult(false, referenceExpression);
    }

    const auto& simplificationResultOfLhs = trySimplify(exprLhsOperand);
    const auto& simplificationResultOfRhs = trySimplify(exprRhsOperand);

    if (simplificationResultOfLhs.couldSimplify || simplificationResultOfRhs.couldSimplify) {
        const auto& newBinaryExpr = std::make_shared<syrec::BinaryExpression>(
                simplificationResultOfLhs.simplifiedExpression,
                *parser::ParserUtilities::mapOperationToInternalFlag(*mappedFlagToEnum),
                simplificationResultOfRhs.simplifiedExpression);

        const auto& simplificationResultOfNewTopLevelBinaryExpr = trySimplifyOptionallyOnlyTopLevelExpr(newBinaryExpr, true);
        return BinaryExpressionSimplificationResult(true, simplificationResultOfNewTopLevelBinaryExpr.simplifiedExpression);
    }
    return BinaryExpressionSimplificationResult(false, referenceExpression);

}

BinaryExpressionSimplificationResult optimizations::trySimplify(const syrec::expression::ptr& expr) {
    const std::shared_ptr<syrec::BinaryExpression> exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr);
    const std::shared_ptr<syrec::ShiftExpression> exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr);
    
    if (exprAsBinaryExpr != nullptr) {
        return trySimplifyOptionallyOnlyTopLevelExpr(exprAsBinaryExpr, false);
    }
    if (exprAsShiftExpr != nullptr) {
        return trySimplifyOptionallyOnlyTopLevelExpr(exprAsShiftExpr, false);
    }
    return BinaryExpressionSimplificationResult(false, expr);
}