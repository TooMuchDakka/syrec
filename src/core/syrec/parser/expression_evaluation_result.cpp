#include <algorithm>
#include <cmath>
#include "core/syrec/parser/expression_evaluation_result.hpp"

using namespace parser;

 // TODO: Usage of std::make_shared<T> requires a public constructor for T
// To still use this pattern, and avoid two allocations via std::shared_ptr(new T(..)) use
// @ https://stackoverflow.com/questions/8147027/how-do-i-call-stdmake-shared-on-a-class-with-only-protected-or-private-const/8147213#8147213
// https://stackoverflow.com/questions/45127107/private-constructor-and-make-shared
ExpressionEvaluationResult::ptr ExpressionEvaluationResult::createFromConstantValue(const unsigned int constantValue, const std::optional<unsigned int>& optionalExpectedSignalWidth) {
    return std::shared_ptr<ExpressionEvaluationResult>(new ExpressionEvaluationResult(constantValue, optionalExpectedSignalWidth));
}

ExpressionEvaluationResult::ptr ExpressionEvaluationResult::createFromExpression(const syrec::Expression::ptr& expression, const std::vector<std::optional<unsigned int>>& explicitlyAccessedValuesPerDimension) {
    if (const auto& exprAsSignalAccessExpr = std::dynamic_pointer_cast<syrec::VariableExpression>(expression); exprAsSignalAccessExpr != nullptr) {
        return createFromExprDefiningSignalAccess(exprAsSignalAccessExpr, explicitlyAccessedValuesPerDimension);
    }
    return std::shared_ptr<ExpressionEvaluationResult>(new ExpressionEvaluationResult(expression, 1, {}, {1}));
}

std::vector<unsigned> ExpressionEvaluationResult::OperandSizeInformation::determineNumAccessedValuesPerDimension() const {
    std::vector<unsigned int> resultContainer( numDeclaredDimensionOfOperand, 1);
    std::copy(numValuesPerNotExplicitlyAccessedDimension.cbegin(), numValuesPerNotExplicitlyAccessedDimension.cend(), std::next(resultContainer.begin(), numDeclaredDimensionOfOperand - numValuesPerNotExplicitlyAccessedDimension.size()));
    return resultContainer;
}

bool ExpressionEvaluationResult::isConstantValue() const {
    return this->evaluationResult.has_value() && this->isConstant;
}

std::optional<unsigned int> ExpressionEvaluationResult::getAsConstant() const {
    if (!isConstantValue()) {
        return std::nullopt;   
    }
    return std::make_optional(std::get<ConstantValueAndBitwidthPair>(this->evaluationResult.value()).first);
}

std::optional<syrec::Expression::ptr> ExpressionEvaluationResult::getAsExpression() const {
    if (!this->evaluationResult.has_value()) {
        return std::nullopt;
    }

    if (!isConstantValue()) {
        return std::make_optional(std::get<syrec::Expression::ptr>(this->evaluationResult.value()));
    }

    const auto& constantValueAndBitwidthPair = std::get<ConstantValueAndBitwidthPair>(this->evaluationResult.value());
    const auto& containerForConstantValue    = std::make_shared<syrec::Number>(constantValueAndBitwidthPair.first);
    const auto& generatedExpression          = std::make_shared<syrec::NumericExpression>(containerForConstantValue, constantValueAndBitwidthPair.second);
    return std::make_optional(generatedExpression);
}

ExpressionEvaluationResult::ptr ExpressionEvaluationResult::createFromExprDefiningSignalAccess(const syrec::VariableExpression::ptr& exprDefiningSignalAccess, const std::vector<std::optional<unsigned int>>& explicitlyAccessedValuesPerDimension) {
    // We already verified prior to this call that the given variable expression is actually a variable expression (since the given parameter type is std::shared_ptr<syrec::expression> and not std::shared_ptr<syrec::VariableExpression>
    const auto& exprCasted                    = std::dynamic_pointer_cast<syrec::VariableExpression>(exprDefiningSignalAccess);
    const auto& definedSignalAccessOfExpr     = exprCasted->var;
    const auto& numDeclaredDimensionsOfSignal = definedSignalAccessOfExpr->var->dimensions.size();

    std::vector<unsigned int> numValuesPerNotExplicitlyAccessedDimension;
    if (explicitlyAccessedValuesPerDimension.size() < numDeclaredDimensionsOfSignal) {
        numValuesPerNotExplicitlyAccessedDimension.reserve(numDeclaredDimensionsOfSignal - explicitlyAccessedValuesPerDimension.size());
        for (std::size_t i = explicitlyAccessedValuesPerDimension.size(); i < numDeclaredDimensionsOfSignal; ++i) {
            numValuesPerNotExplicitlyAccessedDimension.emplace_back(definedSignalAccessOfExpr->var->dimensions.at(i));
        }
    }
    return std::shared_ptr<ExpressionEvaluationResult>(new ExpressionEvaluationResult(exprDefiningSignalAccess, numDeclaredDimensionsOfSignal, explicitlyAccessedValuesPerDimension, numValuesPerNotExplicitlyAccessedDimension));
}