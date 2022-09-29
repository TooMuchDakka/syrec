#ifndef SIGNAL_EVALUATION_RESULT_HPP
#define SIGNAL_EVALUATION_RESULT_HPP

#include "./core/syrec/number.hpp"
#include "./core/syrec/variable.hpp"

#include <variant>

namespace parser {
    class SignalEvaluationResult {
    public:
        typedef std::shared_ptr<SignalEvaluationResult> ptr;

        SignalEvaluationResult() = default;
        void updateResultToVariableAccess(const syrec::VariableAccess::ptr& variableAccess);

        bool isValid() const;
        bool isConstant() const;
        bool isVariableAccess() const;

        std::optional<syrec::VariableAccess::ptr> getAsVariableAccess();
        std::optional<syrec::Number::ptr> getAsNumber();

    private:
        typedef std::variant<syrec::VariableAccess::ptr, syrec::Number::ptr>        availableValueOptions;
        std::optional<availableValueOptions> evaluationResult;
    };
}


#endif