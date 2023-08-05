#include "core/syrec/parser/antlr/parserComponents/syrec_statement_visitor.hpp"

#include "core/syrec/parser/custom_semantic_errors.hpp"
#include "core/syrec/parser/expression_comparer.hpp"
#include "core/syrec/parser/expression_evaluation_result.hpp"
#include "core/syrec/parser/infix_iterator.hpp"
#include "core/syrec/parser/module_call_guess.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/signal_access_restriction.hpp"
#include "core/syrec/parser/signal_evaluation_result.hpp"
#include "core/syrec/parser/value_broadcaster.hpp"
#include "core/syrec/parser/optimizations/loop_optimizer.hpp"
#include "core/syrec/parser/utils/binary_expression_simplifier.hpp"
#include "core/syrec/parser/utils/bit_helpers.hpp"
#include "core/syrec/parser/optimizations/no_additional_line_assignment.hpp"
#include "core/syrec/parser/optimizations/reassociate_expression.hpp"
#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/main_additional_line_for_assignment_simplifier.hpp"
#include "core/syrec/parser/utils/loop_body_value_propagation_blocker.hpp"
#include "core/syrec/parser/utils/loop_range_utils.hpp"

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

    sharedData->parserConfig->performConstantPropagation = !sharedData->performingReadOnlyParsingOfLoopBody;
    // We need to disable the value lookup after having parsed the given signal access since constant propagation would otherwise replace the signal access with its fetched value
    sharedData->performPotentialValueLookupForCurrentlyAccessedSignal = false;
    const auto assignedToSignal             = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->signal());
    sharedData->performPotentialValueLookupForCurrentlyAccessedSignal = true;
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

        sharedData->createSignalAccessRestriction(*(*assignedToSignal)->getAsVariableAccess());
        needToResetSignalRestriction = true;
    }

    const auto definedAssignmentOperation = getDefinedOperation(context->assignmentOp);
    if (!definedAssignmentOperation.has_value() || (syrec_operation::operation::XorAssign != *definedAssignmentOperation && syrec_operation::operation::AddAssign != *definedAssignmentOperation && syrec_operation::operation::MinusAssign != *definedAssignmentOperation)) {
        // TODO: Error position
        createError(mapAntlrTokenPosition(context->assignmentOp), InvalidAssignOperation);
        allSemanticChecksOk = false;
    }

    sharedData->shouldSkipSignalAccessRestrictionCheck = false;
    sharedData->currentlyParsingAssignmentStmtRhs      = true;
    // TODO: Check if signal width of left and right operand of assign statement match (similar to swap statement ?)
    const auto assignmentOpRhsOperand = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->expression());
    allSemanticChecksOk &= assignmentOpRhsOperand.has_value();
    sharedData->currentlyParsingAssignmentStmtRhs      = false;
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
            const auto assignStmt = std::make_shared<syrec::AssignStatement>(
                    *(*assignedToSignal)->getAsVariableAccess(),
                    *syrec_operation::tryMapAssignmentOperationEnumToFlag(*definedAssignmentOperation),
                    *(*assignmentOpRhsOperand)->getAsExpression());

            if (const auto& simplificationResultOfAssignment = trySimplifyAssignmentStatement(assignStmt); !simplificationResultOfAssignment.empty()) {
                for (const auto& generatedAssignment : simplificationResultOfAssignment) {
                    performAssignmentOperation(generatedAssignment);
                    addStatementToOpenContainer(generatedAssignment);
                }    
            }
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

    /*
     * TODO: CONSTANT_PROPAGATION:
     * Signal access restrictions can be "trashed" since functionality is already provided by signal value lookup.
     * What we do need is to refactor the dimension propagation blocked functionality into a separate class
     */ 
    if (needToResetSignalRestriction) {
        sharedData->resetSignalAccessRestriction();
    }
    return 0;
}

// TODO: DEAD_CODE_ELIMINATION: Module calls with only in parameters can also be removed
// TODO: DEAD_CODE_ELIMINIATION: Call + uncall of module of empty module (i.e. after dead store removal) can be removed
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
                // We also need to increment the reference count of the callee arguments if we know that the literal references a valid entry in the symbol table
                incrementReferenceCountOfSignal(calleeArguments.back());
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


        if (!sharedData->modificationsOfReferenceCountsDisabled) {
            // TODO: UNUSED_REFERENCE - Marked as used
            /*
            * Since we have an ambiguous call, we will mark all modules matching the current call signature as used
            */
            for (const auto& moduleMatchingCall: (*potentialModulesToCall)->getMatchesForGuess()) {
                sharedData->currentSymbolTableScope->incrementReferenceCountOfModulesMatchingSignature(moduleMatchingCall);
            }    
        }
        return 0;
    }

    if (!sharedData->modificationsOfReferenceCountsDisabled) {
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
    }

    const auto moduleMatchingCalleeArguments = (*potentialModulesToCall)->getMatchesForGuess().front();
    std::set<std::size_t> positionOfParametersThatWillNotBeInvalidatedDueThemBeingUnusedParameters;
    if (sharedData->parserConfig->deadCodeEliminationEnabled) {
        positionOfParametersThatWillNotBeInvalidatedDueThemBeingUnusedParameters = *sharedData->currentSymbolTableScope->getPositionOfUnusedParametersForModule(moduleMatchingCalleeArguments);
    }

    trimAndDecrementReferenceCountOfUnusedCalleeParameters(calleeArguments, positionOfParametersThatWillNotBeInvalidatedDueThemBeingUnusedParameters);
    /*
     * At this point the optimizer cannot determine without additional overhead whether a module call will have a global side effect since this depends on the result of other optimizations
     * that will be performed after the call statement was created. What we can check is, whether the called module consists of only skip statements or has no side effects (i.e. only read only parameters)
     * but we currently do not check whether the module has any global side effects as mentioned initially. Otherwise, we could also omit the call.
     *
     * TODO: Perform relaxed global side effect check by checking whether modifiable parameter are actually modified
     */
    if (sharedData->parserConfig->deadCodeEliminationEnabled 
        && (doesModuleOnlyHaveReadOnlyParameters(moduleMatchingCalleeArguments) 
            || doesModuleOnlyConsistOfSkipStatements(moduleMatchingCalleeArguments))) {
        if (!sharedData->modificationsOfReferenceCountsDisabled) {
            decrementReferenceCountsOfCalledModuleAndActuallyUsedCalleeArguments(moduleMatchingCalleeArguments, calleeArguments);   
        }
    } else {
        syrec::Statement::ptr createdCallOrUncallStmt;
        if (*isCallOperation) {
            // TODO: CONSTANT_PROPAGATION
            invalidateValuesForVariablesUsedAsParametersChangeableByModuleCall(moduleMatchingCalleeArguments, calleeArguments, positionOfParametersThatWillNotBeInvalidatedDueThemBeingUnusedParameters);
            createdCallOrUncallStmt = std::make_shared<syrec::CallStatement>(moduleMatchingCalleeArguments, calleeArguments);
        } else {
            createdCallOrUncallStmt = std::make_shared<syrec::UncallStatement>(moduleMatchingCalleeArguments, calleeArguments);
        }

        if (!calleeArguments.empty()) {
            addStatementToOpenContainer(createdCallOrUncallStmt);
        } else {
            if (!sharedData->modificationsOfReferenceCountsDisabled) {
                sharedData->currentSymbolTableScope->decrementReferenceCountOfModulesMatchingSignature(moduleMatchingCalleeArguments);
            }
        }        
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

/*
 * TODO: LOOP_UNROLLING: During "read-only" parsing run of loop do not modifiy reference counts
 * TODO: LOOP_UNROLLING: Do not perform unroll if iteration range is not known at compile time
 * TODO: LOOP_UNROLLING: Def-use list for loop unrolling to not insert values for signals that are changed in later iterations
 *
 * TODO: DEAD_CODE_ELIMINATION: Support of reference counting for loop variable (and removal/extracting loop invariant code)
 */
std::any SyReCStatementVisitor::visitForStatement(SyReCParser::ForStatementContext* context) {
    if (sharedData->performingReadOnlyParsingOfLoopBody || !sharedData->parserConfig->deadCodeEliminationEnabled) {
        if (const auto& parsedLoopStmt = visitForStatementInReadonlyMode(context); parsedLoopStmt.has_value()) {
            addStatementToOpenContainer(*parsedLoopStmt);
        }
        return std::nullopt;
    }

    visitForStatementWithOptimizationsEnabled(context);
    return std::nullopt;
}


void SyReCStatementVisitor::buildParseRuleInformationFromUnrolledLoops(
    std::vector<antlr4::ParserRuleContext*>& buildParseRuleInformation,
    std::size_t& stmtOffsetFromParentLoop, 
    SyReCParser::ForStatementContext* loopToUnroll, 
    const CustomLoopContextInformation& customLoopContextInformation, 
    std::vector<UnrolledLoopVariableValue>& unrolledLoopVariableValues,
    std::vector<ModifiedLoopHeaderInformation>& modifiedLoopHeaderInformation,
    const optimizations::LoopUnroller::UnrollInformation& unrollInformation) {

    if (std::holds_alternative<optimizations::LoopUnroller::NotModifiedLoopInformation>(unrollInformation.data)) {
        buildParseRuleInformation.emplace_back(loopToUnroll);
        stmtOffsetFromParentLoop++;
        return;
    }

    if (std::holds_alternative<optimizations::LoopUnroller::FullyUnrolledLoopInformation>(unrollInformation.data)
        || std::holds_alternative<optimizations::LoopUnroller::PeeledIterationsOfLoopInformation>(unrollInformation.data)) {
        const bool canLoopBeFullyUnrolled = std::holds_alternative<optimizations::LoopUnroller::FullyUnrolledLoopInformation>(unrollInformation.data);
        
        auto& unrolledLoopInformationPerUnrolledIteration = canLoopBeFullyUnrolled
            ? std::get<optimizations::LoopUnroller::FullyUnrolledLoopInformation>(unrollInformation.data).nestedLoopUnrollInformationPerIteration
            : std::get<optimizations::LoopUnroller::PeeledIterationsOfLoopInformation>(unrollInformation.data).nestedLoopUnrollInformationPerPeeledIteration;

        std::optional<optimizations::LoopUnroller::LoopIterationInfo>                                                                         optionalLoopVariableValueLookup;
        std::optional<std::pair<optimizations::LoopUnroller::LoopIterationInfo, std::vector<optimizations::LoopUnroller::UnrollInformation>>> optionalLoopRemainedUnrollInformation;
        std::size_t numUnrolledIterations;
        if (!canLoopBeFullyUnrolled) {
            const auto& partialUnrollInformation = std::get<optimizations::LoopUnroller::PeeledIterationsOfLoopInformation>(unrollInformation.data);
            numUnrolledIterations = partialUnrollInformation.nestedLoopUnrollInformationPerPeeledIteration.size();
            if (partialUnrollInformation.loopRemainedUnrollInformation.has_value()) {
                optionalLoopRemainedUnrollInformation.emplace(partialUnrollInformation.loopRemainedUnrollInformation.value());    
            }
            optionalLoopVariableValueLookup = partialUnrollInformation.valueLookupForLoopVariableThatWasPeeled;
        }
        else {
            const auto& fullyUnrolledLoopInformation = std::get<optimizations::LoopUnroller::FullyUnrolledLoopInformation>(unrollInformation.data);
            numUnrolledIterations = fullyUnrolledLoopInformation.repetitionPerStatement;
            optionalLoopVariableValueLookup          = fullyUnrolledLoopInformation.valueLookupForLoopVariableThatWasRemovedDueToUnroll;
        }
        
        for (std::size_t i = 0; i < numUnrolledIterations; ++i) {
            if (optionalLoopVariableValueLookup.has_value() && optionalLoopVariableValueLookup->loopVariableIdent.has_value()) {
                const auto& valueOfLoopVariableInCurrentIteration = optionalLoopVariableValueLookup->initialLoopVariableValue + (i * optionalLoopVariableValueLookup->stepSize);
                unrolledLoopVariableValues.emplace_back(UnrolledLoopVariableValue({stmtOffsetFromParentLoop, *optionalLoopVariableValueLookup->loopVariableIdent, valueOfLoopVariableInCurrentIteration}));
            }

            std::size_t nestedLoopIdx = 0;
            for (const auto& stmtContext : loopToUnroll->statementList()->stmts) {
                if (const auto& stmtContextAsLoopStmtContext = dynamic_cast<SyReCParser::ForStatementContext*>(stmtContext); stmtContextAsLoopStmtContext != nullptr) {
                    buildParseRuleInformationFromUnrolledLoops(
                            buildParseRuleInformation,
                            ++stmtOffsetFromParentLoop,
                            stmtContextAsLoopStmtContext,
                            customLoopContextInformation,
                            unrolledLoopVariableValues,
                            modifiedLoopHeaderInformation,
                            unrolledLoopInformationPerUnrolledIteration.at(i).at(++nestedLoopIdx));
                }
                else {
                    buildParseRuleInformation.emplace_back(stmtContext);
                    stmtOffsetFromParentLoop++;
                }
            }
        }

        if (optionalLoopRemainedUnrollInformation.has_value()) {
            const auto& modifiedCurrentLoopHeaderInformation = optionalLoopRemainedUnrollInformation->first;
            modifiedLoopHeaderInformation.emplace_back(ModifiedLoopHeaderInformation({stmtOffsetFromParentLoop++,
                                                                                      modifiedCurrentLoopHeaderInformation.loopVariableIdent,
                                                                                      modifiedCurrentLoopHeaderInformation.initialLoopVariableValue,
                                                                                      modifiedCurrentLoopHeaderInformation.finalLoopVariableValue,
                                                                                      modifiedCurrentLoopHeaderInformation.stepSize}));
            buildParseRuleInformation.emplace_back(loopToUnroll);
        }
    }
}

std::optional<std::string> SyReCStatementVisitor::tryGetLoopVariableIdent(SyReCParser::ForStatementContext* loopContext) {
    if (loopContext->loopVariableDefinition() != nullptr && loopContext->loopVariableDefinition()->IDENT() != nullptr) {
        return std::make_optional(loopContext->loopVariableDefinition()->IDENT()->getText());
    }
    return std::nullopt;
}

// TODO: DEAD_CODE_ELIMINATION: If both branches turn out to be empty due to optimizations we would also have to update the reference counts of the optimized away variables
std::any SyReCStatementVisitor::visitIfStatement(SyReCParser::IfStatementContext* context) {
    const auto guardExpression        = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->guardCondition);

    std::optional<unsigned int> constantValueOfGuardExpr;
    if (guardExpression.has_value()) {
        if ((*guardExpression)->isConstantValue()) {
            constantValueOfGuardExpr = (*guardExpression)->getAsConstant();
        } else {
            const auto& guardConditionAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(*(*guardExpression)->getAsExpression());
            if (guardConditionAsNumericExpr != nullptr && guardConditionAsNumericExpr->value->isConstant()) {
                constantValueOfGuardExpr = (*guardConditionAsNumericExpr).value->evaluate(sharedData->loopVariableMappingLookup);
            }
        }   
    }
    bool canTrueBranchBeOmitted  = constantValueOfGuardExpr.has_value() && constantValueOfGuardExpr <= 0;
    bool canFalseBranchBeOmitted = constantValueOfGuardExpr.has_value() && constantValueOfGuardExpr > 0;
    const bool canAnyBranchBeOmitted   = canTrueBranchBeOmitted || canFalseBranchBeOmitted;

    const bool backupOfStatusOfInCurrentlyInOmittedRegion = sharedData->modificationsOfReferenceCountsDisabled;
    sharedData->modificationsOfReferenceCountsDisabled = canTrueBranchBeOmitted;
    sharedData->createNewLocalSignalValueBackupScope();
    auto trueBranchStmts        = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(context->trueBranchStmts);
    sharedData->modificationsOfReferenceCountsDisabled = backupOfStatusOfInCurrentlyInOmittedRegion;

    const auto& backupOfCurrentValueOfSignalsChangedInTrueBranch = createCopiesOfCurrentValuesOfChangedSignalsAndResetToOriginal(*sharedData->getCurrentLocalSignalValueBackupScope());
    sharedData->popCurrentLocalSignalValueBackupScope();

    sharedData->createNewLocalSignalValueBackupScope();
    sharedData->modificationsOfReferenceCountsDisabled = canFalseBranchBeOmitted;
    auto falseBranchStmts       = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(context->falseBranchStmts);
    sharedData->modificationsOfReferenceCountsDisabled = backupOfStatusOfInCurrentlyInOmittedRegion;
    const auto& backupOfCurrentValueOfSignalsChangedInFalseBranch = createCopiesOfCurrentValuesOfChangedSignalsAndResetToOriginal(*sharedData->getCurrentLocalSignalValueBackupScope());
    sharedData->popCurrentLocalSignalValueBackupScope();

    mergeChangesFromBranchesAndUpdateSymbolTable(backupOfCurrentValueOfSignalsChangedInTrueBranch, backupOfCurrentValueOfSignalsChangedInFalseBranch, canAnyBranchBeOmitted, canTrueBranchBeOmitted);

    const auto closingGuardExpression = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->matchingGuardExpression);

    // TODO: Replace pointer comparison with actual equality check of expressions
    if (!guardExpression.has_value() || !closingGuardExpression.has_value()) {
        return 0;
    }

    /*
     * Do not increment reference count when evaluating unoptimized guard and closing guard expression
     */
    const auto& evaluationResultOfUnoptimizedGuardExpression = getUnoptimizedExpression(context->guardCondition);
    const auto& evaluationResultOfUnoptimizedMatchingGuardExpression = getUnoptimizedExpression(context->matchingGuardExpression);
    const auto& unoptimizedGuardExpr   = *evaluationResultOfUnoptimizedGuardExpression;
    const auto& unoptimizedClosingExpr = *evaluationResultOfUnoptimizedMatchingGuardExpression;
    
    // TODO: Is we are performing constant propagation this check does not work because the value of the variables used in the guard condition could change in the if-statement
    // TODO: If dead code elimination is enabled and we can drop either of the branches we should also update the reference count for the used variables in both the guard as well as the closing expression
    // TODO: Error position
    if (!areExpressionsEqual(unoptimizedGuardExpr, unoptimizedClosingExpr)) {
        createError(mapAntlrTokenPosition(context->getStart()), IfAndFiConditionMissmatch);
        return 0;
    }

    const bool isTrueBranchEmpty = !trueBranchStmts.has_value() || trueBranchStmts->empty();
    const bool isFalseBranchEmpty = !falseBranchStmts.has_value() || falseBranchStmts->empty();
    const bool canWholeIfBeOmitted = (isTrueBranchEmpty && isFalseBranchEmpty) || (canAnyBranchBeOmitted && canTrueBranchBeOmitted && isFalseBranchEmpty) || (canAnyBranchBeOmitted && canFalseBranchBeOmitted && isTrueBranchEmpty);
    
    if (sharedData->parserConfig->deadCodeEliminationEnabled && (canWholeIfBeOmitted || canAnyBranchBeOmitted)) {
        /*
         * Only add the statements of the branch not omitted (when only considering the guard expression) if its contains at least one statement
         */
        if (!canWholeIfBeOmitted) {
            if (canTrueBranchBeOmitted) {
                for (const auto& falseBranchStmt: *falseBranchStmts) {
                    addStatementToOpenContainer(falseBranchStmt);
                }
            } else {
                for (const auto& trueBranchStmt: *trueBranchStmts) {
                    addStatementToOpenContainer(trueBranchStmt);
                }
            }
        }

        if (!unoptimizedGuardExpr->isConstantValue()) {
            /*
             * We only need to decrement the reference count of the variables used either in the guard or closing guard expression since we are assuming they are the same
             */
            decrementReferenceCountForUsedVariablesInExpression(*unoptimizedGuardExpr->getAsExpression());
        }
        return 0;
    }

    if (isTrueBranchEmpty) {
        trueBranchStmts = syrec::Statement::vec();
        insertSkipStatementIfStatementListIsEmpty(*trueBranchStmts);
    }
    if (isFalseBranchEmpty) {
        falseBranchStmts = syrec::Statement::vec();
        insertSkipStatementIfStatementListIsEmpty(*falseBranchStmts);
    }

    const auto ifStatement      = std::make_shared<syrec::IfStatement>();
    ifStatement->condition      = (*guardExpression)->getAsExpression().value();
    ifStatement->thenStatements = *trueBranchStmts;
    ifStatement->elseStatements = *falseBranchStmts;
    /*
     *  Since the value of the variables used in the closing guard expression could have changed in either of the branches, we simply reuse the guard expression here.
     *  We are allowed to do this since we already checked that the guard and closing guard expression matched.
     */
    ifStatement->fiCondition = (*guardExpression)->getAsExpression().value();
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
    sharedData->performPotentialValueLookupForCurrentlyAccessedSignal = false;
    const auto swapLhsOperand                            = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->lhsOperand);
    const bool lhsOperandOk   = swapLhsOperand.has_value() && (*swapLhsOperand)->isVariableAccess() && isSignalAssignableOtherwiseCreateError(context->lhsOperand->IDENT()->getSymbol(), *(*swapLhsOperand)->getAsVariableAccess());
    
    const auto swapRhsOperand                            = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->rhsOperand);
    sharedData->performPotentialValueLookupForCurrentlyAccessedSignal = true;
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

    createAndStoreBackupForAssignedToSignal(lhsAccessedSignal);
    createAndStoreBackupForAssignedToSignal(rhsAccessedSignal);
    if (lhsAccessedSignalWidthEvaluated.has_value() && rhsAccessedSignalWidthEvaluated.has_value() && *lhsAccessedSignalWidthEvaluated != *rhsAccessedSignalWidthEvaluated) {
        // TODO: Error position
        createError(mapAntlrTokenPosition(context->start), fmt::format(InvalidSwapSignalWidthMissmatch, *lhsAccessedSignalWidthEvaluated, *rhsAccessedSignalWidthEvaluated));
        invalidateStoredValueFor(lhsAccessedSignal);
        invalidateStoredValueFor(rhsAccessedSignal);
        return 0;
    }

    if (areAccessedValuesForDimensionAndBitsConstant(lhsAccessedSignal) && areAccessedValuesForDimensionAndBitsConstant(rhsAccessedSignal)
        && !isValuePropagationBlockedDueToLoopDataFlowAnalysis(lhsAccessedSignal) && !isValuePropagationBlockedDueToLoopDataFlowAnalysis(rhsAccessedSignal)) {
        /*
         * We do not perform value updates when constant propagation is disabled, i.e. during the readonly parsing of a loop body
         */
        if (!sharedData->parserConfig->performConstantPropagation) {
            sharedData->currentSymbolTableScope->swap(lhsAccessedSignal, rhsAccessedSignal);   
        }
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

    if (!unaryOperation.has_value() || (syrec_operation::operation::InvertAssign != *unaryOperation && syrec_operation::operation::IncrementAssign != *unaryOperation && syrec_operation::operation::DecrementAssign != *unaryOperation)) {
        allSemanticChecksOk = false;
        // TODO: Error position
        createError(mapAntlrTokenPosition(context->unaryOp), InvalidUnaryOperation);
    }

    sharedData->performPotentialValueLookupForCurrentlyAccessedSignal = false;
    // TODO: Is a sort of broadcasting check required (i.e. if a N-D signal is accessed and broadcasting is not supported an error should be created and otherwise the statement will be simplified [one will be created for every accessed dimension instead of one for the whole signal])
    const auto accessedSignal                            = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->signal());
    sharedData->performPotentialValueLookupForCurrentlyAccessedSignal = true;

    allSemanticChecksOk &= accessedSignal.has_value() && (*accessedSignal)->isVariableAccess() && isSignalAssignableOtherwiseCreateError(context->signal()->IDENT()->getSymbol(), *(*accessedSignal)->getAsVariableAccess());
    if (allSemanticChecksOk) {
        const auto& accessedSignalParts = *(*accessedSignal)->getAsVariableAccess();
        if (const auto& fetchedValueForAccessedSignal = sharedData->currentSymbolTableScope->tryFetchValueForLiteral(accessedSignalParts);
            sharedData->parserConfig->performConstantPropagation && fetchedValueForAccessedSignal.has_value()) {
            const auto newValueGeneratedByAssignment = syrec_operation::apply(*unaryOperation, *fetchedValueForAccessedSignal);
            if (newValueGeneratedByAssignment.has_value()) {
                const auto& containerForNewValue = std::make_shared<syrec::Number>(*newValueGeneratedByAssignment);
                const auto  bitWidthOfAccessedSignalParts = tryDetermineBitwidthAfterVariableAccess(accessedSignalParts, determineContextStartTokenPositionOrUseDefaultOne(context));
                if (bitWidthOfAccessedSignalParts.has_value()) {
                    const auto& wrapperForContainer = std::make_shared<syrec::NumericExpression>(containerForNewValue, *bitWidthOfAccessedSignalParts);
                    tryUpdateOrInvalidateStoredValueFor(accessedSignalParts, wrapperForContainer);   
                } else {
                    invalidateStoredValueFor(accessedSignalParts);
                }
            } else {
                invalidateStoredValueFor(accessedSignalParts);
            }
        }
        else {
            invalidateStoredValueFor(accessedSignalParts);
        }
        addStatementToOpenContainer(std::make_shared<syrec::UnaryStatement>(*syrec_operation::tryMapUnaryAssignmentOperationEnumToFlag(*unaryOperation), accessedSignalParts));
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

[[nodiscard]] std::optional<ExpressionEvaluationResult::ptr> SyReCStatementVisitor::getUnoptimizedExpression(SyReCParser::ExpressionContext* context) {
    const auto backupOfConstantPropagationStatus = sharedData->parserConfig->performConstantPropagation;
    const auto backupOfReassociateConstantPropagationStatus = sharedData->parserConfig->reassociateExpressionsEnabled;
    const auto backupOfCurrentlyInOmittedRegion             = sharedData->modificationsOfReferenceCountsDisabled;

    sharedData->parserConfig->performConstantPropagation = false;
    sharedData->parserConfig->reassociateExpressionsEnabled = false;
    sharedData->modificationsOfReferenceCountsDisabled                    = true;
    const auto& unoptimizedExpression                       = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context);
    sharedData->parserConfig->performConstantPropagation    = backupOfConstantPropagationStatus;
    sharedData->parserConfig->reassociateExpressionsEnabled = backupOfReassociateConstantPropagationStatus;
    sharedData->modificationsOfReferenceCountsDisabled                    = backupOfCurrentlyInOmittedRegion;
    return unoptimizedExpression;
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

// TODO: CONSTANT_PROPAGATION: Handling of out of range indizes for dimension or bit range
// TODO: CONSTANT_PROPAGATION: In case the rhs expr was a constant, should we trim its value in case it cannot be stored in the lhs ?
// TODO: CONSTANT_PROPAGATION: When updating N-D signals with another N-D signal, multiple updates need to be performed (one per accessed value of dimension), i.e. wire a[2][4], b[2][4], a += b => a[0][0] += a[0][0]; a[0][1] += b[0][1] ... a[0][3] += b[0][3]; a[1][0] += b[1][0] ... a[1][3] += a[1][3]
void SyReCStatementVisitor::tryUpdateOrInvalidateStoredValueFor(const syrec::VariableAccess::ptr& assignedToVariableParts, const syrec::expression::ptr& exprContainingNewValue) const {
    if (!sharedData->parserConfig->performConstantPropagation) {
        return;
    }

    // TODO: CONSTANT_PROPAGATION: Find a better way to not work with incorrect signal state in omitted if-condition branch
    /*if (sharedData->modificationsOfReferenceCountsDisabled) {
        return;
    }*/

    const auto& constantValueOfExprContainingNewValue = std::dynamic_pointer_cast<syrec::NumericExpression>(exprContainingNewValue);
    const auto& variableToCopyValuesAndRestrictionsFrom = std::dynamic_pointer_cast<syrec::VariableExpression>(exprContainingNewValue);

    /*
     * We should invalidate the assigned to signal parts if we cannot determine:
     * I.   Which bits were accessed (this could be either if the expression is not constant or one is using a loop variable in a loop that will not be unrolled)
     * II.  Which value of any accessed dimension will be effected (this could be either if the expression is not constant or one is using a loop variable in a loop that will not be unrolled)
     * III. The rhs side of the assignment is not a constant value or a variable access
     * IV.  Either I. or II. does not hold for the defined variable access on the rhs
     * V.   There exists an external restriction for the assigned to parts of the signal
     *
     */
    const auto shouldInvalidateInsteadOfUpdateAssignedToVariable =
        !areAccessedValuesForDimensionAndBitsConstant(assignedToVariableParts)
        || (constantValueOfExprContainingNewValue == nullptr && variableToCopyValuesAndRestrictionsFrom == nullptr)
        || (constantValueOfExprContainingNewValue != nullptr && !constantValueOfExprContainingNewValue->value->isConstant())
        || isValuePropagationBlockedDueToLoopDataFlowAnalysis(assignedToVariableParts);
    

    if (shouldInvalidateInsteadOfUpdateAssignedToVariable) {
        invalidateStoredValueFor(assignedToVariableParts);
    }
    else {
        if (constantValueOfExprContainingNewValue != nullptr) {
            createAndStoreBackupForAssignedToSignal(assignedToVariableParts);
            sharedData->currentSymbolTableScope->updateStoredValueFor(assignedToVariableParts, constantValueOfExprContainingNewValue->value->evaluate({}));
        }
        else {
            const auto typeOfRhsOperandVariable       = variableToCopyValuesAndRestrictionsFrom->var->var->type;
            bool       canRhsOperandVariableHaveValue;
            switch (typeOfRhsOperandVariable) {
                case syrec::Variable::Wire:
                case syrec::Variable::Inout:
                case syrec::Variable::Out:
                    canRhsOperandVariableHaveValue = true;
                    break;
                default:
                    canRhsOperandVariableHaveValue = false;
                    break;
            }

            if (!canRhsOperandVariableHaveValue || !areAccessedValuesForDimensionAndBitsConstant(variableToCopyValuesAndRestrictionsFrom->var)) {
                invalidateStoredValueFor(assignedToVariableParts);
            } else {
                createAndStoreBackupForAssignedToSignal(assignedToVariableParts);
                sharedData->currentSymbolTableScope->updateViaSignalAssignment(assignedToVariableParts, variableToCopyValuesAndRestrictionsFrom->var);   
            }
        }
    }
}

// TODO: CONSTANT_PROPAGATION: We are assuming that no loop variables or constant can be passed as parameter and that all passed callee arguments were declared
void SyReCStatementVisitor::invalidateValuesForVariablesUsedAsParametersChangeableByModuleCall(const syrec::Module::ptr& calledModule, const std::vector<std::string>& calleeArguments, const std::set<std::size_t>& positionsOfParametersLeftUntouched) const {
    if (!sharedData->parserConfig->performConstantPropagation) {
        return;
    }

    std::size_t parameterIdx = 0;
    for (const auto& formalCalleeArgument: calledModule->parameters) {
        if (positionsOfParametersLeftUntouched.count(parameterIdx) != 0) {
            parameterIdx++;
            continue;
        }

        if (formalCalleeArgument->type == syrec::Variable::Types::Inout || formalCalleeArgument->type == syrec::Variable::Types::Out) {
            const auto& symbolTableEntryForParameter = *sharedData->currentSymbolTableScope->getVariable(calleeArguments.at(parameterIdx));

            auto variableAccessOnCalleeArgument = std::make_shared<syrec::VariableAccess>();
            variableAccessOnCalleeArgument->setVar(std::get<syrec::Variable::ptr>(symbolTableEntryForParameter));
            invalidateStoredValueFor(variableAccessOnCalleeArgument);
        }
        parameterIdx++;
    }
}

void SyReCStatementVisitor::invalidateStoredValueFor(const syrec::VariableAccess::ptr& assignedToVariableParts) const {
    if (!sharedData->parserConfig->performConstantPropagation) {
        return;
    }

    createAndStoreBackupForAssignedToSignal(assignedToVariableParts);
    sharedData->currentSymbolTableScope->invalidateStoredValuesFor(assignedToVariableParts);
}


void SyReCStatementVisitor::trimAndDecrementReferenceCountOfUnusedCalleeParameters(std::vector<std::string>& calleeArguments, const std::set<std::size_t>& positionsOfUnusedParameters) const {
    std::size_t numRemovedParameters = 0;
    for (const auto unusedParameterPosition: positionsOfUnusedParameters) {
        const auto positionOfCalleeArgument = unusedParameterPosition - numRemovedParameters++;
        decrementReferenceCountOfSignal(calleeArguments.at(positionOfCalleeArgument));
        calleeArguments.erase(std::next(calleeArguments.begin(), positionOfCalleeArgument));
    }
}

bool SyReCStatementVisitor::areAccessedValuesForDimensionAndBitsConstant(const syrec::VariableAccess::ptr& accessedSignalParts) const {
    const auto invalidateAccessedBitRange = accessedSignalParts->range.has_value() && (!accessedSignalParts->range->first->isConstant() || !accessedSignalParts->range->second->isConstant());

    const auto accessedAnyNonConstantValueOfDimension = std::any_of(
            accessedSignalParts->indexes.cbegin(),
            accessedSignalParts->indexes.cend(),
            [](const syrec::expression::ptr& valueOfDimensionExpr) {
                if (const auto& numericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(valueOfDimensionExpr); numericExpr != nullptr) {
                    return !numericExpr->value->isConstant();
                }
                return true;
            });

    return !invalidateAccessedBitRange && !accessedAnyNonConstantValueOfDimension;
}

void SyReCStatementVisitor::decrementReferenceCountForUsedVariablesInExpression(const syrec::expression::ptr& expression) {
    if (sharedData->modificationsOfReferenceCountsDisabled) {
        return;
    }

    if (const auto& expressionAsVariableAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(expression); expressionAsVariableAccess != nullptr) {
        const auto& accessedVariable = expressionAsVariableAccess->var;
        decrementReferenceCountOfSignal(accessedVariable->var->name);
    }
    else if (const auto& expressionAsBinaryExpression = std::dynamic_pointer_cast<syrec::BinaryExpression>(expression); expressionAsBinaryExpression != nullptr) {
        decrementReferenceCountForUsedVariablesInExpression(expressionAsBinaryExpression->lhs);
        decrementReferenceCountForUsedVariablesInExpression(expressionAsBinaryExpression->rhs);
    }
    else if (const auto& expressionAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expression); expressionAsShiftExpr != nullptr) {
        decrementReferenceCountForUsedVariablesInExpression(expressionAsShiftExpr->lhs);
        if (expressionAsShiftExpr->rhs->isLoopVariable()) {
            decrementReferenceCountOfSignal(expressionAsShiftExpr->rhs->variableName());
        }
    }
    else if (const auto& expressionAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(expression); expressionAsNumericExpr != nullptr) {
        if (expressionAsNumericExpr->value->isLoopVariable()) {
            decrementReferenceCountOfSignal(expressionAsNumericExpr->value->variableName());
        }
    }
}

/*
 * TODO: We need a more elaborate scheme to update the orignal value after both branches have been parsed
 * We cannot simply update signals that were only used in one branch (assuming the other cannot be omitted), we need to invalidate the updated values instead
 * 
 *
 */

SymbolTableBackupHelper::ptr SyReCStatementVisitor::createCopiesOfCurrentValuesOfChangedSignalsAndResetToOriginal(const SymbolTableBackupHelper::ptr& backupOfOriginalValues) const {
    const auto& backupOfCurrentValuesOfSymbolTable = std::make_shared<SymbolTableBackupHelper>();
    for (const auto& identOfChangedSignal: backupOfOriginalValues->getIdentsOfChangedSignals()) {
        backupOfCurrentValuesOfSymbolTable->storeBackupOf(identOfChangedSignal, *sharedData->currentSymbolTableScope->createBackupOfValueOfSignal(identOfChangedSignal));
        sharedData->currentSymbolTableScope->restoreValuesFromBackup(identOfChangedSignal, *backupOfOriginalValues->getBackedupEntryFor(identOfChangedSignal));
    }
    return backupOfCurrentValuesOfSymbolTable;
}

/*
 * TODO: Added parameter if any and which branch was omitted
 */
void SyReCStatementVisitor::mergeChangesFromBranchesAndUpdateSymbolTable(const SymbolTableBackupHelper::ptr& valuesOfChangedSignalsInTrueBranch, const SymbolTableBackupHelper::ptr& valuesOfChangedSignalsInFalseBranch, bool canAnyBranchBeOmitted, bool canTrueBranchBeOmitted) const {
    const auto& changedSignalsInTrueBranch = valuesOfChangedSignalsInTrueBranch->getIdentsOfChangedSignals();
    const auto& changedSignalsInFalseBranch = valuesOfChangedSignalsInFalseBranch->getIdentsOfChangedSignals();

    std::set<std::string> unionOfChangedSignalsOfBothBranches;
    std::set_union(
    changedSignalsInTrueBranch.cbegin(), changedSignalsInTrueBranch.cend(),
    changedSignalsInFalseBranch.cbegin(), changedSignalsInFalseBranch.cend(),
            std::inserter(unionOfChangedSignalsOfBothBranches, unionOfChangedSignalsOfBothBranches.begin()));

    std::set<std::string> changedSignalsInBothBranches;
    std::set_intersection(
    changedSignalsInTrueBranch.cbegin(), changedSignalsInTrueBranch.cend(),
    changedSignalsInFalseBranch.cbegin(), changedSignalsInFalseBranch.cend(),
    std::inserter(changedSignalsInBothBranches, changedSignalsInBothBranches.begin()));   

    for (const auto& changedSignal: unionOfChangedSignalsOfBothBranches) {
        const valueLookup::SignalValueLookup::ptr originalValueOfSignal = *sharedData->currentSymbolTableScope->createBackupOfValueOfSignal(changedSignal);
        if (changedSignalsInBothBranches.count(changedSignal) != 0 && !canAnyBranchBeOmitted) {
            const auto& backupOfValueInTrueBranch = *valuesOfChangedSignalsInTrueBranch->getBackedupEntryFor(changedSignal);
            const auto& backupOfValueInFalseBranch = *valuesOfChangedSignalsInFalseBranch->getBackedupEntryFor(changedSignal);
            originalValueOfSignal->copyRestrictionsAndMergeValuesFromAlternatives(backupOfValueInTrueBranch, backupOfValueInFalseBranch);
        } else {
            const bool  wasSignalOnlyChangedInTrueBranch  = ((canAnyBranchBeOmitted && !canTrueBranchBeOmitted) ? true : changedSignalsInFalseBranch.count(changedSignal) == 0)
                && changedSignalsInTrueBranch.count(changedSignal) != 0;

            const bool canUpdateBeIgnored = (wasSignalOnlyChangedInTrueBranch && canAnyBranchBeOmitted && canTrueBranchBeOmitted) ||
                                            (!wasSignalOnlyChangedInTrueBranch && canAnyBranchBeOmitted && !canTrueBranchBeOmitted);

            if (canUpdateBeIgnored) {
                continue;
            }

            if (wasSignalOnlyChangedInTrueBranch) {
                const auto& backupOfValueInTrueBranch = *valuesOfChangedSignalsInTrueBranch->getBackedupEntryFor(changedSignal);
                if (!canAnyBranchBeOmitted) {
                    originalValueOfSignal->copyRestrictionsAndInvalidateChangedValuesFrom(backupOfValueInTrueBranch);
                }
                else {
                    originalValueOfSignal->copyRestrictionsAndUnrestrictedValuesFrom(
                           {},
                            std::nullopt,
                           {},
                            std::nullopt,
                            *backupOfValueInTrueBranch); 
                }
            } else {
                const auto& backupOfValueInFalseBranch = *valuesOfChangedSignalsInFalseBranch->getBackedupEntryFor(changedSignal);
                if (!canAnyBranchBeOmitted) {
                    originalValueOfSignal->copyRestrictionsAndInvalidateChangedValuesFrom(backupOfValueInFalseBranch);
                } else {
                    originalValueOfSignal->copyRestrictionsAndUnrestrictedValuesFrom(
                            {},
                            std::nullopt,
                            {},
                            std::nullopt,
                            *backupOfValueInFalseBranch);
                }
            }
        }
        sharedData->currentSymbolTableScope->restoreValuesFromBackup(changedSignal, originalValueOfSignal);
    }
}

void SyReCStatementVisitor::createAndStoreBackupForAssignedToSignal(const syrec::VariableAccess::ptr& assignedToSignalParts) const {
    const auto& assignedToSignal = assignedToSignalParts->var->name;
    if (const auto& ifConditionBranchAssignedSignalsBackupHelper = sharedData->getCurrentLocalSignalValueBackupScope(); ifConditionBranchAssignedSignalsBackupHelper.has_value() && !(*ifConditionBranchAssignedSignalsBackupHelper)->hasEntryFor(assignedToSignal)) {
        if (const auto& copyOfCurrentValueOfAssignedToSignal = sharedData->currentSymbolTableScope->createBackupOfValueOfSignal(assignedToSignal); copyOfCurrentValueOfAssignedToSignal.has_value()) {
            ifConditionBranchAssignedSignalsBackupHelper.value()->storeBackupOf(assignedToSignal, *copyOfCurrentValueOfAssignedToSignal);
        }
    }
}

syrec::expression::ptr SyReCStatementVisitor::tryDetermineNewSignalValueFromAssignment(const syrec::VariableAccess::ptr& assignedToSignal, syrec_operation::operation assignmentOp, const syrec::expression::ptr& assignmentStmtRhsExpr) const {
    const auto& assignmentRhsExprAsConstant = std::dynamic_pointer_cast<syrec::NumericExpression>(assignmentStmtRhsExpr);
    const auto& currentValueOfAssignedToSignal = sharedData->currentSymbolTableScope->tryFetchValueForLiteral(assignedToSignal);

    if (assignmentRhsExprAsConstant == nullptr || !assignmentRhsExprAsConstant->value->isConstant() || !currentValueOfAssignedToSignal.has_value()) {
        return assignmentStmtRhsExpr;
    }

    const auto& assignmentRhsExprEvaluated = assignmentRhsExprAsConstant->value->evaluate({});
    return std::make_shared<syrec::NumericExpression>(
            std::make_shared<syrec::Number>(*syrec_operation::apply(assignmentOp, *currentValueOfAssignedToSignal, assignmentRhsExprEvaluated)),
            assignmentStmtRhsExpr->bitwidth());
}

std::optional<SyReCStatementVisitor::ParsedLoopHeaderInformation> SyReCStatementVisitor::determineLoopHeader(SyReCParser::ForStatementContext* context, bool& openedNewSymbolTableScopeForLoopVariable) {
    const auto loopVariableIdent                                 = tryVisitAndConvertProductionReturnValue<std::string>(context->loopVariableDefinition());
    bool       loopHeaderValid                                   = true;

    if (loopVariableIdent.has_value()) {
        if (sharedData->performingReadOnlyParsingOfLoopBody) {
            if (sharedData->currentSymbolTableScope->contains(*loopVariableIdent)) {
                createError(mapAntlrTokenPosition(context->loopVariableDefinition()->variableIdent), fmt::format(DuplicateDeclarationOfIdent, *loopVariableIdent));
                loopHeaderValid = false;
            } else {
                SymbolTable::openScope(sharedData->currentSymbolTableScope);
                openedNewSymbolTableScopeForLoopVariable          = true;
                // TODO: Since we are currently assuming that the value of a signal can be stored in at most 32 bits, we assume the latter as the bit width of the loop variable value
                sharedData->currentSymbolTableScope->addEntry(std::make_shared<syrec::Number>(*loopVariableIdent), 32, std::nullopt);
                sharedData->lastDeclaredLoopVariable.emplace(*loopVariableIdent);
            }
        } else {
            if (!sharedData->currentSymbolTableScope->contains(*loopVariableIdent)) {
                SymbolTable::openScope(sharedData->currentSymbolTableScope);
                openedNewSymbolTableScopeForLoopVariable = true;
                sharedData->currentSymbolTableScope->addEntry(std::make_shared<syrec::Number>(*loopVariableIdent), 32, std::nullopt);
            }
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

    /*
     * If the either the start and end or the end value are defined as expressions, we have to be more permissible regarding errors specific to the step size being zero (if it was specified or evaluated to zero)
     * since the effect of the different optimizations would be to cumbersome to check, i.e. simply checking if the start and end value expression are syntactically equivalent
     * is not correct in all cases when the reassociate expression or constant propagation optimization is enabled since subexpression could be dropped by these optimizations.
     */
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
                } else if (*stepSizeEvaluated != 0) {
                    numIterations = (negativeStepSizeDefined ? (*iterationRangeStartValueEvaluated - *iterationRangeEndValueEvaluated) : (*iterationRangeEndValueEvaluated - *iterationRangeStartValueEvaluated)) + 1;
                    numIterations /= *stepSizeEvaluated;
                }
                else if (*stepSizeEvaluated == 0) {
                    if (*iterationRangeStartValueEvaluated != *iterationRangeEndValueEvaluated) {
                        createError(mapAntlrTokenPosition(context->loopStepsizeDefinition()->number()->start), fmt::format(InvalidLoopIterationRangeStepSizeCannotBeZeroWhenStartAndEndValueDiffer, *iterationRangeStartValueEvaluated, *iterationRangeEndValueEvaluated));
                        loopHeaderValid = false;
                    }
                }
            }
        }
    }
    if (!loopHeaderValid) {
        return std::nullopt;
    }

    if (!wasStartValueExplicitlyDefined) {
        rangeStartValue = rangeEndValue;
    }

    return std::make_optional(ParsedLoopHeaderInformation({loopVariableIdent, std::make_pair(*rangeStartValue, *rangeEndValue), *stepSize}));
}

std::optional<SyReCStatementVisitor::ParsedLoopHeaderInformation> SyReCStatementVisitor::determineLoopHeaderFromPredefinedOne(bool& openedNewSymbolTableScopeForLoopVariable) {
    if (!preDeterminedLoopHeaderInformation.has_value()) {
        return std::nullopt;
    }

    const auto& predefinedLoopHeader = *preDeterminedLoopHeaderInformation;
    const auto  loopValueRangeStart  = std::make_shared<syrec::Number>(predefinedLoopHeader.newLoopStartValue);
    const auto  loopValueRangeEnd    = std::make_shared<syrec::Number>(predefinedLoopHeader.loopEndValue);
    const auto  loopValueRange       = std::make_pair(loopValueRangeStart, loopValueRangeEnd);
    const auto  loopStepsize         = std::make_shared<syrec::Number>(predefinedLoopHeader.newLoopStepsize);

    if (predefinedLoopHeader.loopVariableIdent.has_value()) {
        sharedData->lastDeclaredLoopVariable = *predefinedLoopHeader.loopVariableIdent;
        if (!sharedData->currentSymbolTableScope->contains(*predefinedLoopHeader.loopVariableIdent)) {
            SymbolTable::openScope(sharedData->currentSymbolTableScope);
            sharedData->currentSymbolTableScope->addEntry(std::make_shared<syrec::Number>(*predefinedLoopHeader.loopVariableIdent), 32, std::nullopt);
            openedNewSymbolTableScopeForLoopVariable = true;
        }
    }
    const ParsedLoopHeaderInformation loopHeader({predefinedLoopHeader.loopVariableIdent, loopValueRange, loopStepsize});
    preDeterminedLoopHeaderInformation.reset();

    

    return std::make_optional(loopHeader);
}

std::optional<SyReCStatementVisitor::LoopIterationRange> SyReCStatementVisitor::tryDetermineCompileTimeLoopIterationRange(const ParsedLoopHeaderInformation& loopHeader) {
    const auto& potentialErrorPosition       = determineContextStartTokenPositionOrUseDefaultOne(nullptr);
    const auto& iterationRangeStartEvaluated = canEvaluateNumber(loopHeader.range.first) ? tryEvaluateNumber(loopHeader.range.first, potentialErrorPosition) : std::nullopt;
    const auto& iterationRangeEndEvaluated   = canEvaluateNumber(loopHeader.range.second) ? tryEvaluateNumber(loopHeader.range.second, potentialErrorPosition) : std::nullopt;
    const auto& stepSizeEvaluated            = canEvaluateNumber(loopHeader.stepSize) ? tryEvaluateNumber(loopHeader.stepSize, potentialErrorPosition) : std::nullopt;

    if (!iterationRangeStartEvaluated.has_value() || !iterationRangeEndEvaluated.has_value() || !stepSizeEvaluated.has_value()) {
        return std::nullopt;
    }
    return std::make_optional(LoopIterationRange({*iterationRangeStartEvaluated, *iterationRangeEndEvaluated, *stepSizeEvaluated}));
}

// TODO: Evaluation of number without creation of error ?
bool SyReCStatementVisitor::doesLoopOnlyPerformOneIteration(const ParsedLoopHeaderInformation& loopHeader) {
    if (const auto& compileTimeIterationRangeValues = tryDetermineCompileTimeLoopIterationRange(loopHeader); compileTimeIterationRangeValues.has_value()) {
        return determineNumberOfLoopIterations(*compileTimeIterationRangeValues) == 1;
    }
    return false;
}

std::size_t SyReCStatementVisitor::determineNumberOfLoopIterations(const LoopIterationRange& loopIterationRange) {
    if (loopIterationRange.stepSize == 0) {
        return 0;
    }

    return *utils::determineNumberOfLoopIterations(static_cast<unsigned int>(loopIterationRange.startValue), static_cast<unsigned int>(loopIterationRange.endValue), static_cast<unsigned int>(loopIterationRange.stepSize));
}

SyReCStatementVisitor::CustomLoopContextInformation SyReCStatementVisitor::buildCustomLoopContextInformation(SyReCParser::ForStatementContext* loopStatement) {
    std::vector<CustomLoopContextInformation> childLoops;
    for (const auto& stmtContext : loopStatement->statementList()->stmts) {
        if (const auto& stmtContextAsLoopContext = dynamic_cast<SyReCParser::ForStatementContext*>(stmtContext); stmtContextAsLoopContext != nullptr) {
            childLoops.emplace_back(buildCustomLoopContextInformation(stmtContextAsLoopContext));
        }
    }
    return CustomLoopContextInformation({loopStatement, childLoops});
}

std::optional<syrec::Statement::vec> SyReCStatementVisitor::determineLoopBodyWithSideEffectsDisabled(SyReCParser::StatementListContext* loopBodyStmtsContext) {
    const bool backupOfReadonlyParsingOfLoops                     = sharedData->performingReadOnlyParsingOfLoopBody;
    const bool backupOfConstantPropagationOptimizationFlag        = sharedData->parserConfig->performConstantPropagation;
    const bool backupOfNoAdditionalLineOptimizationFlag           = sharedData->parserConfig->noAdditionalLineOptimizationEnabled;
    const bool backupOfModificationsOfReferenceCountFlag          = sharedData->modificationsOfReferenceCountsDisabled;
    const bool backofReassociateExpressionsFlag                   = sharedData->parserConfig->reassociateExpressionsEnabled;
    sharedData->parserConfig->performConstantPropagation          = false;
    sharedData->parserConfig->noAdditionalLineOptimizationEnabled = false;
    sharedData->modificationsOfReferenceCountsDisabled            = true;
    sharedData->parserConfig->reassociateExpressionsEnabled       = false;
    sharedData->performingReadOnlyParsingOfLoopBody               = true;
    const auto& loopBodyStmts                                     = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(loopBodyStmtsContext);
    sharedData->parserConfig->performConstantPropagation          = backupOfConstantPropagationOptimizationFlag;
    sharedData->parserConfig->noAdditionalLineOptimizationEnabled = backupOfNoAdditionalLineOptimizationFlag;
    sharedData->modificationsOfReferenceCountsDisabled            = backupOfModificationsOfReferenceCountFlag;
    sharedData->parserConfig->reassociateExpressionsEnabled       = backofReassociateExpressionsFlag;
    sharedData->performingReadOnlyParsingOfLoopBody               = backupOfReadonlyParsingOfLoops;
    return loopBodyStmts;
}

bool SyReCStatementVisitor::isValuePropagationBlockedDueToLoopDataFlowAnalysis(const syrec::VariableAccess::ptr& accessedPartsOfSignalToBeUpdated) const {
    if (!sharedData->loopBodyValuePropagationBlockers.empty()) {
        const auto& blockedUpdatesToDueLoopDataFlowAnalysis = sharedData->loopBodyValuePropagationBlockers.top();
        return blockedUpdatesToDueLoopDataFlowAnalysis->isAccessBlockedFor(accessedPartsOfSignalToBeUpdated);
    }
    return false;
}

std::optional<syrec::ForStatement::ptr> SyReCStatementVisitor::visitForStatementInReadonlyMode(SyReCParser::ForStatementContext* context) {
    sharedData->loopNestingLevel++;

    bool        needToCloseOpenedSymbolTableScopeForLoopVariable       = false;
    const auto& parsedLoopHeader                                       = preDeterminedLoopHeaderInformation.has_value() ? determineLoopHeaderFromPredefinedOne(needToCloseOpenedSymbolTableScopeForLoopVariable) : determineLoopHeader(context, needToCloseOpenedSymbolTableScopeForLoopVariable);
    const auto& loopVariableIdent                                      = parsedLoopHeader.has_value() ? parsedLoopHeader->loopVariableIdent : std::nullopt;
    const bool  loopHeaderValid                                        = parsedLoopHeader.has_value();

    if (parsedLoopHeader.has_value() && parsedLoopHeader->loopVariableIdent.has_value()) {
        sharedData->lastDeclaredLoopVariable.reset();
        addOrUpdateLoopVariableEntryAndOptionallyMakeItsValueAvailableForEvaluations(*loopVariableIdent, std::nullopt, needToCloseOpenedSymbolTableScopeForLoopVariable);
    }
    
    const auto loopBody        = determineLoopBodyWithSideEffectsDisabled(context->statementList());
    const bool isValidLoopBody = loopBody.has_value() && !(*loopBody).empty();

    if (!isValidLoopBody || !loopHeaderValid) {
        return std::nullopt;
    }

    // TODO: Instead of opening and closing a new scope simply insert and remove the entry from the symbol table
    if (needToCloseOpenedSymbolTableScopeForLoopVariable) {
        removeLoopVariableAndMakeItsValueUnavailableForEvaluations(*loopVariableIdent, needToCloseOpenedSymbolTableScopeForLoopVariable);
    }

    const auto loopStatement    = std::make_shared<syrec::ForStatement>();
    loopStatement->loopVariable = loopVariableIdent.has_value() ? *loopVariableIdent : "";
    loopStatement->range        = parsedLoopHeader->range;
    loopStatement->step         = parsedLoopHeader->stepSize;
    loopStatement->statements   = *loopBody;

    if (sharedData->loopNestingLevel > 0) {
        sharedData->loopNestingLevel--;
    }

    return std::make_optional(loopStatement);
}

void SyReCStatementVisitor::visitForStatementWithOptimizationsEnabled(SyReCParser::ForStatementContext* context) {
    sharedData->loopNestingLevel++;

    bool        needToCloseOpenedSymbolTableScopeForLoopVariable = false;
    const auto& parsedLoopHeader                                 = preDeterminedLoopHeaderInformation.has_value() ? determineLoopHeaderFromPredefinedOne(needToCloseOpenedSymbolTableScopeForLoopVariable) : determineLoopHeader(context, needToCloseOpenedSymbolTableScopeForLoopVariable);
    const auto& loopVariableIdent                                = parsedLoopHeader.has_value() ? parsedLoopHeader->loopVariableIdent : std::nullopt;
    const bool  loopHeaderValid                                  = parsedLoopHeader.has_value();

    const bool doesLoopOnlyPerformOneIterationInTotal = loopHeaderValid && doesLoopOnlyPerformOneIteration(*parsedLoopHeader);
    const auto& valueOfLoopVariableInCurrentIteration = loopHeaderValid ? tryEvaluateNumber(parsedLoopHeader->range.first, determineContextStartTokenPositionOrUseDefaultOne(nullptr)) : std::nullopt;

    if (loopHeaderValid) {
        if (const auto& determinedIterationRange = tryDetermineCompileTimeLoopIterationRange(*parsedLoopHeader); determinedIterationRange.has_value()) {
            const auto numberOfIterationsOfLoop = determineNumberOfLoopIterations(*determinedIterationRange);
            if (numberOfIterationsOfLoop == 1 
                && sharedData->parserConfig->deadCodeEliminationEnabled 
                && (sharedData->parserConfig->optionalLoopUnrollConfig.has_value() ? sharedData->loopNestingLevel <= sharedData->parserConfig->optionalLoopUnrollConfig->maxAllowedNestingLevelOfInnerLoops : true) 
                && loopVariableIdent.has_value()
                && valueOfLoopVariableInCurrentIteration.has_value()) {
                addOrUpdateLoopVariableEntryAndOptionallyMakeItsValueAvailableForEvaluations(*loopVariableIdent, valueOfLoopVariableInCurrentIteration, needToCloseOpenedSymbolTableScopeForLoopVariable);
            }
        }    
    }

    std::optional<syrec::Statement::vec> loopBody;
    const auto&                          inheritedValuePropagationBlockersFromParentLoopDataFlowAnalysis = !sharedData->loopBodyValuePropagationBlockers.empty()
    ? std::make_optional(sharedData->loopBodyValuePropagationBlockers.top())
    : std::nullopt;

    /*
     * If we can determine that the loop will only perform one loop iteration we can skip the data flow analysis of its loop body
     * but still need to take into account restrictions that stem from a prior data flow analysis of any parent loop
     */
    const auto& stmtsOfLoopBodyToPerformDataFlowAnalysisOn = !doesLoopOnlyPerformOneIterationInTotal
    ? determineLoopBodyWithSideEffectsDisabled(context->statementList())
    : std::make_optional(syrec::Statement::vec());

    if (stmtsOfLoopBodyToPerformDataFlowAnalysisOn.has_value()) {
        // Avoid unnecessary work and only make result of data flow analysis regarding assigned to signals in loop body available if any assignments are performed
        if (const auto additionalValuePropagationRestrictionsDueToDataFlowAnalysis = std::make_shared<optimizations::LoopBodyValuePropagationBlocker>(*stmtsOfLoopBodyToPerformDataFlowAnalysisOn, sharedData->currentSymbolTableScope, inheritedValuePropagationBlockersFromParentLoopDataFlowAnalysis); additionalValuePropagationRestrictionsDueToDataFlowAnalysis->areAnyAssignmentsPerformed()) {
            sharedData->loopBodyValuePropagationBlockers.push(additionalValuePropagationRestrictionsDueToDataFlowAnalysis);
            loopBody = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(context->statementList());
            sharedData->loopBodyValuePropagationBlockers.pop();      
        }
        else {
            loopBody = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(context->statementList());    
        }
    }

    const bool isValidLoopBody = loopBody.has_value() && !(*loopBody).empty();
    if (!isValidLoopBody || !loopHeaderValid) {
        return;
    }

    if (doesLoopOnlyPerformOneIterationInTotal) {
        for (const auto& stmt : *loopBody) {
            addStatementToOpenContainer(stmt);
        }
        return;
    }

    const auto loopStatement    = std::make_shared<syrec::ForStatement>();
    loopStatement->loopVariable = loopVariableIdent.has_value() ? *loopVariableIdent : "";
    loopStatement->range        = parsedLoopHeader->range;
    loopStatement->step         = parsedLoopHeader->stepSize;
    loopStatement->statements   = *loopBody;

    
    auto unrollConfigToUse = sharedData->parserConfig->optionalLoopUnrollConfig;
    /*
         * If we could determine that a loop will only perform one iteration and if the dead code elimination optimization is enabled
         * we will create a loop unroll configuration that will perform the unroll in case that no user provided one is given.
         */
    if (!sharedData->parserConfig->optionalLoopUnrollConfig.has_value() && sharedData->parserConfig->deadCodeEliminationEnabled && doesLoopOnlyPerformOneIterationInTotal) {
        unrollConfigToUse.emplace(optimizations::LoopOptimizationConfig(1, 0, UINT_MAX, false, false));
    }

    /*
     * If we are parsing an already predetermined loop header (generated during an unroll of a loop with a remainder), we perform no unrolling
     */

    if (preDeterminedLoopHeaderInformation.has_value() || !unrollConfigToUse.has_value()) {
        addStatementToOpenContainer(loopStatement);
        return;
    }
    unrollAndProcessLoopBody(context, loopStatement, *unrollConfigToUse);

    // TODO: Instead of opening and closing a new scope simply insert and remove the entry from the symbol table
    if (needToCloseOpenedSymbolTableScopeForLoopVariable) {
        removeLoopVariableAndMakeItsValueUnavailableForEvaluations(*loopVariableIdent, needToCloseOpenedSymbolTableScopeForLoopVariable);   
    }

    if (sharedData->loopNestingLevel > 0) {
        sharedData->loopNestingLevel--;
    }
}

void SyReCStatementVisitor::unrollAndProcessLoopBody(SyReCParser::ForStatementContext* context, const std::shared_ptr<syrec::ForStatement>& loopStatement, const optimizations::LoopOptimizationConfig& loopUnrollConfigToUse) {
    const auto  loopUnrollerInstance    = std::make_unique<optimizations::LoopUnroller>(sharedData->loopVariableMappingLookup);
    const auto& unrolledLoopInformation = loopUnrollerInstance->tryUnrollLoop(loopUnrollConfigToUse, loopStatement);

    if (std::holds_alternative<optimizations::LoopUnroller::NotModifiedLoopInformation>(unrolledLoopInformation.data)) {
        addStatementToOpenContainer(loopStatement);
        return;
    }

    /*
     * We cannot perform an unroll of a loop with a defined variable if the constant propagation optimization is turn off because this would not fixup the value of the loop variable in the unrolled statements
     * i.e. for $i = 0 to 4 step 1 do c[$i] += ... rof would be unrolled to c[$i] += ... with the definition of the loop variable $i being removed since the loop was unrolled completely
     */
    if (!std::holds_alternative<optimizations::LoopUnroller::NotModifiedLoopInformation>(unrolledLoopInformation.data) && !loopStatement->loopVariable.empty()
        && !sharedData->parserConfig->performConstantPropagation) {
        addStatementToOpenContainer(loopStatement);
        return;
    }

    const auto&                                customLoopContextInformation = buildCustomLoopContextInformation(context);
    std::vector<ModifiedLoopHeaderInformation> modifiedLoopHeaderInformation;
    std::vector<UnrolledLoopVariableValue>     unrolledLoopVariableValueLookup;
    std::vector<antlr4::ParserRuleContext*>    transformedLoopContext;
    std::size_t                                stmtOffsetFromRootLoop = 0;
    SyReCStatementVisitor::buildParseRuleInformationFromUnrolledLoops(
            transformedLoopContext,
            stmtOffsetFromRootLoop,
            context,
            customLoopContextInformation,
            unrolledLoopVariableValueLookup,
            modifiedLoopHeaderInformation,
            unrolledLoopInformation);

    auto loopVariableValueChange = unrolledLoopVariableValueLookup.begin();
    auto modifiedLoopHeader      = modifiedLoopHeaderInformation.begin();
    bool openedNewSymbolTableScopeForLoopVariable = false;
    
    for (std::size_t unrolledStmtOffsetFromRootLoop = 0; unrolledStmtOffsetFromRootLoop < transformedLoopContext.size(); ++unrolledStmtOffsetFromRootLoop) {
        if (loopVariableValueChange != unrolledLoopVariableValueLookup.end()) {
            if (loopVariableValueChange->activateAtLineRelativeToParent == unrolledStmtOffsetFromRootLoop) {
                addOrUpdateLoopVariableEntryAndOptionallyMakeItsValueAvailableForEvaluations(loopVariableValueChange->loopVariableIdent, std::make_optional(loopVariableValueChange->value), openedNewSymbolTableScopeForLoopVariable);
                ++loopVariableValueChange;
            }
        }

        if (modifiedLoopHeader != modifiedLoopHeaderInformation.end() && unrolledStmtOffsetFromRootLoop == modifiedLoopHeader->activateAtLineRelativeToParent) {
            if (modifiedLoopHeader->loopVariableIdent.has_value()) {
                removeLoopVariableAndMakeItsValueUnavailableForEvaluations(*modifiedLoopHeader->loopVariableIdent, openedNewSymbolTableScopeForLoopVariable);    
            }
            
            /*if (modifiedLoopHeader->loopVariableIdent.has_value() && sharedData->currentSymbolTableScope->contains(*modifiedLoopHeader->loopVariableIdent)) {
                sharedData->currentSymbolTableScope->removeVariable(*modifiedLoopHeader->loopVariableIdent);
            }*/
            preDeterminedLoopHeaderInformation.emplace(
                    ModifiedLoopHeaderInformation(
                            {modifiedLoopHeader->activateAtLineRelativeToParent,
                             modifiedLoopHeader->loopVariableIdent,
                             modifiedLoopHeader->newLoopStartValue,
                             modifiedLoopHeader->loopEndValue,
                             modifiedLoopHeader->newLoopStepsize}));
            ++modifiedLoopHeader;
        }


        const auto& parserContextOfStmtToVisit = transformedLoopContext.at(unrolledStmtOffsetFromRootLoop);
        //std::optional<syrec::Statement::ptr> unrolledStmt;
        if (const auto generalParserContextAsContextForLoopStmt = dynamic_cast<SyReCParser::ForStatementContext*>(parserContextOfStmtToVisit); generalParserContextAsContextForLoopStmt != nullptr) {
            visitForStatementWithOptimizationsEnabled(generalParserContextAsContextForLoopStmt);
        }
        else {
            const auto unrolledStmt = tryVisitAndConvertProductionReturnValue<syrec::Statement::ptr>(transformedLoopContext.at(unrolledStmtOffsetFromRootLoop));
            if (!unrolledStmt.has_value()) {
                continue;
            }
            addStatementToOpenContainer(*unrolledStmt);
        }
    }
}

void SyReCStatementVisitor::addOrUpdateLoopVariableEntryAndOptionallyMakeItsValueAvailableForEvaluations(const std::string& loopVariableIdent, const std::optional<unsigned>& valueOfLoopVariable, bool& wasNewSymbolTableScopeOpened) const {
    if (!sharedData->currentSymbolTableScope->contains(loopVariableIdent)) {
        SymbolTable::openScope(sharedData->currentSymbolTableScope);
        wasNewSymbolTableScopeOpened = true;

        if (valueOfLoopVariable.has_value()) {
            sharedData->currentSymbolTableScope->addEntry(std::make_shared<syrec::Number>(loopVariableIdent), BitHelpers::getRequiredBitsToStoreValue(*valueOfLoopVariable), valueOfLoopVariable);   
        }
        else {
            sharedData->currentSymbolTableScope->addEntry(std::make_shared<syrec::Number>(loopVariableIdent), UINT_MAX, valueOfLoopVariable);
        }
    } else {
        if (valueOfLoopVariable.has_value()) {
            sharedData->currentSymbolTableScope->updateStoredValueForLoopVariable(loopVariableIdent, *valueOfLoopVariable);   
        }
        else {
            sharedData->currentSymbolTableScope->invalidateStoredValueForLoopVariable(loopVariableIdent);
        }
    }

    if (sharedData->loopVariableMappingLookup.count(loopVariableIdent) != 0) {
        if (valueOfLoopVariable.has_value()) {
            sharedData->loopVariableMappingLookup[loopVariableIdent] = *valueOfLoopVariable;
        }
        else {
            sharedData->loopVariableMappingLookup.erase(loopVariableIdent);
        }
    }
    else if (valueOfLoopVariable.has_value()){
        sharedData->loopVariableMappingLookup.insert(std::make_pair(loopVariableIdent, *valueOfLoopVariable));
    }
}

void SyReCStatementVisitor::removeLoopVariableAndMakeItsValueUnavailableForEvaluations(const std::string& loopVariableIdent, bool wasNewSymbolTableScopeOpenedForLoopVariable) const {
    if (wasNewSymbolTableScopeOpenedForLoopVariable) {
        SymbolTable::closeScope(sharedData->currentSymbolTableScope);
    }

    if (sharedData->loopVariableMappingLookup.count(loopVariableIdent) != 0) {
        sharedData->loopVariableMappingLookup.erase(loopVariableIdent);
    }
}

bool SyReCStatementVisitor::doesModuleOnlyConsistOfSkipStatements(const syrec::Module::ptr& calledModule) {
    /*
     * Since we have no other way than this 'hack' to differentiate whether a given Statement struct has the specific subtype 'SkipStatement' that is not
     * define as a variant of the statement struct but rather as a type alias of the base statement struct, we have to use the typeid at runtime for
     * this distinction (we cannot use the std::dynamic_pointer_cast function to perform this check since the sidecast between a subtype and the base type does always work).
     *
     */
    const auto& typeIdOfSkipStatement = typeid(std::make_shared<syrec::SkipStatement>());
    return std::all_of(
    calledModule->statements.cbegin(),
    calledModule->statements.cend(),
    [&typeIdOfSkipStatement](const syrec::Statement::ptr& statement) {
        return typeid(statement.get()) == typeIdOfSkipStatement;
    });
}

bool SyReCStatementVisitor::doesModuleOnlyHaveReadOnlyParameters(const syrec::Module::ptr& calledModule) {
    return std::all_of(
    calledModule->parameters.cbegin(),
    calledModule->parameters.cend(),
    [](const syrec::Variable::ptr& parameter) {
        return parameter->type == syrec::Variable::In;
    });
}

void SyReCStatementVisitor::decrementReferenceCountsOfCalledModuleAndActuallyUsedCalleeArguments(const syrec::Module::ptr& calledModule, const std::vector<std::string>& filteredCalleeArguments) const {
    for (const auto& actuallyUsedCalleeArgument: filteredCalleeArguments) {
        decrementReferenceCountOfSignal(actuallyUsedCalleeArgument);
    }
    sharedData->currentSymbolTableScope->decrementReferenceCountOfModulesMatchingSignature(calledModule);
}

syrec::AssignStatement::vec SyReCStatementVisitor::trySimplifyAssignmentStatement(const syrec::AssignStatement::ptr& assignmentStmt) const {
    // TODO: Remove, only used for testing
    // TODO: Bitwidth is not set for simplification of binary expression for example circuit bn_2
    //return {assignmentStmt};

    const auto& assignmentStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    if (assignmentStmtCasted == nullptr) {
        return {assignmentStmt};
    }
    const auto definedAssignmentOperation = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignmentStmtCasted->op);
    if (!definedAssignmentOperation.has_value()) {
        return {assignmentStmt};
    }

    auto assignmentStmtRhs = assignmentStmtCasted->rhs;
    performAssignmentRhsExprSimplification(assignmentStmtRhs);

    if (const auto& assignmentStmtRhsAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(assignmentStmtRhs); 
        assignmentStmtRhsAsNumericExpr != nullptr && (sharedData->parserConfig->deadCodeEliminationEnabled || sharedData->parserConfig->deadStoreEliminationEnabled)) {
        const auto& optionalConstantValueOfRhs = assignmentStmtRhsAsNumericExpr->value->isConstant()
            ? std::make_optional(assignmentStmtRhsAsNumericExpr->value->evaluate(sharedData->loopVariableMappingLookup))
            : std::nullopt;

        if (optionalConstantValueOfRhs.has_value() && syrec_operation::isOperandUseAsRhsInOperationIdentityElement(*definedAssignmentOperation, *optionalConstantValueOfRhs)) {
            return {};
        }
    }

    const auto& transformedAssignmentStmt = assignmentStmtRhs == assignmentStmtCasted->rhs
        ? assignmentStmt
        : std::make_shared<syrec::AssignStatement>(assignmentStmtCasted->lhs, assignmentStmtCasted->op, assignmentStmtRhs);
    
     if (sharedData->parserConfig->noAdditionalLineOptimizationEnabled) {
        const auto& noAdditionalLineAssignmentSimplifier = std::make_unique<noAdditionalLineSynthesis::MainAdditionalLineForAssignmentSimplifier>(sharedData->currentSymbolTableScope, nullptr, nullptr);
        if (const auto& generatedSimplifierAssignments = noAdditionalLineAssignmentSimplifier->tryReduceRequiredAdditionalLinesFor(transformedAssignmentStmt); !generatedSimplifierAssignments.empty()) {
            return generatedSimplifierAssignments;
        }
     }
     return {transformedAssignmentStmt};
}

/*
 * TODO: Behaviour during loop unrolling
 *
 * When the readonly parse step of the loop body takes place, the assignment will probably invalidate the stored value of the signal
 * Is the value, if available prior to the loop unroll, preserved for the non-readonly step ?
 */
void SyReCStatementVisitor::performAssignmentOperation(const syrec::AssignStatement::ptr& assignmentStmt) const {
    const auto& assignmentStmtCasted = std::dynamic_pointer_cast<syrec::AssignStatement>(assignmentStmt);
    const auto  definedAssignmentOperation = syrec_operation::tryMapAssignmentOperationFlagToEnum(assignmentStmtCasted->op);
    if (assignmentStmtCasted == nullptr) {
        return;
    }

    if (isValuePropagationBlockedDueToLoopDataFlowAnalysis(assignmentStmtCasted->lhs) || !definedAssignmentOperation.has_value()) {
        invalidateStoredValueFor(assignmentStmtCasted->lhs);
    } else {
        // We need to update the currently stored value for the assigned to signal if the rhs expression evaluates to a constant, otherwise invalidate the stored value for the former
        tryUpdateOrInvalidateStoredValueFor(assignmentStmtCasted->lhs, tryDetermineNewSignalValueFromAssignment(assignmentStmtCasted->lhs, *definedAssignmentOperation, assignmentStmtCasted->rhs));
    }
}

void SyReCStatementVisitor::performAssignmentRhsExprSimplification(syrec::expression::ptr& assignmentRhsExpr) const {
    if (std::dynamic_pointer_cast<syrec::BinaryExpression>(assignmentRhsExpr) != nullptr || std::dynamic_pointer_cast<syrec::ShiftExpression>(assignmentRhsExpr) != nullptr) {
        if (sharedData->parserConfig->reassociateExpressionsEnabled) {
            if (const auto& reassociateExpressionOptimizationResult = optimizations::simplifyBinaryExpression(assignmentRhsExpr, sharedData->parserConfig->operationStrengthReductionEnabled, sharedData->optionalMultiplicationSimplifier); reassociateExpressionOptimizationResult != assignmentRhsExpr) {
                assignmentRhsExpr = reassociateExpressionOptimizationResult;
            }
        } else if (const auto& defaultBinaryExpressionOptimizationResult = optimizations::trySimplify(assignmentRhsExpr, sharedData->parserConfig->operationStrengthReductionEnabled); defaultBinaryExpressionOptimizationResult.couldSimplify) {
            assignmentRhsExpr = defaultBinaryExpressionOptimizationResult.simplifiedExpression;
        }
    }
}
