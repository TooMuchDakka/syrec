#ifndef ASSIGNMENT_WITH_ONLY_REVERSIBLE_OPERATIONS_SIMPLIFIER_HPP
#define ASSIGNMENT_WITH_ONLY_REVERSIBLE_OPERATIONS_SIMPLIFIER_HPP
#pragma once

#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/base_assignment_simplifier.hpp"

namespace noAdditionalLineSynthesis {
    class AssignmentWithOnlyReversibleOperationsSimplifier: public BaseAssignmentSimplifier {
    public:
        explicit AssignmentWithOnlyReversibleOperationsSimplifier(parser::SymbolTable::ptr symbolTable)
            : BaseAssignmentSimplifier(std::move(symbolTable)) {}

        ~AssignmentWithOnlyReversibleOperationsSimplifier() override = default;
    protected:
        [[nodiscard]] bool                  simplificationPrecondition(const syrec::AssignStatement::ptr& assignmentStmt) override;
        [[nodiscard]] syrec::Statement::vec simplifyWithoutPreconditionCheck(const syrec::AssignStatement::ptr& assignmentStmt, bool isValueOfAssignedToSignalBlockedByDataFlowAnalysis) override;
    };
}
#endif