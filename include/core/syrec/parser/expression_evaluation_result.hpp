#ifndef EXPRESSION_EVALUATION_RESULT_HPP
#define EXPRESSION_EVALUATION_RESULT_HPP

#include "core/syrec/expression.hpp"

#include <variant>

// https://stackoverflow.com/questions/69652744/c-singleton-private-constructor-not-accessible-from-static-function?noredirect=1&lq=1

namespace parser {
    class ExpressionEvaluationResult {
    public:
        typedef std::shared_ptr <ExpressionEvaluationResult> ptr;

        std::vector<unsigned int> numValuesPerDimension;

        [[nodiscard]] bool isConstantValue() const;
        [[nodiscard]] std::optional<unsigned int> getAsConstant() const;
        [[nodiscard]] std::optional<syrec::expression::ptr> getAsExpression() const;

        // TODO: Usage of std::make_shared<T> requires a public constructor for T
        // To still use this pattern, and avoid two allocations via std::shared_ptr(new T(..)) use
        // @ https://stackoverflow.com/questions/8147027/how-do-i-call-stdmake-shared-on-a-class-with-only-protected-or-private-const/8147213#8147213
        // https://stackoverflow.com/questions/45127107/private-constructor-and-make-shared
        [[nodiscard]] static ExpressionEvaluationResult::ptr createFromConstantValue(const unsigned int constantValue, const std::optional<unsigned int>& optionalExpectedSignalWidth)
        {
            return std::shared_ptr<ExpressionEvaluationResult>(new ExpressionEvaluationResult(constantValue, optionalExpectedSignalWidth));
        }
        [[nodiscard]] static ExpressionEvaluationResult::ptr createFromExpression(const syrec::expression::ptr& expression, std::vector<unsigned int> numValuesPerDimension) {
            return std::shared_ptr<ExpressionEvaluationResult>(new ExpressionEvaluationResult(expression, numValuesPerDimension));
        }

        [[nodiscard]] static unsigned int                           getRequiredBitWidthToStoreSignal(unsigned int constantValue);

    private:
        ExpressionEvaluationResult(unsigned int constantValue, const std::optional<unsigned int>& optionalExpectedSignalWidth):
            isConstant(true), optionalExpectedSignalWidthForConstantValue(optionalExpectedSignalWidth) {
            numValuesPerDimension.emplace_back(1);
            evaluationResult = std::make_pair(constantValue, optionalExpectedSignalWidth.has_value() ? *optionalExpectedSignalWidth : getRequiredBitWidthToStoreSignal(constantValue));
        }

        ExpressionEvaluationResult(const syrec::expression::ptr& expression, std::vector<unsigned int>& numValuesPerDimension):
            numValuesPerDimension(std::move(numValuesPerDimension)), isConstant(false) {
            evaluationResult = expression;
        }

        using ConstantValueAndBitwidthPair = std::pair<unsigned int, unsigned int>;
        using AvailableOptions = std::variant<syrec::expression::ptr, ConstantValueAndBitwidthPair>;

        bool isConstant;
        std::optional<AvailableOptions> evaluationResult;
        std::optional<unsigned int>     optionalExpectedSignalWidthForConstantValue;
    };
}

#endif