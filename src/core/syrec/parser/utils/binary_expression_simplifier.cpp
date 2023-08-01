#include "core/syrec/parser/utils/binary_expression_simplifier.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/optimizations/operation_strength_reduction.hpp"

using namespace optimizations;

static BinaryExpressionSimplificationResult trySimplifyOptionallyOnlyTopLevelExpr(const std::variant<std::shared_ptr<syrec::ShiftExpression>, std::shared_ptr<syrec::BinaryExpression>>& exprToOptimize, bool onlyTopLevelExpr, bool shouldPerformOperationStrengthReduction) {
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

    auto mappedFlagToEnum = syrec_operation::tryMapBinaryOperationFlagToEnum(usedOperation);
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
            const auto& simplificationResultOfRhs = trySimplify(exprRhsOperand, shouldPerformOperationStrengthReduction);
            return BinaryExpressionSimplificationResult(true, simplificationResultOfRhs.simplifiedExpression);   
        }
        if (!isLeftOperandConstant && syrec_operation::isOperandUseAsRhsInOperationIdentityElement(*mappedFlagToEnum, *rExprAsConstantValue)) {
            if (onlyTopLevelExpr) {
                return BinaryExpressionSimplificationResult(true, exprLhsOperand);
            }
            const auto& simplificationResultOfLhs = trySimplify(exprLhsOperand, shouldPerformOperationStrengthReduction);
            return BinaryExpressionSimplificationResult(true, simplificationResultOfLhs.simplifiedExpression);   
        }

        std::optional<unsigned int> operationSpecificSimplificationResult;
        switch (*mappedFlagToEnum) {
            case syrec_operation::operation::Division:
                if (isLeftOperandConstant && *lExprAsConstantValue == 0) {
                    operationSpecificSimplificationResult.emplace(0);
                }
                break;
            case syrec_operation::operation::BitwiseAnd:
            case syrec_operation::operation::LogicalAnd:
            case syrec_operation::operation::Multiplication:
                if ((isLeftOperandConstant && *lExprAsConstantValue == 0) 
                    || (!isLeftOperandConstant && *rExprAsConstantValue == 0)) {
                    operationSpecificSimplificationResult.emplace(0);
                }
                break;
            case syrec_operation::operation::ShiftLeft:
            case syrec_operation::operation::ShiftRight:
                if (!isLeftOperandConstant && *rExprAsConstantValue == 0) {
                    const auto& simplificationResultOfNonConstantOperand = trySimplify(exprLhsOperand, shouldPerformOperationStrengthReduction);
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

        // TODO: Do we need to refetch the rhs expression ? We would expect that it is updated automatically since we are only holding references to it ?
        /*
         * Since operation strength can change the operation of a binary expression, we need to update the used operation of the binary expression
         */
        if (tryPerformOperationStrengthReduction(referenceExpression)) {
            mappedFlagToEnum = syrec_operation::tryMapBinaryOperationFlagToEnum(usedOperation);
            if (!mappedFlagToEnum.has_value()) {
                return BinaryExpressionSimplificationResult(false, referenceExpression);
            }
        }
    }

    if (onlyTopLevelExpr) {
        return BinaryExpressionSimplificationResult(false, referenceExpression);
    }

    const auto& simplificationResultOfLhs = trySimplify(exprLhsOperand, shouldPerformOperationStrengthReduction);
    const auto& simplificationResultOfRhs = trySimplify(exprRhsOperand, shouldPerformOperationStrengthReduction);

    if (simplificationResultOfLhs.couldSimplify || simplificationResultOfRhs.couldSimplify) {
        const auto& newBinaryExpr = std::make_shared<syrec::BinaryExpression>(
                simplificationResultOfLhs.simplifiedExpression,
                *syrec_operation::tryMapBinaryOperationEnumToFlag(*mappedFlagToEnum),
                simplificationResultOfRhs.simplifiedExpression);

        const auto& simplificationResultOfNewTopLevelBinaryExpr = trySimplifyOptionallyOnlyTopLevelExpr(newBinaryExpr, true, shouldPerformOperationStrengthReduction);
        return BinaryExpressionSimplificationResult(true, simplificationResultOfNewTopLevelBinaryExpr.simplifiedExpression);
    }
    return BinaryExpressionSimplificationResult(false, referenceExpression);

}

BinaryExpressionSimplificationResult optimizations::trySimplify(const syrec::expression::ptr& expr, bool shouldPerformOperationStrengthReduction) {
    const std::shared_ptr<syrec::BinaryExpression> exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr);
    const std::shared_ptr<syrec::ShiftExpression> exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr);
    
    if (exprAsBinaryExpr != nullptr) {
        return trySimplifyOptionallyOnlyTopLevelExpr(exprAsBinaryExpr, false, shouldPerformOperationStrengthReduction);
    }
    if (exprAsShiftExpr != nullptr) {
        return trySimplifyOptionallyOnlyTopLevelExpr(exprAsShiftExpr, false, shouldPerformOperationStrengthReduction);
    }
    return BinaryExpressionSimplificationResult(false, expr);
}