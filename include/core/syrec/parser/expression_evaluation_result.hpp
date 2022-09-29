#ifndef EXPRESSION_EVALUATION_RESULT_HPP
#define EXPRESSION_EVALUATION_RESULT_HPP

#include "core/syrec/expression.hpp"

#include <variant>

namespace parser {
    class ExpressionEvaluationResult {
    public:
        typedef std::shared_ptr <ExpressionEvaluationResult> ptr;

        ExpressionEvaluationResult() : isConstant(false) {
            
        }

        [[nodiscard]] bool hasValue() const;
        [[nodiscard]] bool evaluatedToConstant() const;
        [[nodiscard]] std::optional<unsigned int> getAsConstant() const;
        [[nodiscard]] std::optional<syrec::expression::ptr> getOrConvertToExpression(const std::optional<unsigned int>& expectedExpressionBitWidth) const;


        void setResult(unsigned int constantValue, const std::optional<unsigned int>& bitwidth);
        void setResult(const syrec::expression::ptr& expression);

    private:
        using ConstantValueAndBitwidthPair = std::pair<unsigned int, unsigned int>;
        using AvailableOptions = std::variant<syrec::expression::ptr, ConstantValueAndBitwidthPair>;

        bool isConstant;
        std::optional<AvailableOptions> evaluationResult;
    };
}

#endif