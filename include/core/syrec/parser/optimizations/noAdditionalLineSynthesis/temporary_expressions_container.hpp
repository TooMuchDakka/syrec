#ifndef TEMPORARY_EXPRESSION_CONTAINER_HPP
#define TEMPORARY_EXPRESSION_CONTAINER_HPP
#pragma once

#include "core/syrec/expression.hpp"
#include "core/syrec/parser/symbol_table.hpp"

#include <memory>
#include <unordered_set>

namespace noAdditionalLineSynthesis {
    class TemporaryExpressionsContainer {
    public:
        using ptr = std::unique_ptr<TemporaryExpressionsContainer>;
        TemporaryExpressionsContainer(const parser::SymbolTable::ptr& symbolTableReference): activeExpressions({}), symbolTableReference(symbolTableReference) {}

        void               resetInternals();
        void               activateExpression(const syrec::expression::ptr& expr);
        void               deactivateExpression(const syrec::expression::ptr& expr);
        [[nodiscard]] bool existsAnyExpressionDefiningOverlappingSignalAccess(const syrec::VariableAccess& accessedSignalPartsToCheckForOverlaps) const;

    protected:
        std::unordered_set<syrec::expression::ptr> activeExpressions;
        parser::SymbolTable::ptr                   symbolTableReference;
        [[nodiscard]] static bool                  doesExpressionDefineOverlappingSignalAccess(const syrec::expression& expr, const syrec::VariableAccess& accessedSignalPartsToCheckForOverlaps, const parser::SymbolTable& symbolTableReference);
    };
}

#endif