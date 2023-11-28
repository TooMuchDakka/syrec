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
         * \param assignedToSignal The assigned to signal defining of the original assignment which servers as the default left hand side operand of any generated sub-assignment.
         * \param initialAssignmentOperation The initial assignment operation.
         * \param expr The expression which should be simplified by generating subexpressions if possible.
         * \return A vector containing either the generated sub assignments or an assignment consisting of the user provided parameters of the form \p assignedToSignal \p initialAssignmentOperation \p expr.
         */
        [[nodiscard]] syrec::AssignStatement::vec createSubAssignmentsBySplitOfExpr(const syrec::VariableAccess::ptr& assignedToSignal, syrec_operation::operation initialAssignmentOperation, const syrec::expression::ptr& expr);
    protected:
        syrec::AssignStatement::vec                      generatedAssignments;
        syrec::VariableAccess::ptr                       fixedAssignmentLhs;
        std::vector<syrec_operation::operation>          fixedSubAssignmentOperationsQueue;
        std::size_t                                      fixedSubAssignmentOperationsQueueIdx;
        bool                                             operationInversionFlag;

        virtual void                                                   resetInternals();
        virtual void                                                   init(const syrec::VariableAccess::ptr& fixedAssignedToSignalParts, syrec_operation::operation initialAssignmentOperation);
        void                                                           updateOperationInversionFlag(syrec_operation::operation operationDeterminingInversionFlag);
        [[nodiscard]] bool                                             handleExpr(const syrec::expression::ptr& expr);
        [[nodiscard]] bool                                             handleExprWithNoLeafNodes(const syrec::expression::ptr& expr);
        [[nodiscard]] bool                                             handleExprWithOneLeafNode(const syrec::expression::ptr& expr);
        [[nodiscard]] bool                                             handleExprWithTwoLeafNodes(const syrec::expression::ptr& expr);
        void                                                           storeAssignment(const syrec::AssignStatement::ptr& assignment);
        [[nodiscard]] std::optional<syrec_operation::operation>        getOperationOfNextSubAssignment();
        [[nodiscard]] bool                                             createBackupOfInversionFlagStatus() const;
        void                                                           restorePreviousInversionStatusFlag(bool previousInversionStatus);
        void                                                           fixNextSubAssignmentOperation(syrec_operation::operation operation);

        [[nodiscard]] std::optional<syrec_operation::operation>             determineAssignmentOperationToUse(syrec_operation::operation operation) const;
        [[nodiscard]] static syrec::AssignStatement::ptr                    createAssignmentFrom(const syrec::VariableAccess::ptr& assignedToSignalParts, syrec_operation::operation operation, const syrec::expression::ptr& assignmentRhsExpr);
        [[nodiscard]] static bool                                           doesExprDefineSubExpression(const syrec::expression& expr);
        [[nodiscard]] static bool                                           doesOperandDefineLeafNode(const syrec::expression& expr);
        [[nodiscard]] static std::optional<std::pair<bool, bool>>           determineLeafNodeStatusForOperandsOfExpr(const syrec::expression& expr);
        [[nodiscard]] static std::optional<syrec::BinaryExpression::ptr>    convertNumericExprToBinary(const syrec::NumericExpression& numericExpr);
        [[nodiscard]] static std::optional<syrec_operation::operation>      tryMapCompileTimeConstantExprOperationToBinaryOperation(syrec::Number::CompileTimeConstantExpression::Operation operation);
        [[nodiscard]] static syrec::expression::ptr                         convertNumberToExpr(const syrec::Number::ptr& number, unsigned int expectedBitwidth);
        [[nodiscard]] static std::optional<syrec::BinaryExpression::ptr>    convertCompileTimeConstantExprToBinaryExpr(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpr, unsigned int expectedBitwidth);
        [[nodiscard]] static syrec::expression::ptr                         transformExprBeforeProcessing(const syrec::expression::ptr& initialExpr);
    };
}

#endif