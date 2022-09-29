#include "core/syrec/parser/signal_evaluation_result.hpp"

using namespace parser;
void SignalEvaluationResult::updateResultToVariableAccess(const syrec::VariableAccess::ptr& variableAccess) {
    if (isValid()) {
        this->evaluationResult.reset();
    }
    this->evaluationResult.emplace(availableValueOptions(variableAccess));
}

bool SignalEvaluationResult::isConstant() const {
    return std::holds_alternative<syrec::Number::ptr>(this->evaluationResult.value());
}

bool SignalEvaluationResult::isValid() const {
    return this->evaluationResult.has_value();
}

bool SignalEvaluationResult::isVariableAccess() const {
    return std::holds_alternative<syrec::VariableAccess::ptr>(this->evaluationResult.value());
}

std::optional<syrec::VariableAccess::ptr> SignalEvaluationResult::getAsVariableAccess() {
    std::optional<syrec::VariableAccess::ptr> fetchedValue;
    if (isValid() && isVariableAccess()) {
        fetchedValue.emplace(std::get<syrec::VariableAccess::ptr>(this->evaluationResult.value()));
    }
    return fetchedValue;
}

std::optional<syrec::Number::ptr> SignalEvaluationResult::getAsNumber() {
    std::optional<syrec::Number::ptr> fetchedValue;
    if (isValid() && isConstant()) {
        fetchedValue.emplace(std::get<syrec::Number::ptr>(this->evaluationResult.value()));
    }
    return fetchedValue;
}

