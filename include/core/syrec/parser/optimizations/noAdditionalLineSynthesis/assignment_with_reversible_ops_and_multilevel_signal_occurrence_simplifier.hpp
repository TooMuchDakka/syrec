#ifndef ASSIGNMENT_WITH_REVERSIBLE_OPS_AND_MULTILEVEL_SIGNAL_OCCURRENCE_HPP
#define ASSIGNMENT_WITH_REVERSIBLE_OPS_AND_MULTILEVEL_SIGNAL_OCCURRENCE_HPP
#pragma once

#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/base_assignment_simplifier.hpp"

namespace noAdditionalLineSynthesis {
    class AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence: public BaseAssignmentSimplifier {
    public:
        AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence(parser::SymbolTable::ptr symbolTable)
            : BaseAssignmentSimplifier(std::move(symbolTable)) {}

        ~AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence() override = default;
    protected:
        [[nodiscard]] bool simplificationPrecondition(const syrec::AssignStatement::ptr& assignmentStmt) override;
    private:
        [[nodiscard]] syrec::Statement::vec simplifyWithoutPreconditionCheck(const syrec::AssignStatement::ptr& assignmentStmt, bool isValueOfAssignedToSignalBlockedByDataFlowAnalysis) override;
        /*[[nodiscard]] static bool           isXorOperationOnlyDefinedForLeaveNodesInAST(const syrec::expression::ptr& binaryExpr);
        [[nodiscard]] static bool           isXorOperationOnlyDefinedOnOperationNodesWithRhsBeingLeaf(const syrec::expression::ptr& binaryExpr);*/
    };
}
#endif
