#ifndef SYREC_STATEMENT_VISITOR_HPP
#define SYREC_STATEMENT_VISITOR_HPP
#pragma once

#include "SyReCBaseVisitor.h"    
#include "core/syrec/parser/expression_evaluation_result.hpp"
#include "core/syrec/parser/module_call_stack.hpp"
#include "core/syrec/parser/antlr/parserComponents/syrec_custom_base_visitor.hpp"
#include "core/syrec/parser/antlr/parserComponents/syrec_expression_visitor.hpp"
#include "core/syrec/parser/optimizations/loop_optimizer.hpp"

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

         struct ModifiedLoopHeaderInformation {
            const std::size_t                activateAtLineRelativeToParent;
            const std::optional<std::string> loopVariableIdent;
            const std::size_t                newLoopStartValue;
            const std::size_t                loopEndValue;
            const std::size_t                newLoopStepsize;
        };
         std::optional<ModifiedLoopHeaderInformation> preDeterminedLoopHeaderInformation;
        
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

        bool                                                         areSemanticChecksForCallOrUncallDependingOnNameValid(bool isCallOperation, const messageUtils::Message::Position& moduleIdentTokenPosition, const std::optional<std::string>& moduleIdent);
        bool                                                         doArgumentsBetweenCallAndUncallMatch(const messageUtils::Message::Position& positionOfPotentialError, const std::string& uncalledModuleIdent, const std::vector<std::string>& calleeArguments);
        [[nodiscard]] std::optional<ExpressionEvaluationResult::ptr> getUnoptimizedExpression(SyReCParser::ExpressionContext* context);
        [[nodiscard]] static bool                                    areExpressionsEqual(const ExpressionEvaluationResult::ptr& firstExpr, const ExpressionEvaluationResult::ptr& otherExpr);
        
        void                                                         updateReferenceCountForUsedVariablesInExpression(const syrec::expression::ptr& expression, SyReCCustomBaseVisitor::ReferenceCountUpdate typeOfUpdate) const;
        [[nodiscard]] SymbolTableBackupHelper::ptr                   createCopiesOfCurrentValuesOfChangedSignalsAndResetToOriginal(const SymbolTableBackupHelper::ptr& backupOfOriginalValues) const;
        void                                                         mergeChangesFromBranchesAndUpdateSymbolTable(const SymbolTableBackupHelper::ptr& valuesOfChangedSignalsInTrueBranch, const SymbolTableBackupHelper::ptr& valuesOfChangedSignalsInFalseBranch, bool canAnyBranchBeOmitted, bool canTrueBranchBeOmitted) const;
        void                                                         createAndStoreBackupForAssignedToSignal(const syrec::VariableAccess::ptr& assignedToSignalParts) const;
        // TODO: Fix nicer solution than to return an expression::ptr
        [[nodiscard]] syrec::expression::ptr                         tryDetermineNewSignalValueFromAssignment(const syrec::VariableAccess::ptr& assignedToSignal, syrec_operation::operation assignmentOp, const syrec::expression::ptr& assignmentStmtRhsExpr) const;

        [[nodiscard]] std::optional<syrec::ForStatement::ptr> visitForStatementInReadonlyMode(SyReCParser::ForStatementContext* context);
        void                                                  visitForStatementWithOptimizationsEnabled(SyReCParser::ForStatementContext* context);
        void                                                  unrollAndProcessLoopBody(SyReCParser::ForStatementContext* context, const optimizations::LoopUnroller::UnrollInformation& unrolledLoopInformation);

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

        // TODO: Maybe find a better solution than to pass an expression::ptr for the new value
        void tryUpdateOrInvalidateStoredValueFor(const syrec::VariableAccess::ptr& assignedToVariableParts, const syrec::expression::ptr& exprContainingNewValue) const;
        void invalidateValuesForVariablesUsedAsParametersChangeableByModuleCall(const syrec::Module::ptr& calledModule, const std::vector<std::string>& calleeArguments, const std::set<std::size_t>& positionsOfParametersLeftUntouched) const;
        void invalidateStoredValueFor(const syrec::VariableAccess::ptr& assignedToVariableParts) const;
        void trimAndDecrementReferenceCountOfUnusedCalleeParameters(std::vector<std::string>& calleeArguments, const std::set<std::size_t>& positionsOfUnusedParameters) const;
        [[nodiscard]] bool areAccessedValuesForDimensionAndBitsConstant(const syrec::VariableAccess::ptr& accessedSignalParts) const;

        struct ParsedLoopHeaderInformation {
            const std::optional<std::string>                        loopVariableIdent;
            const std::pair<syrec::Number::ptr, syrec::Number::ptr> range;
            const syrec::Number::ptr                                stepSize;
        };
        [[nodiscard]] std::optional<ParsedLoopHeaderInformation> determineLoopHeader(SyReCParser::ForStatementContext* context, bool& openedNewSymbolTableScopeForLoopVariable);
        [[nodiscard]] std::optional<ParsedLoopHeaderInformation> determineLoopHeaderFromPredefinedOne(bool& openedNewSymbolTableScopeForLoopVariable);

        struct LoopIterationRange {
            const std::size_t startValue;
            const std::size_t endValue;
            const std::size_t stepSize;
        };
        [[nodiscard]] std::optional<LoopIterationRange> tryDetermineCompileTimeLoopIterationRange(const ParsedLoopHeaderInformation& loopHeader);
        [[nodiscard]] bool                              doesLoopOnlyPerformOneIteration(const ParsedLoopHeaderInformation& loopHeader);
        [[nodiscard]] static std::size_t                determineNumberOfLoopIterations(const LoopIterationRange& loopIterationRange);

        struct CustomLoopContextInformation {
            SyReCParser::ForStatementContext*         parentLoopContext;
            std::vector<CustomLoopContextInformation> nestedLoopContexts;
        };
        [[nodiscard]] static CustomLoopContextInformation buildCustomLoopContextInformation(SyReCParser::ForStatementContext* loopStatement);

        struct UnrolledLoopVariableValue {
            const std::size_t activateAtLineRelativeToParent;
            const std::string loopVariableIdent;
            const std::size_t value;
        };

         void buildParseRuleInformationFromUnrolledLoops(
                std::vector<antlr4::ParserRuleContext*>&               buildParseRuleInformation,
                std::size_t&                                          stmtOffsetFromParentLoop,
                SyReCParser::ForStatementContext*                     loopToUnroll,
                const CustomLoopContextInformation&                   customLoopContextInformation,
                std::vector<UnrolledLoopVariableValue>&               unrolledLoopVariableValues,
                std::vector<ModifiedLoopHeaderInformation>& modifiedLoopHeaderInformation,
                const optimizations::LoopUnroller::UnrollInformation& unrollInformation);

        [[nodiscard]] static std::optional<std::string>    tryGetLoopVariableIdent(SyReCParser::ForStatementContext* loopContext);
        [[nodiscard]] std::optional<syrec::Statement::vec> determineLoopBodyWithSideEffectsDisabled(SyReCParser::StatementListContext* loopBodyStmtsContext);
        [[nodiscard]] bool                                 isValuePropagationBlockedDueToLoopDataFlowAnalysis(const syrec::VariableAccess::ptr& accessedPartsOfSignalToBeUpdated) const;
        [[nodiscard]] syrec::AssignStatement::vec          trySimplifyAssignmentStatement(const syrec::AssignStatement::ptr& assignmentStmt) const;
        void                                               addOrUpdateLoopVariableEntryAndOptionallyMakeItsValueAvailableForEvaluations(const std::string& loopVariableIdent, const std::optional<unsigned int>& valueOfLoopVariable, bool& wasNewSymbolTableScopeOpened) const;
        void                                               removeLoopVariableAndMakeItsValueUnavailableForEvaluations(const std::string& loopVariableIdent, bool wasNewSymbolTableScopeOpenedForLoopVariable) const;

        [[nodiscard]] static bool doesModuleOnlyHaveReadOnlyParameters(const syrec::Module::ptr& calledModule);
        [[nodiscard]] static bool doesModuleOnlyConsistOfSkipStatements(const syrec::Module::ptr& calledModule);
        
        void                                                       decrementReferenceCountsOfCalledModuleAndActuallyUsedCalleeArguments(const syrec::Module::ptr& calledModule, const std::vector<std::string>& filteredCalleeArguments) const;
        void                                                       performAssignmentRhsExprSimplification(syrec::expression::ptr& assignmentRhsExpr) const;
        void                                                       performAssignmentOperation(const syrec::AssignStatement::ptr& assignmentStmt) const;


        struct IfConditionBranchEvaluationResult {
            std::optional<syrec::Statement::vec> parsedStatements;
            std::optional<SymbolTableBackupHelper::ptr> optionalBackupOfValuesAtEndOfBranch;

            explicit IfConditionBranchEvaluationResult(std::optional<syrec::Statement::vec> parsedStatements, std::optional<SymbolTableBackupHelper::ptr> optionalBackupOfValuesAtEndOfBranch):
                parsedStatements(std::move(parsedStatements)), optionalBackupOfValuesAtEndOfBranch(std::move(optionalBackupOfValuesAtEndOfBranch)) {}

        };

        [[nodiscard]] IfConditionBranchEvaluationResult parseIfConditionBranch(parser::SyReCParser::StatementListContext* ifBranchStmtsContext, bool shouldBeParsedAsReadonly, bool canBranchBeOmittedBasedOnGuardConditionEvaluation);
        void                                            invalidateAssignmentsOfNonNestedLoops(const std::vector<syrec::VariableAccess::ptr>& assignmentsOfNonNestedLoop) const;


        struct IterationInvariantLoopBodyResult {
            std::vector<syrec::VariableAccess::ptr> assignmentsInNonNestedLoops;
            syrec::Statement::vec                   statements;

            explicit IterationInvariantLoopBodyResult(std::vector<syrec::VariableAccess::ptr> assignmentsInNonNestedLoops, syrec::Statement::vec loopBodyStatements)
                : assignmentsInNonNestedLoops(std::move(assignmentsInNonNestedLoops)), statements(std::move(loopBodyStatements)) {}
        };

        /**
         * \brief Perform loop iteration invariant optimizations of the loop body. Value and reference count updates are only activated when one loop iteration is performed
         * \param loopBodyStmtContexts The statements of the loop body
         * \param doesLoopOnlyPerformOneIterationInTotal Will only one loop iteration be performed
         * \return A struct that contains the optimized loop body as well as the assignments of the loop body that were not defined in nested loops
         */
        [[nodiscard]] IterationInvariantLoopBodyResult determineIterationInvariantLoopBody(parser::SyReCParser::StatementListContext* loopBodyStmtContexts, bool doesLoopOnlyPerformOneIterationInTotal);
        [[nodiscard]] std::optional<optimizations::LoopOptimizationConfig> getUserDefinedLoopUnrollConfigOrDefault(bool doesCurrentLoopPerformOneLoopInTotal) const;
        void                                                               incrementReferenceCountsOfSignalsUsedInStatements(const syrec::Statement::vec& statements) const;
        void                                                               incrementReferenceCountsOfSignalsUsedInStatement(const syrec::Statement::ptr& statement) const;
        void                                                               incrementReferenceCountsOfSignalsUsedInNumber(const syrec::Number::ptr& number) const;

        void incrementReferenceCountsOfVariablesUsedInStatementsAndInvalidateAssignedToSignals(const syrec::Statement::vec& loopBodyStatements);
    };
} // namespace parser
#endif