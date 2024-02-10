#include "core/syrec/parser/optimizations/operationSimplification/binary_factoring_multiplication_simplifier.hpp"
#include "core/syrec/parser/utils/bit_helpers.hpp"

using namespace optimizations;

/*
 * TODO: Currently after factoring the given integer a to i.e. a_0 * a_1, the subsequent trySimplifiy call of the generated expression from the found factors will perform a factorization again (which is currently not desired behaviour)
 * For example: 9 could be factorized to 3 * 3 => (((x << 1) + 1) << 1 + ((x << 1) + 1)) but the initial factor would already produce the sequence ((x << 3) + 1) thus the proposed fusion approach of the normal and factorizing method should work best
 */
std::optional<syrec::Expression::ptr> BinaryFactoringMultiplicationSimplifier::trySimplify(const std::shared_ptr<syrec::BinaryExpression>& binaryExpr) {
    const bool isLeftOperandConstant  = isOperandConstant(binaryExpr->lhs);
    const bool isRightOperandConstant = isOperandConstant(binaryExpr->rhs);

    if (!isOperationOfExpressionMultiplicationAndHasAtleastOneConstantOperand(binaryExpr) || !(isLeftOperandConstant ^ isRightOperandConstant)) {
        const auto lhsOperandAsBinaryExpr           = std::dynamic_pointer_cast<syrec::BinaryExpression>(binaryExpr->lhs);
        const auto simplificationResultOfLhsOperand = lhsOperandAsBinaryExpr != nullptr ? trySimplify(lhsOperandAsBinaryExpr) : std::nullopt;

        const auto rhsOperandAsBinaryExpr           = std::dynamic_pointer_cast<syrec::BinaryExpression>(binaryExpr->rhs);
        const auto simplificationResultOfRhsOperand = rhsOperandAsBinaryExpr != nullptr ? trySimplify(rhsOperandAsBinaryExpr) : std::nullopt;

        if (simplificationResultOfLhsOperand.has_value() || simplificationResultOfRhsOperand.has_value()) {
            return std::make_optional(std::make_shared<syrec::BinaryExpression>(
                    simplificationResultOfLhsOperand.value_or(binaryExpr->lhs),
                    binaryExpr->op,
                    simplificationResultOfRhsOperand.value_or(binaryExpr->rhs)));
        }
        return std::nullopt;
    }

    const auto constantOperand    = isLeftOperandConstant ? binaryExpr->lhs : binaryExpr->rhs;
    const auto nonConstantOperand = isLeftOperandConstant ? binaryExpr->rhs : binaryExpr->lhs;
    const auto valueOfConstant    = *tryDetermineValueOfConstant(constantOperand);

    if (const auto& shiftExpressionIfConstantTermIsPowerOfTwo = replaceMultiplicationWithShiftIfConstantTermIsPowerOfTwo(valueOfConstant, nonConstantOperand); shiftExpressionIfConstantTermIsPowerOfTwo.has_value()) {
        return shiftExpressionIfConstantTermIsPowerOfTwo;
    }

    const auto& factorTermsOfConstant = factorizeInteger(valueOfConstant);
    if (factorTermsOfConstant.empty()) {
        return std::nullopt;
    }

    syrec::Expression::ptr toBeMultipliedOperand = nonConstantOperand;
    for (const auto factorTerm: factorTermsOfConstant) {
        const auto exprForFactorTerm = std::make_shared<syrec::Number>(factorTerm);
        toBeMultipliedOperand        = std::make_shared<syrec::BinaryExpression>(
                std::make_shared<syrec::NumericExpression>(exprForFactorTerm, BitHelpers::getRequiredBitsToStoreValue(factorTerm)),
                syrec::BinaryExpression::Multiply,
                toBeMultipliedOperand);
    }
    return BinaryMultiplicationSimplifier::trySimplify(std::dynamic_pointer_cast<syrec::BinaryExpression>(toBeMultipliedOperand));
}


std::vector<unsigned> BinaryFactoringMultiplicationSimplifier::factorizeInteger(unsigned integer) {
    if (factorizingCache.count(integer) != 0) {
        return factorizingCache[integer];
    }

    std::vector<unsigned int> generatedFactors;
    const auto                requiredNumberOfBitsToStoreValue = BitHelpers::getRequiredBitsToStoreValue(integer) - 1;

    auto potentialFactorBase = static_cast<unsigned int>(std::pow(2, requiredNumberOfBitsToStoreValue));
    for (std::size_t i = 0; i < requiredNumberOfBitsToStoreValue && integer > 0; i++) {
        if (const auto foundFactor = returnVariationOfFactorThatDividesNumber(integer, potentialFactorBase); foundFactor.has_value() && *foundFactor != integer && *foundFactor > 1) {
            integer /= *foundFactor;
            generatedFactors.emplace_back(*foundFactor);
            const auto& factorizationResultOfRemainder = factorizeInteger(integer);
            for (const auto factor: factorizationResultOfRemainder) {
                generatedFactors.emplace_back(factor);
            }
            break;
        }
        potentialFactorBase >>= 1;
    }

    if (generatedFactors.empty()) {
        generatedFactors.emplace_back(integer);
    } else {
        combineDuplicateFactors(generatedFactors);
    }

    factorizingCache.insert(std::make_pair(integer, generatedFactors));
    return generatedFactors;
}

bool BinaryFactoringMultiplicationSimplifier::isNumberDividedByWithoutRemainder(unsigned integer, unsigned divisor) {
    return integer % divisor == 0;
}

std::optional<unsigned> BinaryFactoringMultiplicationSimplifier::returnVariationOfFactorThatDividesNumber(unsigned integer, unsigned potentialFactorBase) {
    auto variantOfFactor = potentialFactorBase + 1;
    if (isNumberDividedByWithoutRemainder(integer, variantOfFactor)) {
        return std::make_optional(variantOfFactor);
    }
    variantOfFactor = potentialFactorBase - 1;
    if (isNumberDividedByWithoutRemainder(integer, variantOfFactor)) {
        return std::make_optional(variantOfFactor);
    }
    return std::nullopt;
}

void BinaryFactoringMultiplicationSimplifier::combineDuplicateFactors(std::vector<unsigned>& foundFactors) {
    for (auto factorToCheck = foundFactors.begin(); factorToCheck < std::prev(foundFactors.end());) {
        if (std::next(factorToCheck) == foundFactors.end() || *factorToCheck != *std::next(factorToCheck)) {
            ++factorToCheck;
            continue;
        }

        /* 
         * a_i, a_(i + 1), ... with a_i == a_(i + 1)
         * will be replaced with a_i being removed and
         * a_(i + 1) *= a_i
         * 
         */
        *std::next(factorToCheck) = static_cast<unsigned int>(std::pow(*factorToCheck, 2));
        factorToCheck = foundFactors.erase(factorToCheck);
    }
}