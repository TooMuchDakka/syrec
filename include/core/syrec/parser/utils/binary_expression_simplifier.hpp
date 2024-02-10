#ifndef BINARY_EXPRESSION_SIMPLIFIER_HPP
#define BINARY_EXPRESSION_SIMPLIFIER_HPP

#include "core/syrec/expression.hpp"
#include "core/syrec/parser/symbol_table.hpp"

namespace optimizations {

    // TODO: Extend with new field to give the user the ability to handle removed (optimized) away signal accesses (i.e. for update of reference counts)
    struct BinaryExpressionSimplificationResult {
        const bool couldSimplify;
        const syrec::Expression::ptr simplifiedExpression;

        explicit BinaryExpressionSimplificationResult(bool couldSimplify, syrec::Expression::ptr simplifiedExpression):
            couldSimplify(couldSimplify), simplifiedExpression(std::move(simplifiedExpression)) {}
    };

    [[nodiscard]] BinaryExpressionSimplificationResult trySimplify(const syrec::Expression::ptr& binaryExpr, bool shouldPerformOperationStrengthReduction, const parser::SymbolTable& symbolTable, std::vector<syrec::VariableAccess::ptr>* droppedSignalAccesses);
};
#endif