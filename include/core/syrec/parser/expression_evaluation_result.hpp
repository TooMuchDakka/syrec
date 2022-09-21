#ifndef EXPRESSION_EVALUATION_RESULT_HPP
#define EXPRESSION_EVALUATION_RESULT_HPP

#include "core/syrec/expression.hpp"

#include <variant>

namespace syrec {
    class ExpressionEvaluationResult {
    public:
        typedef std::shared_ptr <ExpressionEvaluationResult> ptr;

        ExpressionEvaluationResult() : isConstant(false) {
            
        }

        [[nodiscard]] bool hasValue() const;
        [[nodiscard]] bool evaluatedToConstant() const;
        [[nodiscard]] std::optional<unsigned int> getAsConstant() const;
        [[nodiscard]] std::optional<expression::ptr> getOrConvertToExpression(const std::optional<unsigned int>& expectedExpressionBitWidth) const;


        void setResult(unsigned int constantValue, const std::optional<unsigned int>& bitwidth);
        void setResult(const expression::ptr& expression);

    private:
        using ConstantValueAndBitwidthPair = std::pair<unsigned int, unsigned int>;
        using AvailableOptions = std::variant<expression::ptr, ConstantValueAndBitwidthPair>;

        bool isConstant;
        std::optional<AvailableOptions> evaluationResult;
    };
}

#endif