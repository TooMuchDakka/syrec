#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_only_reversible_operations_simplifier.hpp"
#include <core/syrec/parser/operation.hpp>
#include <core/syrec/parser/optimizations/noAdditionalLineSynthesis/post_order_expr_traversal_helper.hpp>

using namespace noAdditionalLineSynthesis;

bool AssignmentWithOnlyReversibleOperationsSimplifier::simplificationPrecondition(const syrec::AssignStatement::ptr& assignmentStmt) {
    // Check whether given pointer can be casted to assignment statement is already done in base class
    const auto& assignStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    return doesExprOnlyContainReversibleOperations(assignStmtCasted->rhs) && std::dynamic_pointer_cast<syrec::BinaryExpression>(assignStmtCasted->rhs) != nullptr;
}

// TODO: Simplification could work as in the assignment with only reversible operations and multilevel signal occurrences with the difference that "^" operation nodes with nested expressions can be handled
// if the precondition of unique signal accesses applies to this simplifier (or maybe just in the nested "^" operation but its simplification is still open)
// TODO: Simplification of += operation to ^= operation for first assignment
syrec::Statement::vec AssignmentWithOnlyReversibleOperationsSimplifier::simplifyWithoutPreconditionCheck(const syrec::AssignStatement::ptr& assignmentStmt, bool isValueOfAssignedToSignalBlockedByDataFlowAnalysis) {
    const auto& assignmentStmtCasted   = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    const auto& valueOfInitiallyAssignedToSignalPriorToAssignmentIsZero = !isValueOfAssignedToSignalBlockedByDataFlowAnalysis && !symbolTable->tryFetchValueForLiteral(assignmentStmtCasted->lhs).value_or(true);
    const auto& subExprTraversalHelper = std::make_unique<PostOrderExprTraversalHelper>();
    subExprTraversalHelper->buildPostOrderQueue(*syrec_operation::tryMapAssignmentOperationFlagToEnum(assignmentStmtCasted->op), assignmentStmtCasted->rhs, valueOfInitiallyAssignedToSignalPriorToAssignmentIsZero);

    const auto& assignedToSignal        = assignmentStmtCasted->lhs;
    const auto& generatedSubAssignments = subExprTraversalHelper->getAll();
    if (generatedSubAssignments.empty()) {
        return {};
    }

    syrec::Statement::vec generatedAssignments(generatedSubAssignments.size(), nullptr);
    for (std::size_t i = 0; i < generatedAssignments.size(); ++i) {
        const auto generatedSubAssignment = generatedSubAssignments.at(i);
        generatedAssignments.at(i)        = std::make_shared<syrec::AssignStatement>(
                assignedToSignal,
                *syrec_operation::tryMapAssignmentOperationEnumToFlag(generatedSubAssignment.assignmentOperation),
                generatedSubAssignment.assignmentRhsOperand);
    }
    if (const auto& firstGeneratedSubAssignment = std::dynamic_pointer_cast<syrec::AssignStatement>(generatedAssignments.front()); valueOfInitiallyAssignedToSignalPriorToAssignmentIsZero && firstGeneratedSubAssignment != nullptr && *syrec_operation::tryMapAssignmentOperationFlagToEnum(firstGeneratedSubAssignment->op) == syrec_operation::operation::AddAssign) {
        firstGeneratedSubAssignment->op = *syrec_operation::tryMapAssignmentOperationEnumToFlag(syrec_operation::operation::XorAssign);
    }
    return generatedAssignments;
}