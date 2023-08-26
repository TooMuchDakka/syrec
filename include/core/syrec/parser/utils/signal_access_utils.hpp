#ifndef SIGNAL_ACCESS_UTILS_HPP
#define SIGNAL_ACCESS_UTILS_HPP
#pragma once

#include "core/syrec/number.hpp"
#include "core/syrec/variable.hpp"
#include "core/syrec/parser/symbol_table.hpp"

namespace SignalAccessUtils {
    struct SignalAccessComponentEquivalenceCriteria {
        enum class BitRange {
            Overlapping,
            Enclosed,
            Equal
        };

        enum class DimensionAccess {
            Overlapping,
            Enclosing,
            Equal
        };
    };

    struct SignalAccessEquivalenceResult {
        enum Equality {
            Equal,
            NotEqual,
            Maybe
        };

        Equality equality;
        bool     isResultCertain;

        explicit SignalAccessEquivalenceResult(Equality equality, bool isResultCertain):
            equality(equality), isResultCertain(isResultCertain) {}
    };

    [[nodiscard]] std::optional<unsigned int>   tryEvaluateNumber(const syrec::Number::ptr& number, const parser::SymbolTable::ptr& symbolTable);
    [[nodiscard]] std::optional<unsigned int>   tryEvaluateCompileTimeConstantExpression(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpression, const parser::SymbolTable::ptr& symbolTable);
    [[nodiscard]] SignalAccessEquivalenceResult areAccessedBitRangesEqual(const syrec::VariableAccess::ptr& accessedSignalParts, const syrec::VariableAccess::ptr& referenceSignalAccess, SignalAccessComponentEquivalenceCriteria::BitRange expectedBitRangeEquivalenceCriteria, const parser::SymbolTable::ptr& symbolTable);
    [[nodiscard]] SignalAccessEquivalenceResult areDimensionAccessesEqual(const syrec::VariableAccess::ptr& accessedSignalParts, const syrec::VariableAccess::ptr& referenceSignalAccess, SignalAccessComponentEquivalenceCriteria::DimensionAccess dimensionAccessEquivalenceCriteria, const parser::SymbolTable::ptr& symbolTable);
    [[nodiscard]] SignalAccessEquivalenceResult areSignalAccessesEqual(const syrec::VariableAccess::ptr& accessedSignalParts, const syrec::VariableAccess::ptr& referenceSignalAccess, SignalAccessComponentEquivalenceCriteria::DimensionAccess dimensionAccessEquivalenceCriteria, SignalAccessComponentEquivalenceCriteria::BitRange expectedBitRangeEquivalenceCriteria, const parser::SymbolTable::ptr& symbolTable);
}
#endif