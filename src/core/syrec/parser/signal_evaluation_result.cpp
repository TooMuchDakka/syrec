#include "core/syrec/parser/signal_evaluation_result.hpp"

using namespace parser;
bool SignalEvaluationResult::isConstant() const {
    return std::holds_alternative<syrec::Number::ptr>(this->evaluationResult);
}

bool SignalEvaluationResult::isVariableAccess() const {
    return std::holds_alternative<syrec::VariableAccess::ptr>(this->evaluationResult);
}

std::optional<syrec::VariableAccess::ptr> SignalEvaluationResult::getAsVariableAccess() {
    std::optional<syrec::VariableAccess::ptr> fetchedValue;
    if (isVariableAccess()) {
        fetchedValue.emplace(std::get<syrec::VariableAccess::ptr>(this->evaluationResult));
    }
    return fetchedValue;
}

std::optional<syrec::Number::ptr> SignalEvaluationResult::getAsNumber() {
    std::optional<syrec::Number::ptr> fetchedValue;
    if (isConstant()) {
        fetchedValue.emplace(std::get<syrec::Number::ptr>(this->evaluationResult));
    }
    return fetchedValue;
}

