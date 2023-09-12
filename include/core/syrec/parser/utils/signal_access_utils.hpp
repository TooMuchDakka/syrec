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

    [[nodiscard]] std::optional<unsigned int>   tryEvaluateNumber(const syrec::Number& number, const parser::SymbolTable& symbolTable);
    [[nodiscard]] std::optional<unsigned int>   tryEvaluateCompileTimeConstantExpression(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpression, const parser::SymbolTable& symbolTable);
    [[nodiscard]] SignalAccessEquivalenceResult areAccessedBitRangesEqual(const syrec::VariableAccess& accessedSignalParts, const syrec::VariableAccess& referenceSignalAccess, SignalAccessComponentEquivalenceCriteria::BitRange expectedBitRangeEquivalenceCriteria, const parser::SymbolTable& symbolTable);
    [[nodiscard]] SignalAccessEquivalenceResult areDimensionAccessesEqual(const syrec::VariableAccess& accessedSignalParts, const syrec::VariableAccess& referenceSignalAccess, SignalAccessComponentEquivalenceCriteria::DimensionAccess dimensionAccessEquivalenceCriteria, const parser::SymbolTable& symbolTable);
    [[nodiscard]] SignalAccessEquivalenceResult areSignalAccessesEqual(const syrec::VariableAccess& accessedSignalParts, const syrec::VariableAccess& referenceSignalAccess, SignalAccessComponentEquivalenceCriteria::DimensionAccess dimensionAccessEquivalenceCriteria, SignalAccessComponentEquivalenceCriteria::BitRange expectedBitRangeEquivalenceCriteria, const parser::SymbolTable& symbolTable);
}
#endif