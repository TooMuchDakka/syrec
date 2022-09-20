#include "core/syrec/parser/signal_evaluation_result.hpp"

using namespace syrec;

void SignalEvaluationResult::updateResultToVariableAccess(const VariableAccess::ptr& variableAccess) {
    this->evaluationResult.emplace(availableValueOptions(variableAccess));
}

bool SignalEvaluationResult::isConstant() const {
    return std::holds_alternative<Number::ptr>(this->evaluationResult.value());
}

bool SignalEvaluationResult::isValid() const {
    return this->evaluationResult.has_value();
}

bool SignalEvaluationResult::isVariableAccess() const {
    return std::holds_alternative<VariableAccess::ptr>(this->evaluationResult.value());
}

std::optional<VariableAccess::ptr> SignalEvaluationResult::getAsVariableAccess() {
    std::optional<VariableAccess::ptr> fetchedValue;
    if (isValid() && isVariableAccess()) {
        fetchedValue.emplace(std::get<VariableAccess::ptr>(this->evaluationResult.value()));
    }
    return fetchedValue;
}

std::optional<Number::ptr> SignalEvaluationResult::getAsNumber() {
    std::optional<Number::ptr> fetchedValue;
    if (isValid() && isConstant()) {
        fetchedValue.emplace(std::get<Number::ptr>(this->evaluationResult.value()));
    }
    return fetchedValue;
}

