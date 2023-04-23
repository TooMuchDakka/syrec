#include <cmath>

#include "core/syrec/parser/expression_evaluation_result.hpp"

using namespace parser;

bool ExpressionEvaluationResult::isConstantValue() const {
    return this->evaluationResult.has_value() && this->isConstant;
}

std::optional<unsigned int> ExpressionEvaluationResult::getAsConstant() const {
    if (!isConstantValue()) {
        return std::nullopt;   
    }
    return std::make_optional(std::get<ConstantValueAndBitwidthPair>(this->evaluationResult.value()).first);
}

std::optional<syrec::expression::ptr> ExpressionEvaluationResult::getAsExpression() const {
    if (!this->evaluationResult.has_value()) {
        return std::nullopt;
    }

    if (!isConstantValue()) {
        return std::make_optional(std::get<syrec::expression::ptr>(this->evaluationResult.value()));
    }

    const auto& constantValueAndBitwidthPair = std::get<ConstantValueAndBitwidthPair>(this->evaluationResult.value());
    const auto& containerForConstantValue    = std::make_shared<syrec::Number>(constantValueAndBitwidthPair.first);
    const auto& generatedExpression          = std::make_shared<syrec::NumericExpression>(containerForConstantValue, constantValueAndBitwidthPair.second);
    return std::make_optional(generatedExpression);
}

// TODO:
unsigned ExpressionEvaluationResult::getRequiredBitWidthToStoreSignal(unsigned int constantValue) {
    if (constantValue == 0) {
        return 1;
    }

    return static_cast<unsigned int>(std::log2(constantValue) + 1);
}