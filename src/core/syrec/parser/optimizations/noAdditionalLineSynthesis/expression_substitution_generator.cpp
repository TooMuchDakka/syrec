#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_substitution_generator.hpp"

using namespace noAdditionalLineSynthesis;

std::optional<syrec::VariableAccess::ptr> ExpressionSubstitutionGenerator::generateNextReplacementCandidate(syrec::AssignStatement::vec& requiredInitializationStatementsForCandidate) {
    return std::nullopt;
}