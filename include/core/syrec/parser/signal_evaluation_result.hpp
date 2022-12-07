#ifndef SIGNAL_EVALUATION_RESULT_HPP
#define SIGNAL_EVALUATION_RESULT_HPP

#include "./core/syrec/number.hpp"
#include "./core/syrec/variable.hpp"

#include <variant>

namespace parser {
    class SignalEvaluationResult {
    public:
        typedef std::shared_ptr<SignalEvaluationResult> ptr;

        [[nodiscard]] bool isConstant() const;
        [[nodiscard]] bool isVariableAccess() const;

        [[nodiscard]] std::optional<syrec::VariableAccess::ptr> getAsVariableAccess();
        [[nodiscard]] std::optional<syrec::Number::ptr>         getAsNumber();

        SignalEvaluationResult(const syrec::Number::ptr& number) : evaluationResult(number) {}
        SignalEvaluationResult(const syrec::VariableAccess::ptr& variableAccess) : evaluationResult(variableAccess) {}
    private:
        typedef std::variant<syrec::VariableAccess::ptr, syrec::Number::ptr>        availableValueOptions;
        availableValueOptions evaluationResult;

    };
}


#endif