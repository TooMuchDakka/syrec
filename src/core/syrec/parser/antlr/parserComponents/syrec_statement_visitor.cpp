#include "core/syrec/parser/antlr/parserComponents/syrec_statement_visitor.hpp"

#include "core/syrec/parser/custom_semantic_errors.hpp"
#include "core/syrec/parser/expression_comparer.hpp"
#include "core/syrec/parser/infix_iterator.hpp"
#include "core/syrec/parser/module_call_guess.hpp"
#include "core/syrec/parser/signal_evaluation_result.hpp"

/*
 * TODO: General todo, when checking if both operands have the same dimensions(and also the same values per accessed dimension)
 * we also need to take partial dimension accesses into account (i.e. a[2][4] += b with a = out a[4][6][2][1] and b in[2][1])
 *
 * TODO: Value of signals is not restored after readonly parsing of loops
 * TODO: Should unused out parameters be removed (check if currently calls are correctly adopted to new function interface when removing parameters)
 * TODO: Tests for simplification of binary expressions (removal of identity element in expressions) when reassociate expression optimization is disabled
 * TODO: No additional line synthesis currently does not handle expressions with nonreversible operations and multiple signal occurrences correctly
 * TODO: Transformation of N-D assignments to 1-D assignments by unrolling
 */ 

using namespace parser;

std::any SyReCStatementVisitor::visitStatementList(SyReCParser::StatementListContext* context) {
    // TODO: If custom casting utility function supports polymorphism as described in the description of the function, this stack is no longer needed
    statementListContainerStack.push({});
    sharedData->currentModuleCallNestingLevel++;

    for (const auto& userDefinedStatement: context->stmts) {
        if (userDefinedStatement) {
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
        std::copy(notUncalledModule->calleeArguments.cbegin(), notUncalledModule->calleeArguments.cend(), InfixIterator<std::string>(bufferForStringifiedParametersOfNotUncalledModule, ","));

        const auto moduleIdentLinePosition   = notUncalledModule->moduleIdentPosition.first;
        const auto moduleIdentColumnPosition = notUncalledModule->moduleIdentPosition.second;
        createError(messageUtils::Message::Position(moduleIdentLinePosition, moduleIdentColumnPosition), MissingUncall, notUncalledModule->moduleIdent, bufferForStringifiedParametersOfNotUncalledModule.str());
    }
    sharedData->currentModuleCallNestingLevel--;

    // Wrapping into std::optional is only needed because helper function to cast return type of production required this wrapper
    return validUserDefinedStatements.empty() ? std::nullopt : std::make_optional(validUserDefinedStatements);
}

std::any SyReCStatementVisitor::visitAssignStatement(SyReCParser::AssignStatementContext* context) {
    sharedData->resetSignalAccessRestriction();
    const auto evaluationResultOfUserDefinedAssignedToSignal       = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->signal());

    auto allSemanticChecksOk = evaluationResultOfUserDefinedAssignedToSignal.has_value() && evaluationResultOfUserDefinedAssignedToSignal->get()->isVariableAccess();
    if (allSemanticChecksOk) {
        if (const auto& assignedToSignalParts = evaluationResultOfUserDefinedAssignedToSignal->get()->getAsVariableAccess(); assignedToSignalParts.has_value()) {
            allSemanticChecksOk = isSignalAssignableOtherwiseCreateError(assignedToSignalParts->get()->var->type, assignedToSignalParts->get()->var->name, mapAntlrTokenPosition(context->signal()->IDENT()->getSymbol()));
        }
    }
    
    const auto& assignedToSignal = allSemanticChecksOk
        ? evaluationResultOfUserDefinedAssignedToSignal->get()->getAsVariableAccess()
        : std::nullopt;

    if (assignedToSignal.has_value()) {
        const auto& bitWidthEvaluationError = mapAntlrTokenPosition(context->start);
        fixExpectedBitwidthToValueIfLatterIsLargerThanCurrentOne(tryDetermineBitwidthAfterVariableAccess(**assignedToSignal, &bitWidthEvaluationError).value_or(assignedToSignal->get()->var->bitwidth));
        sharedData->createSignalAccessRestriction(*assignedToSignal);
    }

    const auto userDefinedAssignmentOperation = getDefinedOperation(context->assignmentOp);
    if (!userDefinedAssignmentOperation.has_value() || !syrec_operation::isOperationAssignmentOperation(*userDefinedAssignmentOperation)) {
        createError(mapAntlrTokenPosition(context->assignmentOp),  InvalidAssignOperation);
        allSemanticChecksOk = false;
    }
    
    // TODO: Check if signal width of left and right operand of assign statement match (similar to swap statement ?)
    const auto assignmentOpRhsOperand = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->expression());
    allSemanticChecksOk &= assignmentOpRhsOperand.has_value();
    
    if (allSemanticChecksOk) {
        const auto  lhsOperandVariableAccess                                 = *assignedToSignal;
        const auto& numDeclaredDimensionsOfAssignedToSignal = lhsOperandVariableAccess->var->dimensions.size();
        const auto& [accessedValuePerDimensionOfLhsOperand, firstNotExplicitlyAccessedDimensionIndexOfLhsOperand] = determineNumAccessedValuesPerDimensionAndFirstNotExplicitlyAccessedDimensionIndex(*lhsOperandVariableAccess);

        const auto& rhsOperandSizeInformation = assignmentOpRhsOperand->get()->determineOperandSize();
        const auto& broadcastingErrorPosition = determineContextStartTokenPositionOrUseDefaultOne(context->signal());
        
        const auto& accessedValuePerDimensionOfRhsOperand = rhsOperandSizeInformation.determineNumAccessedValuesPerDimension();
        if (!doOperandsRequireBroadcastingBasedOnDimensionAccessAndLogError(
                                                                    numDeclaredDimensionsOfAssignedToSignal, firstNotExplicitlyAccessedDimensionIndexOfLhsOperand, accessedValuePerDimensionOfLhsOperand,
                                                                    rhsOperandSizeInformation.numDeclaredDimensionOfOperand, rhsOperandSizeInformation.explicitlyAccessedValuesPerDimension.size(), accessedValuePerDimensionOfRhsOperand,
                    &broadcastingErrorPosition, nullptr)) {

            const auto& isLhsOperand1dSignal = isSizeOfSignalAcessResult1DSignalOrLogError(numDeclaredDimensionsOfAssignedToSignal, firstNotExplicitlyAccessedDimensionIndexOfLhsOperand, accessedValuePerDimensionOfLhsOperand, &broadcastingErrorPosition);
            const auto& rhsOperandSizeCheckErrorPosition = determineContextStartTokenPositionOrUseDefaultOne(context->expression());
            const auto& isRhsOperand1dSignal = isSizeOfSignalAcessResult1DSignalOrLogError(rhsOperandSizeInformation.numDeclaredDimensionOfOperand, rhsOperandSizeInformation.explicitlyAccessedValuesPerDimension.size(), accessedValuePerDimensionOfRhsOperand, &rhsOperandSizeCheckErrorPosition);

            if (isLhsOperand1dSignal && isRhsOperand1dSignal) {
                // TODO: Check for bitwidth match
                const auto assignStmt = std::make_shared<syrec::AssignStatement>(
                        *assignedToSignal,
                        *syrec_operation::tryMapAssignmentOperationEnumToFlag(*userDefinedAssignmentOperation),
                        *(*assignmentOpRhsOperand)->getAsExpression());
                addStatementToOpenContainer(assignStmt);       
            }
        }
    }

    sharedData->optionalExpectedExpressionSignalWidth.reset();
    sharedData->resetSignalAccessRestriction();
    return 0;
}

std::any SyReCStatementVisitor::visitCallStatement(SyReCParser::CallStatementContext* context) {
    bool                      isValidCallOperationDefined = context->OP_CALL() || context->OP_UNCALL();
    const std::optional<bool> isCallOperation             = isValidCallOperationDefined ? std::make_optional(context->OP_CALL() != nullptr) : std::nullopt;

    std::optional<std::string>                      moduleIdent;
    bool                                            existsModuleForIdent = false;
    std::optional<std::unique_ptr<ModuleCallGuess>> moduleCallGuessResolver;
    auto                                            moduleIdentPosition = messageUtils::Message::Position(sharedData->fallbackErrorLinePosition, sharedData->fallbackErrorColumnPosition);

    if (context->moduleIdent != nullptr) {
        moduleIdent.emplace(context->moduleIdent->getText());
        moduleIdentPosition  = messageUtils::Message::Position(context->moduleIdent->getLine(), context->moduleIdent->getCharPositionInLine());
        existsModuleForIdent = checkIfSignalWasDeclaredOrLogError(*moduleIdent, moduleIdentPosition);
        if (existsModuleForIdent) {
            moduleCallGuessResolver = ModuleCallGuess::tryInitializeWithModulesMatchingName(sharedData->currentSymbolTableScope, *moduleIdent);
        }
        isValidCallOperationDefined &= existsModuleForIdent;
    } else {
        moduleIdentPosition         = messageUtils::Message::Position(context->start->getLine(), context->start->getCharPositionInLine());
        isValidCallOperationDefined = false;
    }

    if (isCallOperation.has_value()) {
        isValidCallOperationDefined &= areSemanticChecksForCallOrUncallDependingOnNameValid(*isCallOperation, moduleIdentPosition, moduleIdent);
    }

    std::vector<std::string> callerArguments;
    for (const auto& userDefinedCallArgument: context->calleeArguments) {
        if (!userDefinedCallArgument) {
            continue;
        }

        callerArguments.emplace_back(userDefinedCallArgument->getText());
        const auto callArgumentTokenPosition = messageUtils::Message::Position(userDefinedCallArgument->getLine(), userDefinedCallArgument->getCharPositionInLine());
        if (checkIfSignalWasDeclaredOrLogError(userDefinedCallArgument->getText(), callArgumentTokenPosition) && moduleCallGuessResolver.has_value()) {
            const auto symTabEntryForCalleeArgument = *sharedData->currentSymbolTableScope->getVariable(userDefinedCallArgument->getText());
            if (std::holds_alternative<syrec::Variable::ptr>(symTabEntryForCalleeArgument)) {
                moduleCallGuessResolver->get()->refineGuessWithNextParameter(*std::get<syrec::Variable::ptr>(symTabEntryForCalleeArgument));
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
            moduleCallStack->addNewEntry(std::make_shared<ModuleCallStack::CallStackEntry>(*moduleIdent, callerArguments, sharedData->currentModuleCallNestingLevel, std::make_pair(moduleIdentPosition.line, moduleIdentPosition.column)));
        } else if (
                !*isCallOperation && moduleCallStack->containsAnyUncalledModulesForNestingLevel(sharedData->currentModuleCallNestingLevel) && (*moduleCallStack->getLastCalledModuleForNestingLevel(sharedData->currentModuleCallNestingLevel))->moduleIdent == *moduleIdent) {
            if (!doArgumentsBetweenCallAndUncallMatch(moduleIdentPosition, *moduleIdent, callerArguments)) {
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

    if (!moduleCallGuessResolver.has_value()) {
        createError(mapAntlrTokenPosition(context->moduleIdent), NoMatchForGuessWithNActualParameters, *moduleIdent, callerArguments.size());
        return 0;
    }

    (*moduleCallGuessResolver)->discardGuessesWithMoreThanNParameters(callerArguments.size());
    if (!(*moduleCallGuessResolver)->hasSomeMatches()) {
        createError(mapAntlrTokenPosition(context->moduleIdent),NoMatchForGuessWithNActualParameters, *moduleIdent, callerArguments.size());
        return 0;
    }

    if ((*moduleCallGuessResolver)->getMatchesForGuess().size() > 1) {
        // TODO: GEN_ERROR Ambigous call, more than one match for given arguments
        // TODO: Error position
        createError(mapAntlrTokenPosition(context->moduleIdent),  AmbigousCall, *moduleIdent);
        return 0;
    }

    if (!isValidCallOperationDefined || !isCallOperation.has_value()) {
        return 0;
    }

    const auto               signatureDataOfModuleMatchingCalleeArguments = moduleCallGuessResolver->get()->getMatchesForGuess().front();
    if (const auto& moduleMatchingCallerArguments = sharedData->currentSymbolTableScope->getFullDeclaredModuleInformation(*moduleIdent, signatureDataOfModuleMatchingCalleeArguments.internalModuleId); moduleMatchingCallerArguments.has_value()) {
        syrec::Module::ptr calledModuleContainer = std::make_shared<syrec::Module>(**moduleMatchingCallerArguments);
        if (*isCallOperation) {
            const auto& generatedCallStmt = std::make_shared<syrec::CallStatement>(calledModuleContainer, callerArguments);
            addStatementToOpenContainer(generatedCallStmt);
        } else {
            const auto& generatedUncallStmt = std::make_shared<syrec::UncallStatement>(calledModuleContainer, callerArguments);
            addStatementToOpenContainer(generatedUncallStmt);
        }        
    }
    return 0;
}

std::any SyReCStatementVisitor::visitLoopStepsizeDefinition(SyReCParser::LoopStepsizeDefinitionContext* context) {
    return tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->number());
}

std::any SyReCStatementVisitor::visitLoopVariableDefinition(SyReCParser::LoopVariableDefinitionContext* context) {
    return context->variableIdent ? std::make_optional("$" + context->IDENT()->getText()) : std::nullopt;
}

std::any SyReCStatementVisitor::visitForStatement(SyReCParser::ForStatementContext* context) {
    const auto loopVariableIdent = tryGetLoopVariableIdent(context);
    bool       loopHeaderValid   = true;

    if (loopVariableIdent.has_value() && !loopVariableIdent->empty()) {
        if (sharedData->currentSymbolTableScope->contains(*loopVariableIdent)) {
            createError(mapAntlrTokenPosition(context->loopVariableDefinition()->variableIdent), DuplicateDeclarationOfIdent, *loopVariableIdent);
            loopHeaderValid = false;
        } else {
            // TODO: Since we are currently assuming that the value of a signal can be stored in at most 32 bits, we assume the latter as the bit width of the loop variable value
            sharedData->currentSymbolTableScope->addEntry(syrec::Number(*loopVariableIdent), 32, std::nullopt);
            sharedData->lastDeclaredLoopVariable = loopVariableIdent;
        }
    }

    const bool wasIterationRangeStartExplicitlyDefined = context->startValue;
    const bool wasIterationRangeEndExplicitlyDefined   = context->endValue;
    const bool wasIterationRangeStepSizeExplicitlyDefined = context->loopStepsizeDefinition();

    syrec::Number::ptr implicitDefaultIterationRangeStartValue;
    if (!wasIterationRangeStartExplicitlyDefined) {
        implicitDefaultIterationRangeStartValue = std::make_shared<syrec::Number>(0);
    }
    
    auto iterationRangeStartValue = wasIterationRangeStartExplicitlyDefined ? tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->startValue) : std::make_optional(implicitDefaultIterationRangeStartValue);
    sharedData->lastDeclaredLoopVariable.reset();
    auto iterationRangeEndValue = wasIterationRangeEndExplicitlyDefined ? tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->endValue) : iterationRangeStartValue;
    const auto iterationRangeStepSize = wasIterationRangeStepSizeExplicitlyDefined ? tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->loopStepsizeDefinition()) :  std::make_optional(std::make_shared<syrec::Number>(1));

    
    std::optional<unsigned int> evaluatedValueOfIterationRangeStart = 0u;
    std::optional<unsigned int> evaluatedValueOfIterationRangeEnd   = 0u;
    std::optional<unsigned int> evaluatedValueOfIterationRangeStepSize = 1u;
    const auto                  wasNegativeStepSizeDefined = wasIterationRangeStepSizeExplicitlyDefined && context->loopStepsizeDefinition()->OP_MINUS();

    /*
     * Since we are assuming an implicit iteration range start value of 0 if the iteration range start is not defined by the user,
     * we need to "transfer" this assumption unto the iteration range end value if the latter is not explicitly defined by the user.
     */
    if (wasNegativeStepSizeDefined && !wasIterationRangeStartExplicitlyDefined) {
        iterationRangeStartValue.swap(iterationRangeEndValue);
    }

    loopHeaderValid &= (wasIterationRangeStartExplicitlyDefined ? iterationRangeStartValue.has_value() : true)
        && (wasIterationRangeEndExplicitlyDefined ? iterationRangeEndValue.has_value() : true)
        && (wasIterationRangeStepSizeExplicitlyDefined ? iterationRangeStepSize.has_value() : true);


    const auto potentialEndValueEvaluationErrorTokenStart = context->endValue ? context->endValue->start : nullptr;
    const auto potentialStartValueEvaluationErrorTokenStart = wasIterationRangeStartExplicitlyDefined ? (context->startValue ? context->startValue->start : nullptr) : potentialEndValueEvaluationErrorTokenStart;
    if (loopHeaderValid) {
        if (wasIterationRangeStartExplicitlyDefined) {
            const auto& potentialErrorPosition  = mapAntlrTokenPosition(potentialStartValueEvaluationErrorTokenStart);
            evaluatedValueOfIterationRangeStart = tryEvaluateNumber(**iterationRangeStartValue, &potentialErrorPosition);
        }

        if (wasIterationRangeEndExplicitlyDefined) {
            const auto& potentialErrorPosition = mapAntlrTokenPosition(potentialEndValueEvaluationErrorTokenStart);
            evaluatedValueOfIterationRangeEnd  = tryEvaluateNumber(**iterationRangeEndValue, &potentialErrorPosition);
        }
        
        if (wasIterationRangeStepSizeExplicitlyDefined) {
            const auto& potentialErrorPosition     = determineContextStartTokenPositionOrUseDefaultOne(context->loopStepsizeDefinition());
            evaluatedValueOfIterationRangeStepSize = tryEvaluateNumber(**iterationRangeStepSize, &potentialErrorPosition);
        }
    }

    if (evaluatedValueOfIterationRangeStart.has_value() && evaluatedValueOfIterationRangeEnd.has_value()) {
        if (wasNegativeStepSizeDefined && *evaluatedValueOfIterationRangeStart < *evaluatedValueOfIterationRangeEnd) {
            createError(mapAntlrTokenPosition(potentialStartValueEvaluationErrorTokenStart), InvalidLoopVariableValueRangeWithNegativeStepsize, *evaluatedValueOfIterationRangeStart, *evaluatedValueOfIterationRangeEnd, *evaluatedValueOfIterationRangeStepSize);
        } else if (!wasNegativeStepSizeDefined && *evaluatedValueOfIterationRangeStart > *evaluatedValueOfIterationRangeEnd) {
            createError(mapAntlrTokenPosition(potentialStartValueEvaluationErrorTokenStart), InvalidLoopVariableValueRangeWithPositiveStepsize, *evaluatedValueOfIterationRangeStart, *evaluatedValueOfIterationRangeEnd, *evaluatedValueOfIterationRangeStepSize);
        }
        if (evaluatedValueOfIterationRangeStepSize.has_value() && !*evaluatedValueOfIterationRangeStepSize && *evaluatedValueOfIterationRangeStart != *evaluatedValueOfIterationRangeEnd) {
            if (wasIterationRangeStepSizeExplicitlyDefined) {
                createError(mapAntlrTokenPosition(context->loopStepsizeDefinition()->number()->start), InvalidLoopIterationRangeStepSizeCannotBeZeroWhenStartAndEndValueDiffer, *evaluatedValueOfIterationRangeStart, *evaluatedValueOfIterationRangeEnd);   
            }
            // TODO: Error creation when step size was not explicitly defined
        }
    }

    if (const auto& loopBodyStmts = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(context->statementList()); loopHeaderValid && loopBodyStmts.has_value() && !loopBodyStmts->empty()) {
        const auto loopStatement    = std::make_shared<syrec::ForStatement>();
        loopStatement->loopVariable = loopVariableIdent.value_or("");
        loopStatement->range        = std::make_pair(*iterationRangeStartValue, *iterationRangeEndValue);    
        loopStatement->step         = iterationRangeStepSize.value_or(std::make_shared<syrec::Number>(1u));
        loopStatement->statements   = *loopBodyStmts;
        addStatementToOpenContainer(loopStatement);    
    }

    if (loopVariableIdent.has_value() && !loopVariableIdent->empty()) {
        sharedData->currentSymbolTableScope->removeVariable(*loopVariableIdent);
    }
    return 0;
}

std::optional<std::string> SyReCStatementVisitor::tryGetLoopVariableIdent(SyReCParser::ForStatementContext* loopContext) const {
    return loopContext->loopVariableDefinition() && loopContext->loopVariableDefinition()->IDENT() ? std::make_optional("$" + loopContext->loopVariableDefinition()->IDENT()->getText()) : std::nullopt;
}

// TODO: Add tests that only 1D signals/expressions are allowed as operands in statements
std::any SyReCStatementVisitor::visitIfStatement(SyReCParser::IfStatementContext* context) {
    const auto guardExpression        = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->guardCondition);
    bool       wasBroadcastingRequiredForGuardExpression = false;
    if (guardExpression.has_value()) {
        const auto& sizeInformationOfGuardExpression = guardExpression->get()->determineOperandSize();
        const auto& potentialGuardExpressionBroadcastingError = determineContextStartTokenPositionOrUseDefaultOne(context->guardCondition);
        wasBroadcastingRequiredForGuardExpression             = doOperandsRequireBroadcastingBasedOnDimensionAccessAndLogError(
                            sizeInformationOfGuardExpression.numDeclaredDimensionOfOperand, sizeInformationOfGuardExpression.explicitlyAccessedValuesPerDimension.size(), sizeInformationOfGuardExpression.determineNumAccessedValuesPerDimension(),
                            1, 1, {1}, &potentialGuardExpressionBroadcastingError, nullptr);
    }


    const auto numUserDefinedStmtsInTrueBranch = context->trueBranchStmts ? context->trueBranchStmts->stmts.size() : 0;
    auto       trueBranchStmts                 = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(context->trueBranchStmts);

    const auto numUserDefinedStmtsInFalseBranch = context->falseBranchStmts ? context->falseBranchStmts->stmts.size() : 0;
    auto       falseBranchStmts                 = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(context->falseBranchStmts);
    const auto closingGuardExpression           = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->matchingGuardExpression);

    if (!guardExpression.has_value() || !closingGuardExpression.has_value() || !numUserDefinedStmtsInTrueBranch || !numUserDefinedStmtsInFalseBranch) {
        return 0;
    }

    if (!areExpressionsEqual(**guardExpression, **closingGuardExpression)) {
        createError(mapAntlrTokenPosition(context->getStart()),  IfAndFiConditionMissmatch);
        return 0;
    }

    if (wasBroadcastingRequiredForGuardExpression) {
        return 0;
    }

    const auto isTrueBranchEmpty  = !trueBranchStmts.has_value() || trueBranchStmts->empty();
    const auto isFalseBranchEmpty = !falseBranchStmts.has_value() || falseBranchStmts->empty();
      if (isTrueBranchEmpty) {
        trueBranchStmts = syrec::Statement::vec();
        insertSkipStatementIfStatementListIsEmpty(*trueBranchStmts);
    }
    if (isFalseBranchEmpty) {
        falseBranchStmts = syrec::Statement::vec();
        insertSkipStatementIfStatementListIsEmpty(*falseBranchStmts);
    }

    const auto& generatedIfStatement = std::make_shared<syrec::IfStatement>();
    generatedIfStatement->condition = *guardExpression->get()->getAsExpression();
    generatedIfStatement->thenStatements = *trueBranchStmts;
    generatedIfStatement->elseStatements = *falseBranchStmts;
    generatedIfStatement->fiCondition = *closingGuardExpression->get()->getAsExpression();
    addStatementToOpenContainer(generatedIfStatement);
    return 0;
}

std::any SyReCStatementVisitor::visitSkipStatement(SyReCParser::SkipStatementContext* context) {
    if (context->start) {
        addStatementToOpenContainer(std::make_shared<syrec::SkipStatement>());
    }
    return 0;
}

std::any SyReCStatementVisitor::visitSwapStatement(SyReCParser::SwapStatementContext* context) {
    const auto swapLhsOperand                            = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->lhsOperand);
    auto lhsOperandOk   = swapLhsOperand.has_value() && (*swapLhsOperand)->isVariableAccess();
    if (lhsOperandOk) {
        if (const auto& signalPartsOfSwapLhsOperand = swapLhsOperand->get()->getAsVariableAccess(); signalPartsOfSwapLhsOperand.has_value()) {
            lhsOperandOk = isSignalAssignableOtherwiseCreateError(signalPartsOfSwapLhsOperand->get()->var->type, signalPartsOfSwapLhsOperand->get()->var->name, mapAntlrTokenPosition(context->lhsOperand->IDENT()->getSymbol()));
        }
    }

    const auto swapRhsOperand                            = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->rhsOperand);
    auto       rhsOperandOk   = swapRhsOperand.has_value() && (*swapRhsOperand)->isVariableAccess();
    if (rhsOperandOk) {
        if (const auto& signalPartsOfSwapRhsOperand = swapRhsOperand->get()->getAsVariableAccess(); signalPartsOfSwapRhsOperand.has_value()) {
            rhsOperandOk = isSignalAssignableOtherwiseCreateError(signalPartsOfSwapRhsOperand->get()->var->type, signalPartsOfSwapRhsOperand->get()->var->name, mapAntlrTokenPosition(context->rhsOperand->IDENT()->getSymbol()));
        }
    }
    
    if (!lhsOperandOk || !rhsOperandOk) {
        return 0;
    }

    const auto lhsAccessedSignal = *(*swapLhsOperand)->getAsVariableAccess();
    const auto rhsAccessedSignal = *(*swapRhsOperand)->getAsVariableAccess();

    bool        allSemanticChecksOk                                                                           = true;
    const auto& broadCastingErrorPosition                                                                     = mapAntlrTokenPosition(context->start);
    const auto& numDeclaredDimensionsOfAccessedSignalOfLhsOperand                                             = lhsAccessedSignal->var->dimensions.size();
    const auto& numDeclaredDimensionsOfAccessedSignalOfRhsOperand                                             = rhsAccessedSignal->var->dimensions.size();
    const auto& [accessedValuePerDimensionOfLhsOperand, firstNotExplicitlyAccessedDimensionIndexOfLhsOperand] = determineNumAccessedValuesPerDimensionAndFirstNotExplicitlyAccessedDimensionIndex(*lhsAccessedSignal);
    const auto& [accessedValuePerDimensionOfRhsOperand, firstNotExplicitlyAccessedDimensionIndexOfRhsOperand] = determineNumAccessedValuesPerDimensionAndFirstNotExplicitlyAccessedDimensionIndex(*rhsAccessedSignal);
    allSemanticChecksOk                   = !doOperandsRequireBroadcastingBasedOnDimensionAccessAndLogError(
        numDeclaredDimensionsOfAccessedSignalOfLhsOperand, firstNotExplicitlyAccessedDimensionIndexOfLhsOperand, accessedValuePerDimensionOfLhsOperand,
        numDeclaredDimensionsOfAccessedSignalOfRhsOperand, firstNotExplicitlyAccessedDimensionIndexOfRhsOperand, accessedValuePerDimensionOfRhsOperand, 
        &broadCastingErrorPosition, nullptr
    ) && allSemanticChecksOk;
    
    if (!allSemanticChecksOk) {
        return 0;
    }

    const auto& lhsOperandPotentialBitwidthEvaluationErrorPosition = determineContextStartTokenPositionOrUseDefaultOne(context->lhsOperand);
    const auto& rhsOperandPotentialBitwidthEvaluationErrorPosition = determineContextStartTokenPositionOrUseDefaultOne(context->rhsOperand);
    const auto& lhsAccessedSignalWidthEvaluated = tryDetermineBitwidthAfterVariableAccess(*lhsAccessedSignal, &lhsOperandPotentialBitwidthEvaluationErrorPosition);
    const auto& rhsAccessedSignalWidthEvaluated = tryDetermineBitwidthAfterVariableAccess(*rhsAccessedSignal, &rhsOperandPotentialBitwidthEvaluationErrorPosition);
    if (lhsAccessedSignalWidthEvaluated.has_value() && rhsAccessedSignalWidthEvaluated.has_value() && doOperandsRequiredBroadcastingBasedOnBitwidthAndLogError(*lhsAccessedSignalWidthEvaluated, *rhsAccessedSignalWidthEvaluated, &broadCastingErrorPosition)) {
        return 0;
    }
    
    const auto& isLhsOperand1DSignal = isSizeOfSignalAcessResult1DSignalOrLogError(numDeclaredDimensionsOfAccessedSignalOfLhsOperand, firstNotExplicitlyAccessedDimensionIndexOfLhsOperand, accessedValuePerDimensionOfLhsOperand, &lhsOperandPotentialBitwidthEvaluationErrorPosition);
    const auto& isRhsOperand1DSignal = isSizeOfSignalAcessResult1DSignalOrLogError(numDeclaredDimensionsOfAccessedSignalOfRhsOperand, firstNotExplicitlyAccessedDimensionIndexOfRhsOperand, accessedValuePerDimensionOfRhsOperand, &rhsOperandPotentialBitwidthEvaluationErrorPosition);
    if (isLhsOperand1DSignal && isRhsOperand1DSignal) {
        addStatementToOpenContainer(std::make_shared<syrec::SwapStatement>(lhsAccessedSignal, rhsAccessedSignal));        
    }
    return 0;
}

std::any SyReCStatementVisitor::visitUnaryStatement(SyReCParser::UnaryStatementContext* context) {
    auto userDefinedUnaryAssignmentOperation      = getDefinedOperation(context->unaryOp);
    if (!userDefinedUnaryAssignmentOperation.has_value() || !syrec_operation::isOperationUnaryAssignmentOperation(*userDefinedUnaryAssignmentOperation)) {
        // TODO: Error position
        createError(mapAntlrTokenPosition(context->unaryOp),  InvalidUnaryOperation);
        userDefinedUnaryAssignmentOperation.reset();
    }
    
    if (const auto evaluationResultOfUserDefinedSignalAccess = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->signal()); evaluationResultOfUserDefinedSignalAccess.has_value() && evaluationResultOfUserDefinedSignalAccess->get()->isVariableAccess()) {
        const auto& assignedToSignalParts = *evaluationResultOfUserDefinedSignalAccess->get()->getAsVariableAccess();
        if (!isSignalAssignableOtherwiseCreateError(assignedToSignalParts->var->type, assignedToSignalParts->var->name, mapAntlrTokenPosition(context->signal()->IDENT()->getSymbol())) || !userDefinedUnaryAssignmentOperation.has_value()) {
            return 0;
        }

        /*
         * Since broadcasting of operands is currently not supported, we need to check whether the number of not explicitly defined dimensions
         * of the assigned to signal is not larger than one.
         */
        const auto& numDeclaredDimensionsOfAccessedSignal = assignedToSignalParts->var->dimensions.size();
        const auto& broadcastingErrorPosition             = determineContextStartTokenPositionOrUseDefaultOne(context->signal());
        const auto& [accessedValuePerDimension, firstNotExplicitlyAccessedDimension] = determineNumAccessedValuesPerDimensionAndFirstNotExplicitlyAccessedDimensionIndex(*assignedToSignalParts);

        valueBroadcastCheck::DimensionAccessMissmatchResult broadcastBasedOnDimensionAccessResult;
        if (!doOperandsRequireBroadcastingBasedOnDimensionAccessAndLogError(numDeclaredDimensionsOfAccessedSignal, firstNotExplicitlyAccessedDimension, accessedValuePerDimension,
            1, 0, {1}, nullptr, &broadcastBasedOnDimensionAccessResult)) {
            const auto& generatedUnaryAssignmentStmt = std::make_shared<syrec::UnaryStatement>(
                    *syrec_operation::tryMapUnaryAssignmentOperationEnumToFlag(*userDefinedUnaryAssignmentOperation),
                    assignedToSignalParts);
            addStatementToOpenContainer(generatedUnaryAssignmentStmt);            
        } else {
            if (broadcastBasedOnDimensionAccessResult.missmatchReason == valueBroadcastCheck::DimensionAccessMissmatchResult::SizeOfResult) {
                createError(broadcastingErrorPosition, UnaryAssignmentOperandNot1DSignal, broadcastBasedOnDimensionAccessResult.numNotExplicitlyAccessedDimensionPerOperand.first);
            } else if (broadcastBasedOnDimensionAccessResult.missmatchReason == valueBroadcastCheck::DimensionAccessMissmatchResult::AccessedValueOfAnyDimension && !broadcastBasedOnDimensionAccessResult.valuePerNotExplicitlyAccessedDimensionMissmatch.empty()) {
                const auto& missmatchInAccessedValuesOfLastDimension = broadcastBasedOnDimensionAccessResult.valuePerNotExplicitlyAccessedDimensionMissmatch.back();
                createError(broadcastingErrorPosition, UnaryAssignmentOperandMoreThanOneValueOfDimensionAccessed, missmatchInAccessedValuesOfLastDimension.expectedNumberOfValues, missmatchInAccessedValuesOfLastDimension.dimensionIdx);
            }
            else {
                createError(broadcastingErrorPosition, FallbackBroadcastCheckFailedErrorBasedOnDimensionAccess);
            }
        }
    }
    return 0;
}

void SyReCStatementVisitor::addStatementToOpenContainer(const syrec::Statement::ptr& statement) {
    if (statementListContainerStack.empty()) {
        return;
    }
    statementListContainerStack.top().emplace_back(statement);
}

bool SyReCStatementVisitor::areSemanticChecksForCallOrUncallDependingOnNameValid(bool isCallOperation, const messageUtils::Message::Position& moduleIdentTokenPosition, const std::optional<std::string>& moduleIdent) {
    if (isCallOperation) {
        if (moduleCallStack->containsAnyUncalledModulesForNestingLevel(sharedData->currentModuleCallNestingLevel)) {
            createError(moduleIdentTokenPosition,  PreviousCallWasNotUncalled);
            return false;
        }
    } else {
        if (!moduleCallStack->containsAnyUncalledModulesForNestingLevel(sharedData->currentModuleCallNestingLevel)) {
            createError(moduleIdentTokenPosition,  UncallWithoutPreviousCall);
            return false;
        }

        if (!moduleIdent.has_value()) {
            return false;
        }

        const auto& lastCalledModule = *moduleCallStack->getLastCalledModuleForNestingLevel(sharedData->currentModuleCallNestingLevel);
        if (lastCalledModule->moduleIdent != *moduleIdent) {
            createError(moduleIdentTokenPosition,  MissmatchOfModuleIdentBetweenCalledAndUncall, lastCalledModule->moduleIdent, *moduleIdent);
            return false;
        }
    }
    return true;
}

bool SyReCStatementVisitor::doArgumentsBetweenCallAndUncallMatch(const messageUtils::Message::Position& positionOfPotentialError, const std::string_view& uncalledModuleIdent, const std::vector<std::string>& calleeArguments) {
    const auto lastCalledModule = *moduleCallStack->getLastCalledModuleForNestingLevel(sharedData->currentModuleCallNestingLevel);
    if (lastCalledModule->moduleIdent != uncalledModuleIdent) {
        return false;
    }

    bool        argumentsMatched          = true;
    const auto& argumentsOfCall           = lastCalledModule->calleeArguments;
    size_t      numCalleeArgumentsToCheck = calleeArguments.size();

    if (numCalleeArgumentsToCheck != argumentsOfCall.size()) {
        createError(positionOfPotentialError,  NumberOfParametersMissmatchBetweenCallAndUncall, uncalledModuleIdent, argumentsOfCall.size(), calleeArguments.size());
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
        std::copy(missmatchedParameterValues.cbegin(), missmatchedParameterValues.cend(), InfixIterator<std::string>(missmatchedParameterErrorsBuffer, ","));
        createError(positionOfPotentialError,  CallAndUncallArgumentsMissmatch, uncalledModuleIdent, missmatchedParameterErrorsBuffer.str());
    }

    return argumentsMatched;
}

bool SyReCStatementVisitor::areExpressionsEqual(const ExpressionEvaluationResult& firstExpr, const ExpressionEvaluationResult& otherExpr) {
    const bool isFirstExprConstant = firstExpr.isConstantValue();
    const bool isOtherExprConstant = otherExpr.isConstantValue();

    if (isFirstExprConstant ^ isOtherExprConstant) {
        return false;
    }

    if (isFirstExprConstant && isOtherExprConstant) {
        return *firstExpr.getAsConstant() == *otherExpr.getAsConstant();
    }

    return areSyntacticallyEquivalent(firstExpr.getAsExpression().value(), otherExpr.getAsExpression().value());
}

bool SyReCStatementVisitor::areAccessedValuesForDimensionAndBitsConstant(const syrec::VariableAccess& accessedSignalParts) const {
    auto areAllAccessedSignalAccessComponentsCompileTimeConstants = accessedSignalParts.range.has_value() ? (accessedSignalParts.range->first->isConstant() && accessedSignalParts.range->second->isConstant()) : true;
    areAllAccessedSignalAccessComponentsCompileTimeConstants &= std::all_of(
            accessedSignalParts.indexes.cbegin(),
            accessedSignalParts.indexes.cend(),
            [](const syrec::expression::ptr& valueOfDimensionExpr) {
                if (const auto& numericExpr = dynamic_cast<const syrec::NumericExpression*>(&*valueOfDimensionExpr); numericExpr != nullptr) {
                    return numericExpr->value->isConstant();
                }
                return false;
            });
    return areAllAccessedSignalAccessComponentsCompileTimeConstants;
}

std::vector<std::optional<unsigned int>> SyReCStatementVisitor::evaluateAccessedValuePerDimension(const syrec::expression::vec& accessedValuePerDimension) {
    std::vector<std::optional<unsigned int>> containerForEvaluatedValuesPerDimension;
    containerForEvaluatedValuesPerDimension.reserve(accessedValuePerDimension.size());

    std::transform(
            accessedValuePerDimension.cbegin(),
            accessedValuePerDimension.cend(),
            std::back_inserter(containerForEvaluatedValuesPerDimension),
            [&](const syrec::expression::ptr& expr) -> std::optional<unsigned int> {
                if (const auto& exprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(expr); exprAsNumericExpr != nullptr) {
                    return tryEvaluateNumber(*exprAsNumericExpr->value, nullptr);
                }
                return std::nullopt;
            });
    return containerForEvaluatedValuesPerDimension;
}