#include "core/syrec/parser/antlr/parserComponents/syrec_statement_visitor.hpp"

#include "core/syrec/parser/custom_semantic_errors.hpp"
#include "core/syrec/parser/expression_comparer.hpp"
#include "core/syrec/parser/expression_evaluation_result.hpp"
#include "core/syrec/parser/infix_iterator.hpp"
#include "core/syrec/parser/module_call_guess.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/parser_utilities.hpp"
#include "core/syrec/parser/signal_access_restriction.hpp"
#include "core/syrec/parser/signal_evaluation_result.hpp"
#include "core/syrec/parser/value_broadcaster.hpp"

#include <fmt/format.h>

/*
 * TODO: General todo, when checking if both operands have the same dimensions(and also the same values per accessed dimension)
 * we also need to take partial dimension accesses into account (i.e. a[2][4] += b with a = out a[4][6][2][1] and b in[2][1])
 */ 

using namespace parser;

std::any SyReCStatementVisitor::visitStatementList(SyReCParser::StatementListContext* context) {
    // TODO: If custom casting utility function supports polymorphism as described in the description of the function, this stack is no longer needed
    statementListContainerStack.push({});
    sharedData->currentModuleCallNestingLevel++;

    for (const auto& userDefinedStatement: context->stmts) {
        if (userDefinedStatement != nullptr) {
            visitStatement(userDefinedStatement);
        }
        /*
         * TODO: The current implementation of our utility functions does not support polymorphism when casting the value of std::any, thats why we need this workaround with the stack of statements
        const auto validatedUserDefinedStatement = tryVisitAndConvertProductionReturnValue<syrec::Statement::ptr>(userDefinedStatement);
        if (validatedUserDefinedStatement.has_value()) {
            validUserDefinedStatements.emplace_back(*validatedUserDefinedStatement);
        }
        */
    }

    const auto validUserDefinedStatements = statementListContainerStack.top();
    statementListContainerStack.pop();

    // Remove all uncalled modules from the call stack since there is no other possibility after this statement block to uncall them
    for (const auto& notUncalledModule: moduleCallStack->popAllForCurrentNestingLevel(sharedData->currentModuleCallNestingLevel)) {
        std::ostringstream bufferForStringifiedParametersOfNotUncalledModule;
        std::copy(notUncalledModule->calleeArguments.cbegin(), notUncalledModule->calleeArguments.cend(), infix_ostream_iterator<std::string>(bufferForStringifiedParametersOfNotUncalledModule, ","));

        const size_t moduleIdentLinePosition   = notUncalledModule->moduleIdentPosition.first;
        const size_t moduleIdentColumnPosition = notUncalledModule->moduleIdentPosition.second;
        createError(TokenPosition(moduleIdentLinePosition, moduleIdentColumnPosition), fmt::format(MissingUncall, notUncalledModule->moduleIdent, bufferForStringifiedParametersOfNotUncalledModule.str()));
    }
    sharedData->currentModuleCallNestingLevel--;

    // Wrapping into std::optional is only needed because helper function to cast return type of production required this wrapper
    return validUserDefinedStatements.empty() ? std::nullopt : std::make_optional(validUserDefinedStatements);
}

std::any SyReCStatementVisitor::visitAssignStatement(SyReCParser::AssignStatementContext* context) {
    const auto backupOfConstantPropagationFlag = sharedData->parserConfig->performConstantPropagation;

    sharedData->parserConfig->performConstantPropagation = false;
    const auto assignedToSignal             = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->signal());
    sharedData->parserConfig->performConstantPropagation = backupOfConstantPropagationFlag;

    bool       allSemanticChecksOk          = assignedToSignal.has_value() && (*assignedToSignal)->isVariableAccess() && isSignalAssignableOtherwiseCreateError(context->signal()->IDENT()->getSymbol(), *(*assignedToSignal)->getAsVariableAccess());
    bool       needToResetSignalRestriction = false;

    if (allSemanticChecksOk) {
        const auto assignedToSignalBitWidthEvaluated = tryDetermineBitwidthAfterVariableAccess(*(*assignedToSignal)->getAsVariableAccess(), mapAntlrTokenPosition(context->start));
        if (assignedToSignalBitWidthEvaluated.has_value()) {
            sharedData->optionalExpectedExpressionSignalWidth.emplace(*assignedToSignalBitWidthEvaluated);
        } else {
            sharedData->optionalExpectedExpressionSignalWidth.emplace((*(*assignedToSignal)->getAsVariableAccess())->getVar()->bitwidth);
        }

        restrictAccessToAssignedToPartOfSignal(*(*assignedToSignal)->getAsVariableAccess(), determineContextStartTokenPositionOrUseDefaultOne(context));
        needToResetSignalRestriction = true;
    }

    const auto definedAssignmentOperation = getDefinedOperation(context->assignmentOp);
    if (!definedAssignmentOperation.has_value() || (syrec_operation::operation::xor_assign != *definedAssignmentOperation && syrec_operation::operation::add_assign != *definedAssignmentOperation && syrec_operation::operation::minus_assign != *definedAssignmentOperation)) {
        // TODO: Error position
        createError(mapAntlrTokenPosition(context->assignmentOp), InvalidAssignOperation);
        allSemanticChecksOk = false;
    }

    sharedData->shouldSkipSignalAccessRestrictionCheck = false;
    // TODO: Check if signal width of left and right operand of assign statement match (similar to swap statement ?)
    const auto assignmentOpRhsOperand = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->expression());
    allSemanticChecksOk &= assignmentOpRhsOperand.has_value();
    sharedData->shouldSkipSignalAccessRestrictionCheck = true;

    if (allSemanticChecksOk) {
        const auto lhsOperandVariableAccess                          = *(*assignedToSignal)->getAsVariableAccess();
        const auto lhsOperandAccessedDimensionsNumValuesPerDimension = getSizeOfSignalAfterAccess(lhsOperandVariableAccess);
        const auto rhsOperandNumValuesPerAccessedDimension           = (*assignmentOpRhsOperand)->numValuesPerDimension;
        if (requiresBroadcasting(lhsOperandAccessedDimensionsNumValuesPerDimension, rhsOperandNumValuesPerAccessedDimension)) {
            if (!sharedData->parserConfig->supportBroadcastingExpressionOperands) {
                createError(determineContextStartTokenPositionOrUseDefaultOne(context->signal()), fmt::format(MissmatchedNumberOfDimensionsBetweenOperands, lhsOperandAccessedDimensionsNumValuesPerDimension.size(), rhsOperandNumValuesPerAccessedDimension.size()));
                invalidateStoredValueFor(lhsOperandVariableAccess);
            } else {
                createError(determineContextStartTokenPositionOrUseDefaultOne(context->signal()), "Broadcasting of operands is currently not supported");
                invalidateStoredValueFor(lhsOperandVariableAccess);
            }
        } else if (checkIfNumberOfValuesPerDimensionMatchOrLogError(mapAntlrTokenPosition(context->signal()->start), lhsOperandAccessedDimensionsNumValuesPerDimension, rhsOperandNumValuesPerAccessedDimension)) {
            // TODO: CONSTANT_PROPAGATION: Invalidate stored values if rhs is not constant
            const auto& assignedToSignalParts = *(*assignedToSignal)->getAsVariableAccess();

            const auto& containerOfExpressionContainingNewValue = (*assignmentOpRhsOperand)->getAsExpression();
            const auto& exprContainingNewValue = *containerOfExpressionContainingNewValue;
            // We need to update the currently stored value for the assigned to signal if the rhs expression evaluates to a constant, otherwise invalidate the stored value for the former
            tryUpdateOrInvalidateStoredValueFor(assignedToSignalParts, exprContainingNewValue);

            // TODO: Add mapping from custom operation enum to internal "numeric"
            addStatementToOpenContainer(
                    std::make_shared<syrec::AssignStatement>(
                            assignedToSignalParts,
                            *ParserUtilities::mapOperationToInternalFlag(*definedAssignmentOperation),
                            exprContainingNewValue));
        }
        else {
            invalidateStoredValueFor(lhsOperandVariableAccess);
        }
        /*
         * Theoretically we can ignore out of range accesses since the would not destroy the value for any valid value of a dimension.
         * What should happen if either the bit range access or value of dimension access is out of range (the latter should be covered by the above statement) or is malformed (i.e. syntax errors). Our visitor would not return any value, so what should we update now
         * Should we opt to just invalidate the whole signal (even if we could identify some of the accessed dimensions ?
         *
         * The error handling for such cases should be done in the visitor of the signal (add flag that we are parsing the assigned to signal part to shared data and add handling to signal visitor production).
         * Name this flag invalidateSignalValueOnError since the same error handling needs to be done in case of inout/out parameters in call statements
         */
    } else if (assignedToSignal.has_value() && (*assignedToSignal)->isVariableAccess()) {
        // If we can determine which parts of the signal on the lhs of the assignment statement were accessed, we simply invalidate them in case not all semantic checks for the latter where correct
        const auto& assignedToSignalParts = *(*assignedToSignal)->getAsVariableAccess();
        invalidateStoredValueFor(assignedToSignalParts);
    }

    if (sharedData->optionalExpectedExpressionSignalWidth.has_value()) {
        sharedData->optionalExpectedExpressionSignalWidth.reset();
    }

    if (needToResetSignalRestriction) {
        liftRestrictionToAssignedToPartOfSignal(*(*assignedToSignal)->getAsVariableAccess(), determineContextStartTokenPositionOrUseDefaultOne(context));
    }

    return 0;
}

std::any SyReCStatementVisitor::visitCallStatement(SyReCParser::CallStatementContext* context) {
    bool                      isValidCallOperationDefined = context->OP_CALL() != nullptr || context->OP_UNCALL() != nullptr;
    const std::optional<bool> isCallOperation             = isValidCallOperationDefined ? std::make_optional(context->OP_CALL() != nullptr) : std::nullopt;

    std::optional<std::string>          moduleIdent;
    bool                                existsModuleForIdent = false;
    std::optional<ModuleCallGuess::ptr> potentialModulesToCall;
    TokenPosition                       moduleIdentPosition = TokenPosition::createFallbackPosition();

    if (context->moduleIdent != nullptr) {
        moduleIdent.emplace(context->moduleIdent->getText());
        moduleIdentPosition  = TokenPosition(context->moduleIdent->getLine(), context->moduleIdent->getCharPositionInLine());
        existsModuleForIdent = checkIfSignalWasDeclaredOrLogError(*moduleIdent, false, moduleIdentPosition);
        if (existsModuleForIdent) {
            potentialModulesToCall = ModuleCallGuess::fetchPotentialMatchesForMethodIdent(sharedData->currentSymbolTableScope, *moduleIdent);
        }
        isValidCallOperationDefined &= existsModuleForIdent;
    } else {
        moduleIdentPosition         = TokenPosition(context->start->getLine(), context->start->getCharPositionInLine());
        isValidCallOperationDefined = false;
    }

    if (isCallOperation.has_value()) {
        isValidCallOperationDefined &= areSemanticChecksForCallOrUncallDependingOnNameValid(*isCallOperation, moduleIdentPosition, moduleIdent);
    }

    std::vector<std::string> calleeArguments;
    for (const auto& userDefinedCallArgument: context->calleeArguments) {
        if (userDefinedCallArgument == nullptr) {
            continue;
        }

        calleeArguments.emplace_back(userDefinedCallArgument->getText());
        const auto callArgumentTokenPosition = TokenPosition(userDefinedCallArgument->getLine(), userDefinedCallArgument->getCharPositionInLine());
        if (checkIfSignalWasDeclaredOrLogError(userDefinedCallArgument->getText(), false, callArgumentTokenPosition) && potentialModulesToCall.has_value()) {
            const auto symTabEntryForCalleeArgument = *sharedData->currentSymbolTableScope->getVariable(userDefinedCallArgument->getText());
            if (std::holds_alternative<syrec::Variable::ptr>(symTabEntryForCalleeArgument)) {
                (*potentialModulesToCall)->refineGuessWithNextParameter(std::get<syrec::Variable::ptr>(symTabEntryForCalleeArgument));
                continue;
            }
        }
        isValidCallOperationDefined = false;
    }

    if (!moduleIdent.has_value()) {
        return 0;
    }

    if (isCallOperation.has_value()) {
        if (*isCallOperation) {
            moduleCallStack->addNewEntry(std::make_shared<ModuleCallStack::CallStackEntry>(*moduleIdent, calleeArguments, sharedData->currentModuleCallNestingLevel, std::make_pair(moduleIdentPosition.line, moduleIdentPosition.column)));
        } else if (
                !*isCallOperation && moduleCallStack->containsAnyUncalledModulesForNestingLevel(sharedData->currentModuleCallNestingLevel) && (*moduleCallStack->getLastCalledModuleForNestingLevel(sharedData->currentModuleCallNestingLevel))->moduleIdent == *moduleIdent) {
            if (!doArgumentsBetweenCallAndUncallMatch(moduleIdentPosition, *moduleIdent, calleeArguments)) {
                isValidCallOperationDefined = false;
            }
            // Since the idents of the modules in the last call and the current uncall statements matched, we can safely pop the top entry from the call stack
            moduleCallStack->removeLastCalledModule();
        }
    }

    // If the no module was declared for the given ident we do not want to create errors related to a missmatch between the formal and actual parameters since the former do not exist
    if (!existsModuleForIdent) {
        return 0;
    }

    if (!potentialModulesToCall.has_value()) {
        createError(mapAntlrTokenPosition(context->moduleIdent), fmt::format(NoMatchForGuessWithNActualParameters, *moduleIdent, calleeArguments.size()));
        return 0;
    }

    (*potentialModulesToCall)->discardGuessesWithMoreThanNParameters(calleeArguments.size());
    if (!(*potentialModulesToCall)->hasSomeMatches()) {
        createError(mapAntlrTokenPosition(context->moduleIdent), fmt::format(NoMatchForGuessWithNActualParameters, *moduleIdent, calleeArguments.size()));
        return 0;
    }

    if ((*potentialModulesToCall)->getMatchesForGuess().size() > 1) {
        // TODO: GEN_ERROR Ambigous call, more than one match for given arguments
        // TODO: Error position
        createError(mapAntlrTokenPosition(context->moduleIdent), fmt::format(AmbigousCall, *moduleIdent));

        
        // TODO: UNUSED_REFERENCE - Marked as used
        /*
         * Since we have an ambiguous call, we will mark all modules matching the current call signature as used
         */
        for (const auto& moduleMatchingCall: (*potentialModulesToCall)->getMatchesForGuess()) {
            sharedData->currentSymbolTableScope->incrementReferenceCountOfModulesMatchingSignature(moduleMatchingCall);   
        }
        return 0;
    }

    // TODO: UNUSED_REFERENCE - Marked as used
    /*
     * At this point we know that there must be a matching module for the current call/uncall statement, so we mark the former as used.
     * We do this before checking whether the call is valid (at this point it would either indicate a missmatch between the formal and actual parameters)
     * or a semantic error between a call / uncall
     */
    sharedData->currentSymbolTableScope->incrementReferenceCountOfModulesMatchingSignature((*potentialModulesToCall)->getMatchesForGuess().front());

    if (!isValidCallOperationDefined) {
        return 0;
    }

    const auto moduleMatchingCalleeArguments = (*potentialModulesToCall)->getMatchesForGuess().front();
    if (*isCallOperation) {
        // TODO: CONSTANT_PROPAGATION
        invalidateValuesForVariablesUsedAsParametersChangeableByModuleCall(moduleMatchingCalleeArguments, calleeArguments);
        addStatementToOpenContainer(std::make_shared<syrec::CallStatement>(moduleMatchingCalleeArguments, calleeArguments));
    } else {
        addStatementToOpenContainer(std::make_shared<syrec::UncallStatement>(moduleMatchingCalleeArguments, calleeArguments));
    }

    return 0;
}

std::any SyReCStatementVisitor::visitLoopStepsizeDefinition(SyReCParser::LoopStepsizeDefinitionContext* context) {
    // TODO: Handling of negative step size
    const bool isNegativeStepSize = context->OP_MINUS() != nullptr;
    return tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->number());
}

std::any SyReCStatementVisitor::visitLoopVariableDefinition(SyReCParser::LoopVariableDefinitionContext* context) {
    return context->variableIdent != nullptr ? std::make_optional("$" + context->IDENT()->getText()) : std::nullopt;
}

std::any SyReCStatementVisitor::visitForStatement(SyReCParser::ForStatementContext* context) {
    const auto loopVariableIdent = tryVisitAndConvertProductionReturnValue<std::string>(context->loopVariableDefinition());
    bool       loopHeaderValid   = true;
    if (loopVariableIdent.has_value()) {
        if (sharedData->currentSymbolTableScope->contains(*loopVariableIdent)) {
            createError(mapAntlrTokenPosition(context->loopVariableDefinition()->variableIdent), fmt::format(DuplicateDeclarationOfIdent, *loopVariableIdent));
            loopHeaderValid = false;
        } else {
            SymbolTable::openScope(sharedData->currentSymbolTableScope);
            // TODO: Since we are currently assuming that the value of a signal can be stored in at most 32 bits, we assume the latter as the bit width of the loop variable value
            sharedData->currentSymbolTableScope->addEntry(std::make_shared<syrec::Number>(*loopVariableIdent), 32, std::nullopt);
            sharedData->lastDeclaredLoopVariable.emplace(*loopVariableIdent);
        }
    }

    const bool                        wasStartValueExplicitlyDefined = context->startValue != nullptr;
    std::optional<syrec::Number::ptr> rangeStartValue;
    if (wasStartValueExplicitlyDefined) {
        rangeStartValue = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->startValue);
        loopHeaderValid &= rangeStartValue.has_value();
    }
    if (!rangeStartValue.has_value()) {
        rangeStartValue.emplace(std::make_shared<syrec::Number>(0));
    }
    sharedData->lastDeclaredLoopVariable.reset();

    std::optional<syrec::Number::ptr> rangeEndValue = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->endValue);
    loopHeaderValid &= rangeEndValue.has_value();
    if (!rangeEndValue.has_value()) {
        rangeEndValue.emplace(std::make_shared<syrec::Number>(0));
    }

    const bool                        wasStepSizeExplicitlyDefined = context->loopStepsizeDefinition() != nullptr;
    std::optional<syrec::Number::ptr> stepSize;
    if (wasStepSizeExplicitlyDefined) {
        stepSize = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->loopStepsizeDefinition());
        loopHeaderValid &= stepSize.has_value();
    } else {
        stepSize.emplace(std::make_shared<syrec::Number>(1));
    }

    if (loopHeaderValid && (wasStartValueExplicitlyDefined ? canEvaluateNumber(*rangeStartValue) : true) && canEvaluateNumber(*rangeEndValue) && (wasStepSizeExplicitlyDefined ? canEvaluateNumber(*stepSize) : true)) {
        const std::optional<unsigned int> stepSizeEvaluated = tryEvaluateNumber(*stepSize, determineContextStartTokenPositionOrUseDefaultOne(context->loopStepsizeDefinition()));
        if (stepSizeEvaluated.has_value()) {
            std::optional<unsigned int> iterationRangeStartValueEvaluated;
            std::optional<unsigned int> iterationRangeEndValueEvaluated;
            const bool                  negativeStepSizeDefined = *stepSizeEvaluated < 0;

            const auto potentialEndValueEvaluationErrorTokenStart   = context->endValue->start;
            const auto potentialStartValueEvaluationErrorTokenStart = wasStartValueExplicitlyDefined ? context->startValue->start : potentialEndValueEvaluationErrorTokenStart;

            if (negativeStepSizeDefined) {
                iterationRangeStartValueEvaluated = tryEvaluateNumber(*rangeEndValue, mapAntlrTokenPosition(potentialEndValueEvaluationErrorTokenStart));
                iterationRangeEndValueEvaluated   = tryEvaluateNumber(*rangeStartValue, mapAntlrTokenPosition(potentialStartValueEvaluationErrorTokenStart));
            } else {
                iterationRangeStartValueEvaluated = tryEvaluateNumber(*rangeStartValue, mapAntlrTokenPosition(potentialStartValueEvaluationErrorTokenStart));
                iterationRangeEndValueEvaluated   = tryEvaluateNumber(*rangeEndValue, mapAntlrTokenPosition(potentialEndValueEvaluationErrorTokenStart));
            }

            loopHeaderValid &= iterationRangeStartValueEvaluated.has_value() && iterationRangeEndValueEvaluated.has_value();
            if (loopHeaderValid) {
                unsigned int numIterations = 0;
                if (negativeStepSizeDefined && *iterationRangeStartValueEvaluated < *iterationRangeEndValueEvaluated) {
                    createError(mapAntlrTokenPosition(potentialStartValueEvaluationErrorTokenStart), fmt::format(InvalidLoopVariableValueRangeWithNegativeStepsize, *iterationRangeStartValueEvaluated, *iterationRangeEndValueEvaluated, *stepSizeEvaluated));
                    loopHeaderValid = false;
                } else if (!negativeStepSizeDefined && *iterationRangeStartValueEvaluated > *iterationRangeEndValueEvaluated) {
                    createError(mapAntlrTokenPosition(potentialStartValueEvaluationErrorTokenStart), fmt::format(InvalidLoopVariableValueRangeWithPositiveStepsize, *iterationRangeStartValueEvaluated, *iterationRangeEndValueEvaluated, *stepSizeEvaluated));
                    loopHeaderValid = false;
                } else if (stepSizeEvaluated != 0) {
                    numIterations = (negativeStepSizeDefined ? (*iterationRangeStartValueEvaluated - *iterationRangeEndValueEvaluated) : (*iterationRangeEndValueEvaluated - *iterationRangeStartValueEvaluated)) + 1;
                    numIterations /= *stepSizeEvaluated;
                }
            }
        }
    }

    // TODO: It seems like with syntax error in the components of the loop header the statementList will be null
    const auto loopBody        = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(context->statementList());
    const bool isValidLoopBody = loopBody.has_value() && !(*loopBody).empty();

    // TODO: Instead of opening and closing a new scope simply insert and remove the entry from the symbol table
    if (loopVariableIdent.has_value()) {
        SymbolTable::closeScope(sharedData->currentSymbolTableScope);
    }

    // TODO: If a statement must be generated, one could create a skip statement instead of simply returning
    if (loopHeaderValid && isValidLoopBody) {
        const auto loopStatement    = std::make_shared<syrec::ForStatement>();
        loopStatement->loopVariable = loopVariableIdent.has_value() ? *loopVariableIdent : "";
        loopStatement->range        = std::pair(wasStartValueExplicitlyDefined ? *rangeStartValue : *rangeEndValue, *rangeEndValue);
        loopStatement->step         = *stepSize;
        loopStatement->statements   = *loopBody;
        addStatementToOpenContainer(loopStatement);
    }

    if (loopVariableIdent.has_value() && sharedData->evaluableLoopVariables.find(*loopVariableIdent) != sharedData->evaluableLoopVariables.end()) {
        sharedData->evaluableLoopVariables.erase(*loopVariableIdent);
    }

    return 0;
}

std::any SyReCStatementVisitor::visitIfStatement(SyReCParser::IfStatementContext* context) {
    const auto guardExpression        = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->guardCondition);
    const auto trueBranchStmts        = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(context->trueBranchStmts);
    const auto falseBranchStmts       = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(context->falseBranchStmts);
    const auto closingGuardExpression = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->matchingGuardExpression);

    // TODO: Replace pointer comparison with actual equality check of expressions
    if (!guardExpression.has_value() || (!trueBranchStmts.has_value() || (*trueBranchStmts).empty()) || (!falseBranchStmts.has_value() || (*falseBranchStmts).empty()) || !closingGuardExpression.has_value()) {
        return 0;
    }

    // TODO: Error position
    if (!areExpressionsEqual(*guardExpression, *closingGuardExpression)) {
        createError(mapAntlrTokenPosition(context->getStart()), IfAndFiConditionMissmatch);
        return 0;
    }

    const auto ifStatement      = std::make_shared<syrec::IfStatement>();
    ifStatement->condition      = (*guardExpression)->getAsExpression().value();
    ifStatement->thenStatements = *trueBranchStmts;
    ifStatement->elseStatements = *falseBranchStmts;
    ifStatement->fiCondition    = (*closingGuardExpression)->getAsExpression().value();
    addStatementToOpenContainer(ifStatement);
    return 0;
}

std::any SyReCStatementVisitor::visitSkipStatement(SyReCParser::SkipStatementContext* context) {
    if (context->start != nullptr) {
        addStatementToOpenContainer(std::make_shared<syrec::SkipStatement>());
    }
    return 0;
}

// TODO: CONSTANT_PROPAGATION: Swapping restrictions and values
// TODO: CONSTANT_PROPAGATOIN: Maybe find a better solution to toggleing constant propagation
std::any SyReCStatementVisitor::visitSwapStatement(SyReCParser::SwapStatementContext* context) {
    const auto backupOfConstantPropagationFlag = sharedData->parserConfig->performConstantPropagation;

    sharedData->parserConfig->performConstantPropagation = false;
    const auto swapLhsOperand                            = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->lhsOperand);
    sharedData->parserConfig->performConstantPropagation = backupOfConstantPropagationFlag;
    const bool lhsOperandOk   = swapLhsOperand.has_value() && (*swapLhsOperand)->isVariableAccess() && isSignalAssignableOtherwiseCreateError(context->lhsOperand->IDENT()->getSymbol(), *(*swapLhsOperand)->getAsVariableAccess());

    sharedData->parserConfig->performConstantPropagation = false;
    const auto swapRhsOperand                            = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->rhsOperand);
    sharedData->parserConfig->performConstantPropagation = backupOfConstantPropagationFlag;
    const bool rhsOperandOk   = swapRhsOperand.has_value() && (*swapRhsOperand)->isVariableAccess() && isSignalAssignableOtherwiseCreateError(context->rhsOperand->IDENT()->getSymbol(), *(*swapRhsOperand)->getAsVariableAccess());

    // In case any of the operands of the swap operation cannot be determined, we try to check if we could at least determine one of them. If that is the case, invalidate the accessed signal parts of the latter.
    if (!lhsOperandOk || !rhsOperandOk) {
        if (swapLhsOperand.has_value() && (*swapLhsOperand)->isVariableAccess()) {
            const auto lhsAccessedSignal = *(*swapLhsOperand)->getAsVariableAccess();
            invalidateStoredValueFor(lhsAccessedSignal);
        } else if (swapRhsOperand.has_value() && (*swapRhsOperand)->isVariableAccess()) {
            const auto rhsAccessedSignal = *(*swapRhsOperand)->getAsVariableAccess();
            invalidateStoredValueFor(rhsAccessedSignal);
        }
        return 0;
    }

    const auto lhsAccessedSignal = *(*swapLhsOperand)->getAsVariableAccess();
    const auto rhsAccessedSignal = *(*swapRhsOperand)->getAsVariableAccess();

    bool allSemanticChecksOk = true;

    size_t lhsNumAffectedDimensions = 1;
    size_t rhsNumAffectedDimensions = 1;
    if (lhsAccessedSignal->indexes.empty() && !lhsAccessedSignal->getVar()->dimensions.empty()) {
        lhsNumAffectedDimensions = lhsAccessedSignal->getVar()->dimensions.size();
    }

    if (rhsAccessedSignal->indexes.empty() && !rhsAccessedSignal->getVar()->dimensions.empty()) {
        rhsNumAffectedDimensions = rhsAccessedSignal->getVar()->dimensions.size();
    }

    if (lhsNumAffectedDimensions != rhsNumAffectedDimensions) {
        // TODO: Error position
        createError(mapAntlrTokenPosition(context->start), fmt::format(InvalidSwapNumDimensionsMissmatch, lhsNumAffectedDimensions, rhsNumAffectedDimensions));
        invalidateStoredValueFor(lhsAccessedSignal);
        invalidateStoredValueFor(rhsAccessedSignal);
        allSemanticChecksOk = false;
    } else if (lhsAccessedSignal->indexes.empty() && rhsAccessedSignal->indexes.empty()) {
        const auto& lhsSignalDimensions = lhsAccessedSignal->getVar()->dimensions;
        const auto& rhsSignalDimensions = rhsAccessedSignal->getVar()->dimensions;

        bool continueCheck = true;
        for (size_t dimensionIdx = 0; continueCheck && dimensionIdx < lhsSignalDimensions.size(); ++dimensionIdx) {
            continueCheck = lhsSignalDimensions.at(dimensionIdx) == rhsSignalDimensions.at(dimensionIdx);
            if (!continueCheck) {
                // TODO: Error position
                createError(mapAntlrTokenPosition(context->start), fmt::format(InvalidSwapValueForDimensionMissmatch, dimensionIdx, lhsSignalDimensions.at(dimensionIdx), rhsSignalDimensions.at(dimensionIdx)));
                invalidateStoredValueFor(lhsAccessedSignal);
                invalidateStoredValueFor(rhsAccessedSignal);
                allSemanticChecksOk = false;
            }
        }
    }

    if (!allSemanticChecksOk) {
        return 0;
    }

    const auto lhsAccessedSignalWidthEvaluated = tryDetermineBitwidthAfterVariableAccess(lhsAccessedSignal, determineContextStartTokenPositionOrUseDefaultOne(context->lhsOperand));
    const auto rhsAccessedSignalWidthEvaluated = tryDetermineBitwidthAfterVariableAccess(rhsAccessedSignal, determineContextStartTokenPositionOrUseDefaultOne(context->rhsOperand));

    if (lhsAccessedSignalWidthEvaluated.has_value() && rhsAccessedSignalWidthEvaluated.has_value() && *lhsAccessedSignalWidthEvaluated != *rhsAccessedSignalWidthEvaluated) {
        // TODO: Error position
        createError(mapAntlrTokenPosition(context->start), fmt::format(InvalidSwapSignalWidthMissmatch, *lhsAccessedSignalWidthEvaluated, *rhsAccessedSignalWidthEvaluated));
        invalidateStoredValueFor(lhsAccessedSignal);
        invalidateStoredValueFor(rhsAccessedSignal);
        return 0;
    }

    if (areAccessedValuesForDimensionAndBitsConstant(lhsAccessedSignal) && areAccessedValuesForDimensionAndBitsConstant(rhsAccessedSignal)) {
        sharedData->currentSymbolTableScope->swap(lhsAccessedSignal, rhsAccessedSignal);
    }
    else {
        invalidateStoredValueFor(lhsAccessedSignal);
        invalidateStoredValueFor(rhsAccessedSignal);
    }


    addStatementToOpenContainer(std::make_shared<syrec::SwapStatement>(lhsAccessedSignal, rhsAccessedSignal));
    return 0;
}

// TODO: CONSTANT_PROPAGATION: Update of signal value for unary statement (i.e. ++= a;)
std::any SyReCStatementVisitor::visitUnaryStatement(SyReCParser::UnaryStatementContext* context) {
    bool       allSemanticChecksOk = true;
    const auto unaryOperation      = getDefinedOperation(context->unaryOp);

    if (!unaryOperation.has_value() || (syrec_operation::operation::invert_assign != *unaryOperation && syrec_operation::operation::increment_assign != *unaryOperation && syrec_operation::operation::decrement_assign != *unaryOperation)) {
        allSemanticChecksOk = false;
        // TODO: Error position
        createError(mapAntlrTokenPosition(context->unaryOp), InvalidUnaryOperation);
    }

    const auto backupOfConstantPropagationFlag = sharedData->parserConfig->performConstantPropagation;

    sharedData->parserConfig->performConstantPropagation = false;
    // TODO: Is a sort of broadcasting check required (i.e. if a N-D signal is accessed and broadcasting is not supported an error should be created and otherwise the statement will be simplified [one will be created for every accessed dimension instead of one for the whole signal])
    const auto accessedSignal                            = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->signal());
    sharedData->parserConfig->performConstantPropagation = backupOfConstantPropagationFlag;

    allSemanticChecksOk &= accessedSignal.has_value() && (*accessedSignal)->isVariableAccess() && isSignalAssignableOtherwiseCreateError(context->signal()->IDENT()->getSymbol(), *(*accessedSignal)->getAsVariableAccess());
    if (allSemanticChecksOk) {
        // TODO: Add mapping from custom operation enum to internal "numeric" flag
        addStatementToOpenContainer(std::make_shared<syrec::UnaryStatement>(*ParserUtilities::mapOperationToInternalFlag(*unaryOperation), *(*accessedSignal)->getAsVariableAccess()));
    }

    return 0;
}

void SyReCStatementVisitor::addStatementToOpenContainer(const syrec::Statement::ptr& statement) {
    statementListContainerStack.top().emplace_back(statement);
}

bool SyReCStatementVisitor::areSemanticChecksForCallOrUncallDependingOnNameValid(bool isCallOperation, const TokenPosition& moduleIdentTokenPosition, const std::optional<std::string>& moduleIdent) {
    if (isCallOperation) {
        if (moduleCallStack->containsAnyUncalledModulesForNestingLevel(sharedData->currentModuleCallNestingLevel)) {
            createError(moduleIdentTokenPosition, PreviousCallWasNotUncalled);
            return false;
        }
    } else {
        if (!moduleCallStack->containsAnyUncalledModulesForNestingLevel(sharedData->currentModuleCallNestingLevel)) {
            createError(moduleIdentTokenPosition, UncallWithoutPreviousCall);
            return false;
        }

        if (!moduleIdent.has_value()) {
            return false;
        }

        const auto& lastCalledModule = *moduleCallStack->getLastCalledModuleForNestingLevel(sharedData->currentModuleCallNestingLevel);
        if (lastCalledModule->moduleIdent != *moduleIdent) {
            createError(moduleIdentTokenPosition, fmt::format(MissmatchOfModuleIdentBetweenCalledAndUncall, lastCalledModule->moduleIdent, *moduleIdent));
            return false;
        }
    }
    return true;
}

bool SyReCStatementVisitor::doArgumentsBetweenCallAndUncallMatch(const TokenPosition& positionOfPotentialError, const std::string& uncalledModuleIdent, const std::vector<std::string>& calleeArguments) {
    const auto lastCalledModule = *moduleCallStack->getLastCalledModuleForNestingLevel(sharedData->currentModuleCallNestingLevel);
    if (lastCalledModule->moduleIdent != uncalledModuleIdent) {
        return false;
    }

    bool        argumentsMatched          = true;
    const auto& argumentsOfCall           = lastCalledModule->calleeArguments;
    size_t      numCalleeArgumentsToCheck = calleeArguments.size();

    if (numCalleeArgumentsToCheck != argumentsOfCall.size()) {
        createError(positionOfPotentialError, fmt::format(NumberOfParametersMissmatchBetweenCallAndUncall, uncalledModuleIdent, argumentsOfCall.size(), calleeArguments.size()));
        numCalleeArgumentsToCheck = std::min(numCalleeArgumentsToCheck, argumentsOfCall.size());
        argumentsMatched          = false;
    }

    std::vector<std::string> missmatchedParameterValues;
    for (std::size_t parameterIdx = 0; parameterIdx < numCalleeArgumentsToCheck; ++parameterIdx) {
        const auto& argumentOfCall   = argumentsOfCall.at(parameterIdx);
        const auto& argumentOfUncall = calleeArguments.at(parameterIdx);

        if (argumentOfCall != argumentOfUncall) {
            missmatchedParameterValues.emplace_back(fmt::format(ParameterValueMissmatch, parameterIdx, argumentOfUncall, argumentOfCall));
            argumentsMatched = false;
        }
    }

    if (!missmatchedParameterValues.empty()) {
        std::ostringstream missmatchedParameterErrorsBuffer;
        std::copy(missmatchedParameterValues.cbegin(), missmatchedParameterValues.cend(), infix_ostream_iterator<std::string>(missmatchedParameterErrorsBuffer, ","));
        createError(positionOfPotentialError, fmt::format(CallAndUncallArgumentsMissmatch, uncalledModuleIdent, missmatchedParameterErrorsBuffer.str()));
    }

    return argumentsMatched;
}

bool SyReCStatementVisitor::areExpressionsEqual(const ExpressionEvaluationResult::ptr& firstExpr, const ExpressionEvaluationResult::ptr& otherExpr) {
    const bool isFirstExprConstant = firstExpr->isConstantValue();
    const bool isOtherExprConstant = otherExpr->isConstantValue();

    if (isFirstExprConstant ^ isOtherExprConstant) {
        return false;
    }

    if (isFirstExprConstant && isOtherExprConstant) {
        return *firstExpr->getAsConstant() == *otherExpr->getAsConstant();
    }

    return areSyntacticallyEquivalent(firstExpr->getAsExpression().value(), otherExpr->getAsExpression().value());
}

void SyReCStatementVisitor::restrictAccessToAssignedToPartOfSignal(const syrec::VariableAccess::ptr& assignedToSignalPart, const TokenPosition& optionalEvaluationErrorPosition) {
    // a[$i][2][3].0
    // a[2][3][($i + 2)].$i:0
    // a[2][($i + ($i + 5))] = 0
    // a[$i]
    // a[$i].0:($i + ($i / 10)) = 0
    // a

    const std::string& assignedToSignalIdent   = assignedToSignalPart->getVar()->name;
    auto               signalAccessRestriction = tryGetSignalAccessRestriction(assignedToSignalIdent);
    if (!signalAccessRestriction.has_value()) {
        sharedData->signalAccessRestrictions.insert({assignedToSignalIdent, SignalAccessRestriction(assignedToSignalPart->getVar())});
        signalAccessRestriction.emplace(*tryGetSignalAccessRestriction(assignedToSignalIdent));
    }

    const std::optional<SignalAccessRestriction::SignalAccess> evaluatedBitOrRangeAccess =
            assignedToSignalPart->range.has_value() ? tryEvaluateBitOrRangeAccess(*assignedToSignalPart->range, optionalEvaluationErrorPosition) : std::nullopt;

    if (evaluatedBitOrRangeAccess.has_value() && signalAccessRestriction->isAccessRestrictedToBitRange(*evaluatedBitOrRangeAccess)) {
        return;
    }

    if (assignedToSignalPart->indexes.empty()) {
        if (!evaluatedBitOrRangeAccess.has_value()) {
            signalAccessRestriction->blockAccessOnSignalCompletely();
        } else {
            signalAccessRestriction->restrictAccessToBitRange(*evaluatedBitOrRangeAccess);
        }
        updateSignalRestriction(assignedToSignalIdent, signalAccessRestriction.value());
        return;
    }

    for (std::size_t dimensionIdx = 0; dimensionIdx < assignedToSignalPart->indexes.size(); ++dimensionIdx) {
        if (signalAccessRestriction->isAccessRestrictedToDimension(dimensionIdx)) {
            return;
        }

        std::optional<unsigned int> accessedValueForDimension = std::nullopt;
        if (auto const* numeric = dynamic_cast<syrec::NumericExpression*>(assignedToSignalPart->indexes.at(dimensionIdx).get())) {
            if (canEvaluateNumber(numeric->value)) {
                accessedValueForDimension = tryEvaluateNumber(numeric->value, optionalEvaluationErrorPosition);
            }
        }

        if (accessedValueForDimension.has_value() && signalAccessRestriction->isAccessRestrictedToValueOfDimension(dimensionIdx, accessedValueForDimension.value())) {
            return;
        }
    }

    const std::size_t           lastDimensionIdx          = assignedToSignalPart->indexes.size() - 1;
    std::optional<unsigned int> accessedValueForDimension = std::nullopt;
    if (auto const* numeric = dynamic_cast<syrec::NumericExpression*>(assignedToSignalPart->indexes.at(lastDimensionIdx).get())) {
        if (canEvaluateNumber(numeric->value)) {
            accessedValueForDimension = tryEvaluateNumber(numeric->value, optionalEvaluationErrorPosition);
        }
    }

    if (accessedValueForDimension.has_value()) {
        if (evaluatedBitOrRangeAccess.has_value()) {
            signalAccessRestriction->restrictAccessToBitRange(lastDimensionIdx, *accessedValueForDimension, *evaluatedBitOrRangeAccess);
        } else {
            signalAccessRestriction->restrictAccessToValueOfDimension(lastDimensionIdx, *accessedValueForDimension);
        }
    } else {
        if (evaluatedBitOrRangeAccess.has_value()) {
            signalAccessRestriction->restrictAccessToBitRange(lastDimensionIdx, *evaluatedBitOrRangeAccess);
        } else {
            if (lastDimensionIdx == 0) {
                /*
                  * Accessing either a bit range for which we cannot determine which bits were accessed or accessing any value of the first dimension,
                  * allows one to restrict the access to the whole signal for both 1-D as well as N-D signals since the restriction status of subsequent dimensions
                  * depends on the prior ones
                 */
                signalAccessRestriction->blockAccessOnSignalCompletely();
            } else {
                // Accessing either a bit range for which we cannot determine which bits were accessed or accessing any value of a dimension, allows one to restrict the access to the whole dimension
                signalAccessRestriction->restrictAccessToDimension(lastDimensionIdx);
            }
        }
    }
    updateSignalRestriction(assignedToSignalIdent, signalAccessRestriction.value());
}

void SyReCStatementVisitor::liftRestrictionToAssignedToPartOfSignal(const syrec::VariableAccess::ptr& assignedToSignalPart, const TokenPosition& optionalEvaluationErrorPosition) {
    auto signalAccessRestriction = tryGetSignalAccessRestriction(assignedToSignalPart->getVar()->name);
    if (!signalAccessRestriction.has_value()) {
        return;
    }

    const std::string                                          assignedToSignalIdent      = assignedToSignalPart->getVar()->name;
    const bool                                                 wasBitOrRangeAccessDefined = assignedToSignalPart->range.has_value();
    const std::optional<SignalAccessRestriction::SignalAccess> evaluatedBitOrRangeAccess =
            wasBitOrRangeAccessDefined ? tryEvaluateBitOrRangeAccess(*assignedToSignalPart->range, optionalEvaluationErrorPosition) : std::nullopt;

    if (assignedToSignalPart->indexes.empty()) {
        if (evaluatedBitOrRangeAccess.has_value()) {
            signalAccessRestriction->liftAccessRestrictionForBitRange(*evaluatedBitOrRangeAccess);
            updateSignalRestriction(assignedToSignalIdent, signalAccessRestriction.value());
        } else {
            sharedData->signalAccessRestrictions.erase(assignedToSignalIdent);
        }
        return;
    }

    const std::size_t           lastAccessedDimensionIdx = assignedToSignalPart->indexes.size() - 1;
    std::optional<unsigned int> accessedValueForDimension;
    const auto&                 accessedDimensionExpr = assignedToSignalPart->indexes.at(lastAccessedDimensionIdx);
    if (auto const* numeric = dynamic_cast<syrec::NumericExpression*>(accessedDimensionExpr.get())) {
        accessedValueForDimension = canEvaluateNumber(numeric->value) ? tryEvaluateNumber(numeric->value, optionalEvaluationErrorPosition) : std::nullopt;
    }

    if (accessedValueForDimension.has_value()) {
        if (evaluatedBitOrRangeAccess.has_value()) {
            signalAccessRestriction->liftAccessRestrictionForBitRange(lastAccessedDimensionIdx, *accessedValueForDimension, *evaluatedBitOrRangeAccess);
        } else {
            signalAccessRestriction->liftAccessRestrictionForValueOfDimension(lastAccessedDimensionIdx, *accessedValueForDimension);
        }
        updateSignalRestriction(assignedToSignalIdent, signalAccessRestriction.value());
    } else {
        if (evaluatedBitOrRangeAccess.has_value()) {
            signalAccessRestriction->liftAccessRestrictionForBitRange(lastAccessedDimensionIdx, *evaluatedBitOrRangeAccess);
        } else {
            if (assignedToSignalPart->indexes.size() == 1) {
                sharedData->signalAccessRestrictions.erase(assignedToSignalIdent);
                return;
            }
            signalAccessRestriction->liftAccessRestrictionForDimension(lastAccessedDimensionIdx);
        }
        updateSignalRestriction(assignedToSignalIdent, signalAccessRestriction.value());
    }
}

// TODO: CONSTANT_PROPAGATION: Handling of out of range indizes for dimension or bit range
// TODO: CONSTANT_PROPAGATION: In case the rhs expr was a constant, should we trim its value in case it cannot be stored in the lhs ?
void SyReCStatementVisitor::tryUpdateOrInvalidateStoredValueFor(const syrec::VariableAccess::ptr& assignedToVariableParts, const syrec::expression::ptr& exprContainingNewValue) const {
    const auto invalidateAccessedBitRange = assignedToVariableParts->range.has_value()
        && (!assignedToVariableParts->range->first->isConstant() || !assignedToVariableParts->range->second->isConstant());

    const auto accessedAnyNonConstantValueOfDimension = std::any_of(
            assignedToVariableParts->indexes.cbegin(),
            assignedToVariableParts->indexes.cend(),
            [](const syrec::expression::ptr& valueOfDimensionExpr) {
                if (const auto& numericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(valueOfDimensionExpr); numericExpr != nullptr) {
                    return numericExpr->value->isConstant();
                }
                return true;
            });

    const auto& constantValueOfExprContainingNewValue = std::dynamic_pointer_cast<syrec::NumericExpression>(exprContainingNewValue);
    const auto& variableToCopyValuesAndRestrictionsFrom           = std::dynamic_pointer_cast<syrec::VariableExpression>(exprContainingNewValue);

    /*
     * We should invalidate the assigned to signal parts if we cannot determine:
     * I.   Which bits were accessed (this could be either if the expression is not constant or one is using a loop variable in a loop that will not be unrolled)
     * II.  Which value of any accessed dimension will be effected (this could be either if the expression is not constant or one is using a loop variable in a loop that will not be unrolled)
     * III. The rhs side of the assignment is not a constant value or a variable access
     * IV.  Either I. or II. does not hold for the defined variable access on the rhs
     *
     */
    bool shouldInvalidateInsteadOfUpdateAssignedToVariable = 
        !areAccessedValuesForDimensionAndBitsConstant(assignedToVariableParts)
        || (constantValueOfExprContainingNewValue == nullptr && variableToCopyValuesAndRestrictionsFrom == nullptr);

    if (!shouldInvalidateInsteadOfUpdateAssignedToVariable && constantValueOfExprContainingNewValue != nullptr) {
        shouldInvalidateInsteadOfUpdateAssignedToVariable &= constantValueOfExprContainingNewValue->value->isConstant();
    }

    if (shouldInvalidateInsteadOfUpdateAssignedToVariable) {
        invalidateStoredValueFor(assignedToVariableParts);
    }
    else {
        if (constantValueOfExprContainingNewValue != nullptr) {
            sharedData->currentSymbolTableScope->updateStoredValueFor(assignedToVariableParts, constantValueOfExprContainingNewValue->value->evaluate({}));
        }
        else {
            if (!areAccessedValuesForDimensionAndBitsConstant(variableToCopyValuesAndRestrictionsFrom->var)) {
                invalidateStoredValueFor(assignedToVariableParts);
            } else {
                sharedData->currentSymbolTableScope->updateViaSignalAssignment(assignedToVariableParts, variableToCopyValuesAndRestrictionsFrom->var);   
            }
        }
    }
}

// TODO: CONSTANT_PROPAGATION: We are assuming that no loop variables or constant can be passed as parameter and that all passed callee arguments were declared
void SyReCStatementVisitor::invalidateValuesForVariablesUsedAsParametersChangeableByModuleCall(const syrec::Module::ptr& calledModule, const std::vector<std::string>& calleeArguments) const {
    std::size_t parameterIdx = 0;
    for (const auto& formalCalleeArgument: calledModule->parameters) {
        if (formalCalleeArgument->type == syrec::Variable::Types::Inout || formalCalleeArgument->type == syrec::Variable::Types::Out) {
            const auto& symbolTableEntryForParameter = *sharedData->currentSymbolTableScope->getVariable(calleeArguments.at(parameterIdx));

            auto variableAccessOnCalleeArgument = std::make_shared<syrec::VariableAccess>();
            variableAccessOnCalleeArgument->setVar(std::get<syrec::Variable::ptr>(symbolTableEntryForParameter));

            sharedData->currentSymbolTableScope->invalidateStoredValuesFor(variableAccessOnCalleeArgument);
        }
        parameterIdx++;
    }
}

void SyReCStatementVisitor::invalidateStoredValueFor(const syrec::VariableAccess::ptr& assignedToVariableParts) const {
    sharedData->currentSymbolTableScope->invalidateStoredValuesFor(assignedToVariableParts);
}

bool SyReCStatementVisitor::areAccessedValuesForDimensionAndBitsConstant(const syrec::VariableAccess::ptr& accessedSignalParts) const {
    const auto invalidateAccessedBitRange = accessedSignalParts->range.has_value() && (!accessedSignalParts->range->first->isConstant() || !accessedSignalParts->range->second->isConstant());

    const auto accessedAnyNonConstantValueOfDimension = std::any_of(
            accessedSignalParts->indexes.cbegin(),
            accessedSignalParts->indexes.cend(),
            [](const syrec::expression::ptr& valueOfDimensionExpr) {
                if (const auto& numericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(valueOfDimensionExpr); numericExpr != nullptr) {
                    return numericExpr->value->isConstant();
                }
                return true;
            });

    return !invalidateAccessedBitRange && !accessedAnyNonConstantValueOfDimension;
}

