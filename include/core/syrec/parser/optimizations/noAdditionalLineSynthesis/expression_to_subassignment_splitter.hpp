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