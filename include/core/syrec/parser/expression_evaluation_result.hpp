#ifndef EXPRESSION_EVALUATION_RESULT_HPP
#define EXPRESSION_EVALUATION_RESULT_HPP
#pragma once

#include "core/syrec/expression.hpp"
#include "core/syrec/parser/utils/bit_helpers.hpp"

#include <variant>

// https://stackoverflow.com/questions/69652744/c-singleton-private-constructor-not-accessible-from-static-function?noredirect=1&lq=1

namespace parser {
    class ExpressionEvaluationResult {
    public:
        typedef std::shared_ptr <ExpressionEvaluationResult> ptr;

        [[nodiscard]] bool isConstantValue() const;
        [[nodiscard]] std::optional<unsigned int> getAsConstant() const;
        [[nodiscard]] std::optional<syrec::Expression::ptr> getAsExpression() const;
        [[nodiscard]] static ExpressionEvaluationResult::ptr createFromConstantValue(unsigned int constantValue, const std::optional<unsigned int>& optionalExpectedSignalWidth);
        [[nodiscard]] static ExpressionEvaluationResult::ptr createFromExpression(const syrec::Expression::ptr& expression, const std::vector<std::optional<unsigned int>>& explicitlyAccessedValuesPerDimension);

        struct OperandSizeInformation {
            const std::size_t numDeclaredDimensionOfOperand;
            const std::vector<std::optional<unsigned int>> explicitlyAccessedValuesPerDimension;
            const std::vector<unsigned int> numValuesPerNotExplicitlyAccessedDimension;
            
            [[nodiscard]] std::vector<unsigned int>                determineNumAccessedValuesPerDimension() const;
        };
        [[nodiscard]] OperandSizeInformation determineOperandSize() const {
            return OperandSizeInformation({numDeclaredDimensionsOfEvaluationResult, explicitlyAccessedValuesPerDimension, numValuesPerNotExplicitlyAccessedDimension});
        }

    private:
        ExpressionEvaluationResult(unsigned int constantValue, const std::optional<unsigned int>& optionalExpectedSignalWidth):
            isConstant(true), optionalExpectedSignalWidthForConstantValue(optionalExpectedSignalWidth), numDeclaredDimensionsOfEvaluationResult(1), explicitlyAccessedValuesPerDimension({}), numValuesPerNotExplicitlyAccessedDimension({1}) {
            evaluationResult = std::make_pair(constantValue, optionalExpectedSignalWidth.has_value() ? *optionalExpectedSignalWidth : BitHelpers::getRequiredBitsToStoreValue(constantValue));
        }

        ExpressionEvaluationResult(const syrec::Expression::ptr& expression, std::size_t numDeclaredDimensionOfExpr, std::vector<std::optional<unsigned int>> explicitlyAccessedValuesPerDimension, std::vector<unsigned int> numValuesPerNotExplicitlyAccessedDimension):
            isConstant(false),
            optionalExpectedSignalWidthForConstantValue(std::nullopt),
            numDeclaredDimensionsOfEvaluationResult(numDeclaredDimensionOfExpr),
            explicitlyAccessedValuesPerDimension(std::move(explicitlyAccessedValuesPerDimension)),
            numValuesPerNotExplicitlyAccessedDimension(std::move(numValuesPerNotExplicitlyAccessedDimension)) {
            evaluationResult = expression;
        }

        using ConstantValueAndBitwidthPair = std::pair<unsigned int, unsigned int>;
        using AvailableOptions = std::variant<syrec::Expression::ptr, ConstantValueAndBitwidthPair>;

        bool                                     isConstant;
        std::optional<unsigned int>              optionalExpectedSignalWidthForConstantValue;
        std::size_t                              numDeclaredDimensionsOfEvaluationResult;
        std::vector<std::optional<unsigned int>> explicitlyAccessedValuesPerDimension;
        std::vector<unsigned int>                numValuesPerNotExplicitlyAccessedDimension;
        std::optional<AvailableOptions>          evaluationResult;

        [[nodiscard]] static ExpressionEvaluationResult::ptr createFromExprDefiningSignalAccess(const syrec::VariableExpression::ptr& exprDefiningSignalAccess, const std::vector<std::optional<unsigned int>>& explicitlyAccessedValuesPerDimension);
    };
}

#endif