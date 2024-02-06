#ifndef EXPRESSION_TO_SUBASSIGNMENT_SPLITTER_HPP
#define EXPRESSION_TO_SUBASSIGNMENT_SPLITTER_HPP

#include "core/syrec/statement.hpp"
#include "core/syrec/parser/operation.hpp"

namespace noAdditionalLineSynthesis {
    // TODO: Add support for numeric expression that could again define compile time expressions
    class ExpressionToSubAssignmentSplitter {
    public:
        using ptr = std::unique_ptr<ExpressionToSubAssignmentSplitter>;
        virtual ~ExpressionToSubAssignmentSplitter() = default;
        ExpressionToSubAssignmentSplitter():
            fixedSubAssignmentOperationsQueueIdx(0), operationInversionFlag(false) {}

        /**
         * \brief Try to simplify assignment by splitting the rhs operand of said statement into sub-assignments if possible.
         *
         * NOTE: It is the responsibility of the caller to guarantee that no signal accessed overlapping \p assignedToSignal are defined in \p expr.
         * \param assignment The assignment which shall be simplified
         * \param currentValueOfAssignedToSignal The current value of the assigned to signal parts prior to the assignment
         * \return A vector containing either the generated sub assignments or an assignment consisting of the user provided parameters of the form \p assignedToSignal \p initialAssignmentOperation \p expr.
         */
        [[nodiscard]] syrec::AssignStatement::vec createSubAssignmentsBySplitOfExpr(const syrec::AssignStatement::ptr& assignment, const std::optional<unsigned int>& currentValueOfAssignedToSignal);


    protected:
        syrec::AssignStatement::vec                      generatedAssignments;
        syrec::VariableAccess::ptr                       fixedAssignmentLhs;
        std::vector<syrec_operation::operation>          fixedSubAssignmentOperationsQueue;
        std::size_t                                      fixedSubAssignmentOperationsQueueIdx;
        bool                                             operationInversionFlag;
        std::optional<unsigned int>                      currentAssignedToSignalValue;
        
        virtual void resetInternals();
        virtual void init(const syrec::VariableAccess::ptr& fixedAssignedToSignalParts, syrec_operation::operation initialAssignmentOperation, const std::optional<unsigned int>& initialValueOfAssignedToSignalOfOriginalAssignment);

        
        /**
         * \brief Check whether the simplified split of assignment right-hand side expression is allowed.
         *
         * \remarks This is allowed when the following conditions hold (when applicable): <br>
         * I.        The assignment operation is NOT equal to '^=' and the rhs expr does not contain any non-reversible operations when one or both operands are nested expressions or bitwise xor operation. <br>
         * II.       The assignment operation is equal to '^=' while the value of the assigned to signal is equal to zero and the following holds for the rhs expr: <br>
         * II.I.     The rhs expr does not contain any non-reversible operations when either one or both operands are nested expressions <br>
         * II.II.    For each nested expression starting from the topmost expression of the rhs expr check: <br>
         * II.II.I.  Does the rhs operand not define a nested expression, if not repeat the check starting from II.II. for the lhs operand if the said operand defined a nested expression <br>
         *
         * TODO: Remove II.I. Starting from the first operation node of the expression tree when traversing the rhs expr in post-order traversal only the first operation node can define the bitwise
         *       xor operation on an operation node with two leaf nodes, all other operation nodes defining the bitwise xor operation and reachable by the first operation can only contain
         *       a leaf node as the "second" operand of the operation node while the "first" operand must be the expr reachable from the first operation node (when traversing the expression tree
         *       bottom-up) <br>
         * \param operation The defined assignment operation
         * \param topmostAssignmentRhsExpr The right-hand side expression of the assignment
         * \param currentValueOfAssignedToSignal The current value of the assigned to signal parts prior to the assignment
         * \return Whether the simplified split "algorithm" can be applied.
         */
        [[nodiscard]] bool isPreconditionSatisfied(syrec_operation::operation operation, const syrec::expression& topmostAssignmentRhsExpr, const std::optional<unsigned int>& currentValueOfAssignedToSignal) const;

        void                                                           updateOperationInversionFlag(syrec_operation::operation operationDeterminingInversionFlag);
        [[nodiscard]] bool                                             handleExpr(const syrec::expression::ptr& expr);
        [[nodiscard]] bool                                             handleExprWithNoLeafNodes(const syrec::expression::ptr& expr);
        [[nodiscard]] bool                                             handleExprWithOneLeafNode(const syrec::expression::ptr& expr);
        [[nodiscard]] bool                                             handleExprWithTwoLeafNodes(const syrec::expression::ptr& expr);
        void                                                           storeAssignment(const syrec::AssignStatement::ptr& assignment);
        [[nodiscard]] std::optional<syrec_operation::operation>        getOperationOfNextSubAssignment(bool markAsConsumed = true);
        [[nodiscard]] bool                                             createBackupOfInversionFlagStatus() const;
        void                                                           restorePreviousInversionStatusFlag(bool previousInversionStatus);
        void                                                           fixNextSubAssignmentOperation(syrec_operation::operation operation);
        void                                                           updateValueOfAssignedToSignalViaAssignment(const std::optional<syrec_operation::operation>& assignmentOperation, const syrec::expression* assignmentRhsExpr);
        
        /**
         * \brief Determine whether a given expression can be further split into sub-assignments assuming that the expression is the operand of an operation node defining a bitwise xor operation
         * \remarks A split is possible if:
         *  I. If \p expr defines a signal access or a non-nested numeric expression (i.e. the expression is a leaf node of the operation node for which the check if performed)
         *  II. The current value of the assigned to signal is zero while the given expression does not match condition I.
         * \param expr The expression which shall be checked whether it can be further split
         * \return Whether a split can take place
         */
        [[nodiscard]] bool                                             isSplitOfXorOperationIntoSubAssignmentsAllowedForExpr(const syrec::expression& expr) const;

        [[nodiscard]] std::optional<syrec_operation::operation>             determineAssignmentOperationToUse(syrec_operation::operation operation) const;
        [[nodiscard]] static syrec::AssignStatement::ptr                    createAssignmentFrom(const syrec::VariableAccess::ptr& assignedToSignalParts, syrec_operation::operation operation, const syrec::expression::ptr& assignmentRhsExpr);
        [[nodiscard]] static bool                                           doesExprDefineSubExpression(const syrec::expression& expr);
        [[nodiscard]] static bool                                           doesOperandDefineLeafNode(const syrec::expression& expr);
        [[nodiscard]] static std::optional<std::pair<bool, bool>>           determineLeafNodeStatusForOperandsOfExpr(const syrec::expression& expr);

        /**
         * \brief Try perform transformation of expression <assignedToSignal> -= (<subExpr_1> - <subExpr_2>) to <assignedToSignal += (<subExpr_2> - <subExpr_1>) to enable the optimization of replacing <br>
         * an += operation with a ^= operation if the symbol table entry for the assigned to signal value is zero. The latter will not be checked here and only the assignment will be transformed.
         * \param assignment The assignment which shall be checked
         */
        static void                  transformTwoSubsequentMinusOperations(syrec::AssignStatement& assignment);
        [[maybe_unused]] static bool transformXorOperationToAddAssignOperationIfTopmostOpOfRhsExprIsNotBitwiseXor(syrec::AssignStatement& assignment, const std::optional<unsigned int>& currentValueOfAssignedToSignal);
        static void                  transformAddAssignOperationToXorAssignOperationIfAssignedToSignalValueIsZeroAndTopmostOpOfRhsExprIsBitwiseXor(syrec::AssignStatement& assignment, const std::optional<unsigned int>& currentValueOfAssignedToSignal);
        /**
         * \brief Check whether the given \p expr does not defined any operations without a matching assignment operation counterpart when one or both operands are nested expressions <br>
         * \remarks This function assumes that numeric expressions that defined compile time constant expression where already converted to binary expressions
         * \param expr The expression to check
         * \return Whether all defined operations of the expression had a matching assignment operation counterpart defined
         */
        [[nodiscard]] static bool doesExprNotContainAnyOperationWithoutAssignmentCounterpartWhenBothOperandsAreNestedExpr(const syrec::expression& expr);
    };
}

#endif