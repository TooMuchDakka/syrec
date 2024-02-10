#include "core/syrec/parser/optimizations/operationSimplification/base_multiplication_simplifier.hpp"
#include "core/syrec/parser/utils/bit_helpers.hpp"

using namespace optimizations;

std::optional<syrec::Expression::ptr> BaseMultiplicationSimplifier::trySimplify(const std::shared_ptr<syrec::BinaryExpression>& binaryExpr) {
    if (!isOperationOfExpressionMultiplicationAndHasAtleastOneConstantOperand(binaryExpr)) {
        return std::nullopt;
    }

    const auto valueOfLeftOperandIfConstant = tryDetermineValueOfConstant(binaryExpr->lhs);
    const auto valueOfRightOperandIfConstant = tryDetermineValueOfConstant(binaryExpr->rhs);

    if (valueOfLeftOperandIfConstant.has_value() && valueOfRightOperandIfConstant.has_value()) {
        const auto resultOfProduct = *valueOfLeftOperandIfConstant * *valueOfRightOperandIfConstant;
        return std::make_optional(std::make_shared<syrec::NumericExpression>(std::make_shared<syrec::Number>(resultOfProduct), BitHelpers::getRequiredBitsToStoreValue(resultOfProduct)));
    }
    return std::nullopt;
}


bool BaseMultiplicationSimplifier::isOperationOfExpressionMultiplicationAndHasAtleastOneConstantOperand(const std::shared_ptr<syrec::BinaryExpression>& binaryExpr) const {
    return binaryExpr->op == syrec::BinaryExpression::Multiply && (isOperandConstant(binaryExpr->lhs) || isOperandConstant(binaryExpr->rhs));
}

bool BaseMultiplicationSimplifier::isOperandConstant(const syrec::Expression::ptr& expressionOperand) const {
    if (const auto& operandAsNumericExpression = std::dynamic_pointer_cast<syrec::NumericExpression>(expressionOperand); operandAsNumericExpression != nullptr) {
        return operandAsNumericExpression->value->isConstant();
    }
    return false;
}

std::optional<unsigned> BaseMultiplicationSimplifier::tryDetermineValueOfConstant(const syrec::Expression::ptr& operandOfBinaryExpr) const {
    if (const auto operandAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(operandOfBinaryExpr); operandAsNumericExpr != nullptr) {
        if (operandAsNumericExpr->value->isConstant()) {
            return std::make_optional(operandAsNumericExpr->value->evaluate({}));
        }
    }
    return std::nullopt;
}

std::optional<syrec::Expression::ptr> BaseMultiplicationSimplifier::replaceMultiplicationWithShiftIfConstantTermIsPowerOfTwo(unsigned int constantTerm, const syrec::Expression::ptr& nonConstantTerm) const {
    if (constantTerm % 2 != 0 || constantTerm == 0) {
        return std::nullopt;
    }

    const auto shiftAmount = static_cast<unsigned int>(std::log2(constantTerm));
    return std::make_optional(std::make_shared<syrec::ShiftExpression>(nonConstantTerm, syrec::ShiftExpression::Left, std::make_shared<syrec::Number>(shiftAmount)));
}




