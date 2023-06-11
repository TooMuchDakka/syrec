#include "core/syrec/parser/optimizations/operationSimplification/binary_multiplication_simplifier.hpp"
#include "core/syrec/parser/expression_evaluation_result.hpp"

using namespace optimizations;

std::optional<syrec::expression::ptr> BinaryMultiplicationSimplifier::trySimplify(const std::shared_ptr<syrec::BinaryExpression>& binaryExpr) {
    if (!isOperationOfExpressionMultiplicationAndHasAtleastOneConstantOperand(binaryExpr)) {
        const auto lhsOperandAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(binaryExpr->lhs);
        const auto simplificationResultOfLhsOperand = lhsOperandAsBinaryExpr != nullptr ? trySimplify(lhsOperandAsBinaryExpr) : std::nullopt;

        const auto rhsOperandAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(binaryExpr->rhs);
        const auto simplificationResultOfRhsOperand = rhsOperandAsBinaryExpr != nullptr ? trySimplify(rhsOperandAsBinaryExpr) : std::nullopt;

        if (simplificationResultOfLhsOperand.has_value() || simplificationResultOfRhsOperand.has_value()) {
            return std::make_optional(std::make_shared<syrec::BinaryExpression>(
                simplificationResultOfLhsOperand.value_or(binaryExpr->lhs), 
                binaryExpr->op, 
                simplificationResultOfRhsOperand.value_or(binaryExpr->rhs)));
        }
        return std::nullopt;
    }

    if (const auto& simplificationResultWithBaseFunctionality = BaseMultiplicationSimplifier::trySimplify(binaryExpr); simplificationResultWithBaseFunctionality.has_value()) {
        return simplificationResultWithBaseFunctionality;
    }

    const bool isLeftOperandConstant = isOperandConstant(binaryExpr->lhs);
    const auto valueOfConstantOperand = std::dynamic_pointer_cast<syrec::NumericExpression>(isLeftOperandConstant ? binaryExpr->lhs : binaryExpr->rhs)->value->evaluate({});
    auto       nonConstantOperand     = isLeftOperandConstant ? binaryExpr->rhs : binaryExpr->lhs;

    const auto simplificationSteps = cache.count(valueOfConstantOperand) != 0
    ? cache.at(valueOfConstantOperand)
    : generateOperationSteps(valueOfConstantOperand, false);

    if (simplificationSteps.empty()) {
        return std::make_optional(binaryExpr);
    }

    if (cache.count(valueOfConstantOperand) == 0) {
        cache.insert(std::make_pair(valueOfConstantOperand, simplificationSteps));
    }

    if (const auto nonConstantOperandAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(nonConstantOperand); nonConstantOperandAsBinaryExpr != nullptr) {
        const auto& simplificationResultOfNonConstantOperand = trySimplify(nonConstantOperandAsBinaryExpr);
        if (simplificationResultOfNonConstantOperand.has_value()) {
            nonConstantOperand = *simplificationResultOfNonConstantOperand;
        }
    }
    return std::make_optional(generateExpressionFromSimplificationSteps(simplificationSteps, nonConstantOperand));
}

std::vector<std::size_t> BinaryMultiplicationSimplifier::extractOneBitPositionsOfNumber(unsigned number) {
    std::vector<std::size_t> oneBitsOfNumber;
    const auto& numRequiredBitsToStoreNumber = number == 0 ? 1 : std::floor(std::log2(number)) + 1;
    for (auto i = 0; i < numRequiredBitsToStoreNumber; ++i) {
        if (static_cast<bool>((number & (1 << i)) >> i)) {
            oneBitsOfNumber.emplace_back(i);
        }
    }
    return oneBitsOfNumber;
}

std::size_t BinaryMultiplicationSimplifier::determineShiftAmount(std::size_t currentOneBitPosition, std::optional<std::size_t> previousOneBitPosition) {
    if (previousOneBitPosition.has_value()) {
        return currentOneBitPosition - *previousOneBitPosition;
    }
    return 1;
}

void BinaryMultiplicationSimplifier::reverseAndUpdateBitPositionsOfNumber(std::vector<std::size_t>& oneBitPositionsOfNumber) {
    const auto idxOfMSBOneBit = oneBitPositionsOfNumber.back();
    std::reverse(oneBitPositionsOfNumber.begin(), oneBitPositionsOfNumber.end());
    for (auto& oneBitPosition : oneBitPositionsOfNumber) {
        oneBitPosition = idxOfMSBOneBit - oneBitPosition;
    }
}


std::vector<BinaryMultiplicationSimplifier::ShiftAndAddOrSubOperationStep> BinaryMultiplicationSimplifier::generateOperationSteps(std::size_t constantToSimplify, bool aggregateOneBitSequences) {
    if (constantToSimplify == 0 || constantToSimplify == 1) {
        return {};
    }

    auto oneBitsOfConstantOperand = extractOneBitPositionsOfNumber(static_cast<unsigned int>(constantToSimplify));
    reverseAndUpdateBitPositionsOfNumber(oneBitsOfConstantOperand);

    const auto& lastOneBitToCheck        = std::prev(oneBitsOfConstantOperand.end());
    std::vector<BinaryMultiplicationSimplifier::ShiftAndAddOrSubOperationStep> operationSteps;
    std::optional<std::size_t>                                                 prevOneBitPosition;

    for (auto oneBitPositionIterator = oneBitsOfConstantOperand.begin(); oneBitPositionIterator != lastOneBitToCheck; ++oneBitPositionIterator) {
        operationSteps.emplace_back(ShiftAndAddOrSubOperationStep(determineShiftAmount(*oneBitPositionIterator, prevOneBitPosition), true));
        prevOneBitPosition = *oneBitPositionIterator;
    }

    return operationSteps;
}

syrec::BinaryExpression::ptr BinaryMultiplicationSimplifier::generateExpressionFromSimplificationSteps(const std::vector<ShiftAndAddOrSubOperationStep>& simplificationStepsOfConstantOperand, const syrec::expression::ptr& nonConstantOperand) {
    auto nonConstantShiftExprOperand = nonConstantOperand;
    for (const auto& simplificationStep : simplificationStepsOfConstantOperand) {
        const auto generatedShiftExpr    = std::make_shared<syrec::ShiftExpression>(nonConstantShiftExprOperand, syrec::ShiftExpression::Left, std::make_shared<syrec::Number>(simplificationStep.shiftAmount));
        const auto generatedAdditionExpr = std::make_shared<syrec::BinaryExpression>(generatedShiftExpr, syrec::BinaryExpression::Add, nonConstantOperand);
        nonConstantShiftExprOperand      = generatedAdditionExpr;
    }
    return nonConstantShiftExprOperand;
}

