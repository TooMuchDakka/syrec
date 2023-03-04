#ifndef SYREC_STATEMENT_VISITOR_HPP
#define SYREC_STATEMENT_VISITOR_HPP
#pragma once

#include "SyReCBaseVisitor.h"    
#include "core/syrec/parser/expression_evaluation_result.hpp"
#include "core/syrec/parser/module_call_stack.hpp"
#include "core/syrec/parser/antlr/parserComponents/syrec_custom_base_visitor.hpp"
#include "core/syrec/parser/antlr/parserComponents/syrec_expression_visitor.hpp"

namespace parser {
    class SyReCStatementVisitor: public SyReCCustomBaseVisitor {
    public:
        explicit SyReCStatementVisitor(const std::shared_ptr<SharedVisitorData>& sharedVisitorData):
            SyReCCustomBaseVisitor(sharedVisitorData),
            expressionVisitor(std::make_unique<SyReCExpressionVisitor>(sharedVisitorData)),
            moduleCallStack(std::make_unique<ModuleCallStack>(ModuleCallStack())) {}

        /*
         * Statement production visitors
         */
        std::any visitStatementList(SyReCParser::StatementListContext* context) override;
    protected:
        std::unique_ptr<SyReCExpressionVisitor> expressionVisitor;
        std::stack<syrec::Statement::vec>       statementListContainerStack;
        std::unique_ptr<ModuleCallStack>        moduleCallStack;
        
        std::any visitCallStatement(SyReCParser::CallStatementContext* context) override;
        std::any visitForStatement(SyReCParser::ForStatementContext* context) override;
        std::any visitLoopStepsizeDefinition(SyReCParser::LoopStepsizeDefinitionContext* context) override;
        std::any visitLoopVariableDefinition(SyReCParser::LoopVariableDefinitionContext* context) override;

        std::any visitIfStatement(SyReCParser::IfStatementContext* context) override;
        std::any visitUnaryStatement(SyReCParser::UnaryStatementContext* context) override;
        std::any visitAssignStatement(SyReCParser::AssignStatementContext* context) override;
        std::any visitSwapStatement(SyReCParser::SwapStatementContext* context) override;
        std::any visitSkipStatement(SyReCParser::SkipStatementContext* context) override;

        void addStatementToOpenContainer(const syrec::Statement::ptr& statement);
        void restrictAccessToAssignedToPartOfSignal(const syrec::VariableAccess::ptr& assignedToSignalPart, const TokenPosition& optionalEvaluationErrorPosition);
        void liftRestrictionToAssignedToPartOfSignal(const syrec::VariableAccess::ptr& assignedToSignalPart, const TokenPosition& optionalEvaluationErrorPosition);

        bool areSemanticChecksForCallOrUncallDependingOnNameValid(bool isCallOperation, const TokenPosition& moduleIdentTokenPosition, const std::optional<std::string>& moduleIdent);
        bool doArgumentsBetweenCallAndUncallMatch(const TokenPosition& positionOfPotentialError, const std::string& uncalledModuleIdent, const std::vector<std::string>& calleeArguments);
        
        [[nodiscard]] static bool areExpressionsEqual(const ExpressionEvaluationResult::ptr& firstExpr, const ExpressionEvaluationResult::ptr& otherExpr);
    private:
        std::any visitExpressionFromNumber(SyReCParser::ExpressionFromNumberContext* context) override {
            return expressionVisitor->visitExpressionFromNumber(context);
        }

        std::any visitExpressionFromSignal(SyReCParser::ExpressionFromSignalContext* context) override {
            return expressionVisitor->visitExpressionFromSignal(context);
        }

        std::any visitBinaryExpression(SyReCParser::BinaryExpressionContext* context) override {
            return expressionVisitor->visitBinaryExpression(context);    
        }

        std::any visitUnaryExpression(SyReCParser::UnaryExpressionContext* context) override {
            return expressionVisitor->visitUnaryExpression(context);
        }

        std::any visitShiftExpression(SyReCParser::ShiftExpressionContext* context) override {
            return expressionVisitor->visitShiftExpression(context);
        }
    };
} // namespace parser
#endif