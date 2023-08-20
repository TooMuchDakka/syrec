#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/assignment_with_reversible_ops_and_multilevel_signal_occurrence_simplifier.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/post_order_expr_traversal_helper.hpp"

using namespace noAdditionalLineSynthesis;

bool AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence::simplificationPrecondition(const syrec::AssignStatement::ptr& assignmentStmt) {
    // Check whether given pointer can be casted to assignment statement is already done in base class
    const auto& assignStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    return doesExprOnlyContainReversibleOperations(assignStmtCasted->rhs) && std::dynamic_pointer_cast<syrec::BinaryExpression>(assignStmtCasted->rhs) != nullptr;
    
    /*
        We can relax the precondition defined in the original paper (TODO: source) that was defined as the '^' and '-' operation only operating on leaf nodes
        to the '^' operation being defined either on only leaf nodes or on operation nodes with one leaf where the leaf node is the rhs operand.

        Expressions of the form (<subExpr> ^ (<subExpr_1> op <subExpr_2>)) cannot be simplified with this simplifier since the rhs expr cannot be split further 
        due to the possibility of a signal access occuring more than once in the overall expression.
    */
    //const bool isXorCheckOK = !isXorOperationOnlyDefinedForLeaveNodesInAST(assignStmtCasted->rhs) ? isXorOperationOnlyDefinedOnOperationNodesWithRhsBeingLeaf(assignStmtCasted->rhs) : true;
    //return isXorCheckOK && mappedToAssignmentOperationEnumValue == syrec_operation::operation::AddAssign || mappedToAssignmentOperationEnumValue == syrec_operation::operation::MinusAssign;
}

syrec::Statement::vec AssignmentWithReversibleOpsAndMultiLevelSignalOccurrence::simplifyWithoutPreconditionCheck(const syrec::AssignStatement::ptr& assignmentStmt, bool isValueOfAssignedToSignalBlockedByDataFlowAnalysis) {
    const auto& assignmentStmtCasted                                    = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    const auto& valueOfInitiallyAssignedToSignalPriorToAssignmentIsZero = !isValueOfAssignedToSignalBlockedByDataFlowAnalysis && !symbolTable->tryFetchValueForLiteral(assignmentStmtCasted->lhs).value_or(true);
    const auto& subExprTraversalHelper                                  = std::make_unique<PostOrderExprTraversalHelper>();
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