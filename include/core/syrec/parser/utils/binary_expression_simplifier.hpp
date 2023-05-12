#ifndef BINARY_EXPRESSION_SIMPLIFIER_HPP
#define BINARY_EXPRESSION_SIMPLIFIER_HPP

#include "core/syrec/expression.hpp"

namespace optimizations {
    struct BinaryExpressionSimplificationResult {
        const bool couldSimplify;
        const syrec::expression::ptr simplifiedExpression;

        explicit BinaryExpressionSimplificationResult(bool couldSimplify, syrec::expression::ptr simplifiedExpression):
            couldSimplify(couldSimplify), simplifiedExpression(std::move(simplifiedExpression)) {}
    };

    [[nodiscard]] BinaryExpressionSimplificationResult trySimplify(const syrec::expression::ptr& binaryExpr);
};
#endif