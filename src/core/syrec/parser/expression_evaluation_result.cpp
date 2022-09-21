#include "core/syrec/parser/expression_evaluation_result.hpp"

using namespace syrec;

bool ExpressionEvaluationResult::hasValue() const {
    return this->evaluationResult.has_value();
}

bool ExpressionEvaluationResult::evaluatedToConstant() const {
    return hasValue() && this->isConstant;
}

std::optional<unsigned> ExpressionEvaluationResult::getAsConstant() const {
    return evaluatedToConstant() ? std::optional(std::get<ConstantValueAndBitwidthPair>(this->evaluationResult.value()).first) : std::nullopt;
}

std::optional<expression::ptr> ExpressionEvaluationResult::getOrConvertToExpression(const std::optional<unsigned int>& expectedExpressionBitWidth) const {
    if (!evaluatedToConstant()) {
        return std::optional(std::get<expression::ptr>(this->evaluationResult.value()));
    }

    const ConstantValueAndBitwidthPair constantValueAndBitwidth = std::get<ConstantValueAndBitwidthPair>(this->evaluationResult.value());
    const unsigned int bitWidthOfExpression = expectedExpressionBitWidth.has_value() ? expectedExpressionBitWidth.value() : constantValueAndBitwidth.second;
    
    const auto wrapperForConstant = std::make_shared<Number>(Number(constantValueAndBitwidth.first));
    const auto finalWrapper = std::make_shared <NumericExpression> (NumericExpression(wrapperForConstant, bitWidthOfExpression));
    return std::optional(finalWrapper);
    
}

void ExpressionEvaluationResult::setResult(const unsigned constantValue, const std::optional<unsigned int>& bitwidth) {
    if (hasValue()) {
        this->evaluationResult.reset();
    }
    const ConstantValueAndBitwidthPair pair(constantValue, bitwidth.has_value() ? bitwidth.value() : 1u);
    this->evaluationResult.emplace(AvailableOptions(pair));
    this->isConstant = true;
}

void ExpressionEvaluationResult::setResult(const expression::ptr& expression) {
    if (hasValue()) {
        this->evaluationResult.reset();
    }

    const NumericExpression* constantExpression   = dynamic_cast<NumericExpression*>(expression.get());

    // TODO: We currently skip evaluating the value of a loop variable (this optimization should already be done in the parser)
    if (nullptr != constantExpression && constantExpression->value->isConstant()) {
        const ConstantValueAndBitwidthPair pair(constantExpression->value->evaluate({}), constantExpression->bitwidth());
        this->evaluationResult.emplace(pair);
        this->isConstant = true;
    } else {
        this->evaluationResult.emplace(AvailableOptions(expression));
        this->isConstant = false;
    }

}




