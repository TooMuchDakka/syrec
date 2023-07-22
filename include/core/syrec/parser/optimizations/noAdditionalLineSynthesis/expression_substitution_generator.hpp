#ifndef EXPRESSION_SUBSTITUTION_GENERATOR_HPP
#define EXPRESSION_SUBSTITUTION_GENERATOR_HPP
#pragma once

#include "core/syrec/variable.hpp"
#include "core/syrec/parser/symbol_table.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/simple_additional_line_for_assignment_reducer.hpp"

#include <optional>

namespace noAdditionalLineSynthesis {
    class ExpressionSubstitutionGenerator {
    public:
        explicit ExpressionSubstitutionGenerator(parser::SymbolTable::ptr symbolTable, const syrec::expression::ptr& expressionToBeSubstituted, SimpleAdditionalLineForAssignmentReducer::LookupOfExcludedSignalsForReplacement&& candidatesToExclude):
            symbolTable(std::move(symbolTable)), expressionToBeSubstituted(expressionToBeSubstituted), candidatesToExclude(std::move(candidatesToExclude)) {}

        [[nodiscard]] std::optional<syrec::VariableAccess::ptr> generateNextReplacementCandidate(syrec::AssignStatement::vec& requiredInitializationStatementsForCandidate);

    private:
        const parser::SymbolTable::ptr                                                  symbolTable;
        const syrec::expression::ptr                                                    expressionToBeSubstituted;
        SimpleAdditionalLineForAssignmentReducer::LookupOfExcludedSignalsForReplacement candidatesToExclude;

        [[nodiscard]] std::optional<bool>         doesReplacementCandidateRequireResetBeforeUsage(const syrec::VariableAccess::ptr& replacementCandidate);
        [[nodiscard]] syrec::AssignStatement::vec createResetStatementsForReplacementCandidate(const syrec::VariableAccess::ptr& replacementCandidate);
        [[nodiscard]] std::optional<syrec::VariableAccess::ptr> generateNextReplacementCandidate();
    };
}
#endif