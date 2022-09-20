#ifndef SIGNAL_EVALUATION_RESULT_HPP
#define SIGNAL_EVALUATION_RESULT_HPP

#include "./core/syrec/number.hpp"
#include "./core/syrec/variable.hpp"

#include <variant>

namespace syrec {
    class SignalEvaluationResult {
    public:
        typedef std::shared_ptr<SignalEvaluationResult> ptr;

        SignalEvaluationResult() = default;
        void updateResultToVariableAccess(const VariableAccess::ptr& variableAccess);

        bool isValid() const;
        bool isConstant() const;
        bool isVariableAccess() const;

        std::optional<VariableAccess::ptr> getAsVariableAccess();
        std::optional<Number::ptr> getAsNumber();

    private:
        typedef std::variant<VariableAccess::ptr, Number::ptr>        availableValueOptions;
        std::optional<availableValueOptions> evaluationResult;
    };
}


#endif