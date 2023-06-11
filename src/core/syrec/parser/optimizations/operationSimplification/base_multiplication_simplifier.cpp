#include "core/syrec/parser/optimizations/operationSimplification/base_multiplication_simplifier.hpp"
#include <core/syrec/parser/expression_evaluation_result.hpp>

using namespace optimizations;

std::optional<syrec::expression::ptr> BaseMultiplicationSimplifier::trySimplify(const std::shared_ptr<syrec::BinaryExpression>& binaryExpr) {
    if (!isOperationOfExpressionMultiplicationAndHasAtleastOneConstantOperand(binaryExpr)) {
        return std::nullopt;
    }

    const bool isLeftOperandConstant  = isOperandConstant(binaryExpr->lhs);
    const bool isRightOperandConstant = isOperandConstant(binaryExpr->rhs);
    if (isLeftOperandConstant && isRightOperandConstant) {
        const auto& leftOperandValue       = std::dynamic_pointer_cast<syrec::NumericExpression>(binaryExpr->lhs)->value->evaluate({});
        const auto& rightOperandValue      = std::dynamic_pointer_cast<syrec::NumericExpression>(binaryExpr->rhs)->value->evaluate({});
        const auto  finalValueOfExpression = leftOperandValue * rightOperandValue;

        return std::make_optional(std::make_shared<syrec::NumericExpression>(std::make_shared<syrec::Number>(finalValueOfExpression), parser::ExpressionEvaluationResult::getRequiredBitWidthToStoreSignal(finalValueOfExpression)));
    }
    return std::nullopt;
}


bool BaseMultiplicationSimplifier::isOperationOfExpressionMultiplicationAndHasAtleastOneConstantOperand(const std::shared_ptr<syrec::BinaryExpression>& binaryExpr) const {
    return binaryExpr->op == syrec::BinaryExpression::Multiply && (isOperandConstant(binaryExpr->lhs) || isOperandConstant(binaryExpr->rhs));
}

bool BaseMultiplicationSimplifier::isOperandConstant(const syrec::expression::ptr& expressionOperand) const {
    if (const auto& operandAsNumericExpression = std::dynamic_pointer_cast<syrec::NumericExpression>(expressionOperand); operandAsNumericExpression != nullptr) {
        return operandAsNumericExpression->value->isConstant();
    }
    return false;
}


