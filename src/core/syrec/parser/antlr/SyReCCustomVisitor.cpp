#include <cstdint>
#include <optional>
#include <string>
#include <fmt/format.h>

#include "SyReCCustomVisitor.h"

#include "core/syrec/parser/custom_semantic_errors.hpp"
#include "core/syrec/parser/expression_comparer.hpp"
#include "core/syrec/parser/expression_evaluation_result.hpp"
#include "core/syrec/parser/infix_iterator.hpp"
#include "core/syrec/parser/module_call_guess.hpp"
#include "core/syrec/parser/parser_utilities.hpp"
#include "core/syrec/parser/range_check.hpp"

using namespace parser;

// https://stackoverflow.com/questions/60420005/programmatically-access-antlr-grammar-rule-string-literals-outside-of-a-parse-co

// TODO: Currently loop variables are not supported as arguments
// TODO: Currently the parser is not able to correctly parse assign statements such as <a> + = (...) since the operand will be split into two tokens and the assign statement production is not "found" during the parse tree creation
// TODO: Refactor passing of position where error should be created in createError() and semantic checks (i.e. pass exact position as line, col and potentially an ident instead of just passing in the token)

std::any SyReCCustomVisitor::visitProgram(SyReCParser::ProgramContext* context) {
    SymbolTable::openScope(currentSymbolTableScope);
    for (const auto& module : context->module()) {
        const auto moduleParseResult = tryVisitAndConvertProductionReturnValue<syrec::Module::ptr>(module);
        if (moduleParseResult.has_value()) {
            if (currentSymbolTableScope->contains(*moduleParseResult)) {
                createErrorAtTokenPosition(module->IDENT()->getSymbol(), fmt::format(DuplicateModuleIdentDeclaration, (*moduleParseResult)->name));   
            }
            else {
                currentSymbolTableScope->addEntry(*moduleParseResult);
                modules.emplace_back(moduleParseResult.value());                
            }
        }
    }

    return errors.empty();
}

std::any SyReCCustomVisitor::visitModule(SyReCParser::ModuleContext* context) {
    SymbolTable::openScope(this->currentSymbolTableScope);
    const std::string moduleIdent = context->IDENT()->getText();
    syrec::Module::ptr                  module      = std::make_shared<syrec::Module>(moduleIdent);

    const auto declaredParameters = tryVisitAndConvertProductionReturnValue<syrec::Variable::vec>(context->parameterList());
    bool                                isDeclaredModuleValid = context->parameterList() != nullptr ? declaredParameters.has_value() : true;
    
    if (isDeclaredModuleValid && declaredParameters.has_value()) {
        for (const auto& declaredParameter : declaredParameters.value()) {
            module->addParameter(declaredParameter);
        }
    }

    for (const auto& declaredSignals: context->signalList()) {
        const auto parsedDeclaredSignals = tryVisitAndConvertProductionReturnValue<syrec::Variable::vec>(declaredSignals);
        isDeclaredModuleValid &= parsedDeclaredSignals.has_value();
        if (isDeclaredModuleValid) {
            for (const auto& declaredSignal : *parsedDeclaredSignals) {
                module->addVariable(declaredSignal);   
            }
        }
    } 

    const auto validUserDefinedModuleStatements = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(context->statementList());
    if (validUserDefinedModuleStatements.has_value() && !(*validUserDefinedModuleStatements).empty()) {
        module->statements = *validUserDefinedModuleStatements;
    }
    else {
        isDeclaredModuleValid = false;
    }
    
    SymbolTable::closeScope(this->currentSymbolTableScope);
    return isDeclaredModuleValid ? std::make_optional(module) : std::nullopt;
}

std::any SyReCCustomVisitor::visitParameterList(SyReCParser::ParameterListContext* context) {
    bool                                             allParametersValid = true;
    syrec::Variable::vec                             parametersContainer;
    std::optional<syrec::Variable::vec>              parameters;

    for (const auto& parameter : context->parameter()) {
        const auto parsedParameterDefinition = tryVisitAndConvertProductionReturnValue<syrec::Variable::ptr>(parameter);
        allParametersValid &= parsedParameterDefinition.has_value();
        if (allParametersValid) {
            parametersContainer.emplace_back(*parsedParameterDefinition);
        }
    }

    if (allParametersValid) {
        parameters.emplace(parametersContainer);
    }

    return parameters;
}

std::any SyReCCustomVisitor::visitParameter(SyReCParser::ParameterContext* context) {
     const auto parameterType = (getParameterType(context->start));
    if (!parameterType.has_value()) {
        createErrorAtTokenPosition(context->start, InvalidParameterType);
    } 

    auto declaredParameter = tryVisitAndConvertProductionReturnValue<syrec::Variable::ptr>(context->signalDeclaration());
    if (!parameterType.has_value() || !declaredParameter.has_value()) {
        return std::nullopt;
    }

    (*declaredParameter)->type = *parameterType;
    currentSymbolTableScope->addEntry(*declaredParameter);
    return declaredParameter;
}

std::any SyReCCustomVisitor::visitSignalList(SyReCParser::SignalListContext* context) {

    // TODO: Check if tryConvert... can be replace with tryVisit...
    const auto declaredSignalsType = getSignalType(context->start);
    bool       isValidSignalListDeclaration = declaredSignalsType.has_value();

    syrec::Variable::vec declaredSignalsOfType{};
    for (const auto& signal : context->signalDeclaration()) {
        const auto declaredSignal = tryVisitAndConvertProductionReturnValue<syrec::Variable::ptr>(signal);
        isValidSignalListDeclaration &= declaredSignal.has_value();

        if (isValidSignalListDeclaration) {
            (*declaredSignal)->type = *declaredSignalsType;
            declaredSignalsOfType.emplace_back(*declaredSignal);
            currentSymbolTableScope->addEntry(*declaredSignal);
        }
    }
    
    return std::make_optional(declaredSignalsOfType);
}

std::any SyReCCustomVisitor::visitSignalDeclaration(SyReCParser::SignalDeclarationContext* context) {
    std::optional<syrec::Variable::ptr> signal;
    std::vector<unsigned int> dimensions {};
    unsigned int              signalWidth = this->config->cDefaultSignalWidth;
    bool              isValidSignalDeclaration = true;

    const std::string signalIdent = context->IDENT()->getText();
    if (currentSymbolTableScope->contains(signalIdent)) {
        isValidSignalDeclaration = false;
        createErrorAtTokenPosition(context->IDENT()->getSymbol(), fmt::format(DuplicateDeclarationOfIdent, signalIdent));
    }

    for (const auto& dimensionToken: context->dimensionTokens) {
        std::optional<unsigned int> dimensionTokenValueAsNumber = ParserUtilities::convertToNumber(dimensionToken);
        if (dimensionTokenValueAsNumber.has_value()) {
            dimensions.emplace_back((*dimensionTokenValueAsNumber));
        } else {
            isValidSignalDeclaration = false;
        }
    }

    if (context->signalWidthToken != nullptr) {
        const std::optional<unsigned int> customSignalWidth = ParserUtilities::convertToNumber(context->signalWidthToken);
        if (customSignalWidth.has_value()) {
            signalWidth = (*customSignalWidth);
        } else {
            isValidSignalDeclaration = false;
        }
    }

    if (isValidSignalDeclaration) {
        signal.emplace(std::make_shared<syrec::Variable>(0, signalIdent, dimensions, signalWidth));
    }
    return signal;
}

/*
 * Signal production
 */
std::any SyReCCustomVisitor::visitSignal(SyReCParser::SignalContext* context) {
    std::string signalIdent;
    std::optional<syrec::VariableAccess::ptr> accessedSignal;
    bool isValidSignalAccess = context->IDENT() != nullptr && checkIfSignalWasDeclaredOrLogError(context->IDENT()->getSymbol());

    
    if (isValidSignalAccess) {
        signalIdent = context->IDENT()->getText();
        const auto signalSymTabEntry = currentSymbolTableScope->getVariable(signalIdent);
        isValidSignalAccess          = signalSymTabEntry.has_value() && std::holds_alternative<syrec::Variable::ptr>(*signalSymTabEntry);

        if (isValidSignalAccess) {
            const syrec::VariableAccess::ptr container = std::make_shared<syrec::VariableAccess>();
            container->setVar(std::get<syrec::Variable::ptr>(*signalSymTabEntry));
            accessedSignal.emplace(container);
        }
    }

    const size_t numDimensionsOfAccessSignal = accessedSignal.has_value() ? (*accessedSignal)->getVar()->dimensions.size() : SIZE_MAX;
    const size_t numUserDefinedDimensions    = context->accessedDimensions.size();
    if (isValidSignalAccess) {
        const size_t numElementsWithinRange = std::min(numUserDefinedDimensions, numDimensionsOfAccessSignal);
        for (size_t dimensionIdx = 0; dimensionIdx < numElementsWithinRange; ++dimensionIdx) {
            const auto  accessedDimensionExpression = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->accessedDimensions.at(dimensionIdx));

            if (accessedSignal.has_value()) {
                // TODO: Error position
                isValidSignalAccess = validateSemanticChecksIfDimensionExpressionIsConstant(
                    context->accessedDimensions.at(dimensionIdx)->start,
                    dimensionIdx, 
                    (*accessedSignal)->getVar(),
                    accessedDimensionExpression
                );

                if (isValidSignalAccess) {
                    // TODO: Set correct bit width of expression
                    (*accessedSignal)->indexes.emplace_back((*accessedDimensionExpression)->getAsExpression().value());
                }
            }
        }

        isValidSignalAccess &= numUserDefinedDimensions <= numElementsWithinRange;
        for (size_t dimensionOutOfRangeIdx = numElementsWithinRange; dimensionOutOfRangeIdx < numUserDefinedDimensions; ++dimensionOutOfRangeIdx) {
            createErrorAtTokenPosition(context->accessedDimensions.at(dimensionOutOfRangeIdx)->start, fmt::format(DimensionOutOfRange, dimensionOutOfRangeIdx, signalIdent, numDimensionsOfAccessSignal));
        }
    }

    const auto bitOrRangeAccessEvaluated = isBitOrRangeAccessDefined(context->bitStart, context->bitRangeEnd);
    if (accessedSignal.has_value() && bitOrRangeAccessEvaluated.has_value()) {
        const auto  accessVariable                = (*accessedSignal)->getVar();
        const auto& bitOrRangeAccessPair = bitOrRangeAccessEvaluated.value();
        const auto& bitOrRangeAccessPairEvaluated = std::make_pair(bitOrRangeAccessPair.first->evaluate({}), bitOrRangeAccessPair.second->evaluate({}));

        if (bitOrRangeAccessPair.first == bitOrRangeAccessPair.second) {
            if (!isValidBitAccess(accessVariable, bitOrRangeAccessPairEvaluated.first, true)) {
                isValidSignalAccess                                      = false;
                const IndexAccessRangeConstraint constraintsForBitAccess = getConstraintsForValidBitAccess(accessVariable, true);
                // TODO: GEN_ERROR: Bit access out of range
                createErrorAtTokenPosition(context->bitStart->start, fmt::format(BitAccessOutOfRange, bitOrRangeAccessPairEvaluated.first, signalIdent, constraintsForBitAccess.minimumValidValue, constraintsForBitAccess.maximumValidValue));
            }
        }
        else {
            if (bitOrRangeAccessPairEvaluated.first > bitOrRangeAccessPairEvaluated.second) {
                isValidSignalAccess = false;
                // TODO: GEN_ERROR: Bit range start larger than end
                createErrorAtTokenPosition(context->bitStart->start, fmt::format(BitRangeStartLargerThanEnd, bitOrRangeAccessPairEvaluated.first, bitOrRangeAccessPairEvaluated.second));
            }
            else if (!isValidBitRangeAccess(accessVariable, bitOrRangeAccessPairEvaluated, true)) {
                isValidSignalAccess                                           = false;
                const IndexAccessRangeConstraint constraintsForBitRangeAccess = getConstraintsForValidBitAccess((*accessedSignal)->getVar(), true);
                // TODO: GEN_ERROR: Bit range out of range
                createErrorAtTokenPosition(context->bitStart->start, fmt::format(BitRangeOutOfRange, bitOrRangeAccessPairEvaluated.first, bitOrRangeAccessPairEvaluated.second, signalIdent, constraintsForBitRangeAccess.minimumValidValue, constraintsForBitRangeAccess.maximumValidValue));
            }
        }

        if (isValidSignalAccess) {
            accessedSignal.value()->range.emplace(bitOrRangeAccessPair);
        }
    }

    return isValidSignalAccess ? std::make_optional(std::make_shared<SignalEvaluationResult>(*accessedSignal)) : std::nullopt;
}

/*
 * Number production variants
 */
std::any SyReCCustomVisitor::visitNumberFromConstant(SyReCParser::NumberFromConstantContext* context) {
    if (context->INT() == nullptr) {
        return std::nullopt;   
    }

    return std::make_optional(std::make_shared<syrec::Number>(ParserUtilities::convertToNumber(context->INT()->getText()).value()));
}

std::any SyReCCustomVisitor::visitNumberFromSignalwidth(SyReCParser::NumberFromSignalwidthContext* context) {
    if (context->IDENT() == nullptr) {
        return std::nullopt;
    }
    
    const std::string                 signalIdent = context->IDENT()->getText();
    if (!checkIfSignalWasDeclaredOrLogError(context->IDENT()->getSymbol())) {
        return std::nullopt;
    }

    std::optional<syrec::Number::ptr> signalWidthOfSignal;
    const auto& symTableEntryForSignal = currentSymbolTableScope->getVariable(signalIdent);
    if (symTableEntryForSignal.has_value() && std::holds_alternative<syrec::Variable::ptr>(*symTableEntryForSignal)) {
        signalWidthOfSignal.emplace(std::make_shared<syrec::Number>(std::get<syrec::Variable::ptr>(*symTableEntryForSignal)->bitwidth));
    }
    else {
        // TODO: GEN_ERROR, this should not happen
        // TODO: Error position
        createErrorAtTokenPosition(nullptr, "TODO");
    }

    return signalWidthOfSignal;
}

std::any SyReCCustomVisitor::visitNumberFromLoopVariable(SyReCParser::NumberFromLoopVariableContext* context) {
    if (context->IDENT() == nullptr) {
        return std::nullopt;
    }

    const std::string signalIdent = "$" + context->IDENT()->getText();
    if (!checkIfSignalWasDeclaredOrLogError(context->IDENT()->getSymbol(), true)) {
        return std::nullopt;
    }

    std::optional<syrec::Number::ptr> valueOfLoopVariable;
    const auto&                       symTableEntryForSignal = currentSymbolTableScope->getVariable(signalIdent);
    if (symTableEntryForSignal.has_value() && std::holds_alternative<syrec::Number::ptr>(symTableEntryForSignal.value())) {
        valueOfLoopVariable.emplace(std::get<syrec::Number::ptr>(symTableEntryForSignal.value()));
    } else {
        // TODO: GEN_ERROR, this should not happen but check anyways
        // TODO: Error position
        createErrorAtTokenPosition(nullptr, "TODO");
    }

    return valueOfLoopVariable;
}

std::any SyReCCustomVisitor::visitNumberFromExpression(SyReCParser::NumberFromExpressionContext* context) {
    const auto lhsOperand = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->lhsOperand);

    const auto operation  = getDefinedOperation(context->op);
    if (!operation.has_value()) {
        // TODO: Error position
        createErrorAtTokenPosition(context->op, InvalidOrMissingNumberExpressionOperation);
    }

    const auto rhsOperand = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->rhsOperand);
        if (!lhsOperand.has_value() || !operation.has_value() || !rhsOperand.has_value()) {
        return std::nullopt;    
    }

    const auto lhsOperandEvaluated = tryEvaluateNumber(*lhsOperand);
    const auto rhsOperandEvaluated = tryEvaluateNumber(*rhsOperand);

    if (lhsOperandEvaluated.has_value() && rhsOperandEvaluated.has_value()) {
        const auto binaryOperationResult = applyBinaryOperation(*operation, *lhsOperandEvaluated, *rhsOperandEvaluated);
        if (binaryOperationResult.has_value()) {
            const auto resultContainer = std::make_shared<syrec::Number>(*binaryOperationResult);
            return std::make_optional<syrec::Number::ptr>(resultContainer);
        }
    }

    return std::nullopt;
}


/*
 * Expression production visitors
 */

std::any SyReCCustomVisitor::visitExpressionFromNumber(SyReCParser::ExpressionFromNumberContext* context) {
    const auto definedNumber = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->number());

    /*

    // TODO: Is this the correct calculation and is it both safe and does it return the expected result
    if (definedNumber.has_value()) {
        if ((*definedNumber)->isConstant()) {
            const auto valueOfDefinedNumber = (*definedNumber)->evaluate(loopVariableMappingLookup);
            return std::make_optional(ExpressionEvaluationResult::createFromConstantValue(valueOfDefinedNumber));
        } 

        // TODO: When the lookup of the loop variable is working correctly this branch can be replaced with the above, and also use evaluateNumber instead
        const auto expressionFromNumber = std::make_shared<syrec::NumericExpression>(*definedNumber, ExpressionEvaluationResult::getRequiredBitWidthToStoreSignal(0));
        return std::make_optional(ExpressionEvaluationResult::createFromExpression(expressionFromNumber));    
    }
    */

    /*
    if (definedNumber.has_value()) {
        const auto numberEvaluated = evaluateNumber(*definedNumber);
        if (numberEvaluated.has_value()) {
            if ((*definedNumber)->isLoopVariable()) {
                return ExpressionEvaluationResult::createFromExpression(std::make_shared<syrec::NumericExpression>(*definedNumber, ExpressionEvaluationResult::getRequiredBitWidthToStoreSignal(0)));
            }
            return std::make_optional(ExpressionEvaluationResult::createFromConstantValue(*numberEvaluated));
        }
    }
    */

    if (definedNumber.has_value()) {
        const auto valueOfDefinedNumber = tryEvaluateNumber(*definedNumber);
        if ((*definedNumber)->isLoopVariable()) {
        /*
         * TODO:
         * If no loop unrolling is done by default and we do not know the bitwidth that the expression should have from the other operand of the operation,
         * we would need a lookup for the maximum value possible and assume the bitwidth to be the one of the maximum value.
         * The easiest solution would be to pass in the expected bitwidth for this expression
         */
            const auto containerForLoopVariable = std::make_shared<syrec::NumericExpression>(*definedNumber, 1);
            return std::make_optional(ExpressionEvaluationResult::createFromExpression(containerForLoopVariable));   
        }
        return std::make_optional(ExpressionEvaluationResult::createFromConstantValue(*valueOfDefinedNumber, optionalExpectedExpressionSignalWidth));            
    }

    return std::nullopt;
}

std::any SyReCCustomVisitor::visitExpressionFromSignal(SyReCParser::ExpressionFromSignalContext* context) {
    std::optional<ExpressionEvaluationResult::ptr> expressionFromSignal;

    const auto definedSignal = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->signal());
    if (!definedSignal.has_value()) {
        return expressionFromSignal;   
    }
    
    syrec::expression::ptr expressionWrapperForSignal;
    if ((*definedSignal)->isVariableAccess()) {
        expressionWrapperForSignal = std::make_shared<syrec::VariableExpression>(*(*definedSignal)->getAsVariableAccess());
        expressionFromSignal.emplace(ExpressionEvaluationResult::createFromExpression(expressionWrapperForSignal));
    } else if ((*definedSignal)->isConstant()) {
        // TODO: Is this the correct calculation and is it both safe and does it return the expected result
        const auto constantValueOfDefinedSignal = tryEvaluateNumber(*(*definedSignal)->getAsNumber());

        // TODO: Correct handling of loop variable
        expressionWrapperForSignal = std::make_shared<syrec::NumericExpression>(*(*definedSignal)->getAsNumber(), constantValueOfDefinedSignal.has_value() ? ExpressionEvaluationResult::getRequiredBitWidthToStoreSignal(*constantValueOfDefinedSignal) : 0);    
        expressionFromSignal.emplace(ExpressionEvaluationResult::createFromExpression(expressionWrapperForSignal));
    }

    return expressionFromSignal;
}

std::any SyReCCustomVisitor::visitBinaryExpression(SyReCParser::BinaryExpressionContext* context) {
    const auto lhsOperand = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->lhsOperand);
    const auto userDefinedOperation = getDefinedOperation(context->binaryOperation);
    if (!userDefinedOperation.has_value() || !isValidBinaryOperation(userDefinedOperation.value())) {
        createErrorAtTokenPosition(context->binaryOperation, InvalidBinaryOperation);
    }

    bool modifiedExpectedExpressionSignalWidth = !optionalExpectedExpressionSignalWidth.has_value();
    if (lhsOperand.has_value()) {
        optionalExpectedExpressionSignalWidth.emplace((*(*lhsOperand)->getAsExpression())->bitwidth());
        modifiedExpectedExpressionSignalWidth = true;
    }

    const auto rhsOperand = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->rhsOperand);

    std::optional<ExpressionEvaluationResult::ptr> result = std::nullopt;
    if (lhsOperand.has_value() && userDefinedOperation.has_value() && rhsOperand.has_value()) {
        const auto lhsOperandConversionToConstant = (*lhsOperand)->getAsConstant();
        const auto rhsOperandConversionToConstant = (*rhsOperand)->getAsConstant();

        if (lhsOperandConversionToConstant.has_value() && rhsOperandConversionToConstant.has_value()) {
            const auto binaryExpressionEvaluated = applyBinaryOperation(*userDefinedOperation, *lhsOperandConversionToConstant, *rhsOperandConversionToConstant);
            if (binaryExpressionEvaluated.has_value()) {
                result.emplace(ExpressionEvaluationResult::createFromConstantValue(*binaryExpressionEvaluated, (*(*lhsOperand)->getAsExpression())->bitwidth()));
            } else {
                // TODO: Error creation
                // TODO: Error position
                createErrorAtTokenPosition(nullptr, "TODO: Calculation of binary expression for constant values failed");   
            }
        } else {
            const auto lhsOperandAsExpression   = (*lhsOperand)->getAsExpression();
            const auto rhsOperandAsExpression   = (*rhsOperand)->getAsExpression();
            const auto binaryExpressionBitwidth = std::max((*lhsOperandAsExpression)->bitwidth(), (*rhsOperandAsExpression)->bitwidth());

            // TODO: Handling of binary expression bitwidth, i.e. both operands will be "promoted" to a bitwidth of the larger expression
            result.emplace(ExpressionEvaluationResult::createFromExpression(std::make_shared<syrec::BinaryExpression>(*lhsOperandAsExpression, *ParserUtilities::mapOperationToInternalFlag(*userDefinedOperation), *rhsOperandAsExpression)));
        }
    }

    if (modifiedExpectedExpressionSignalWidth) {
        optionalExpectedExpressionSignalWidth.reset();
    }

    return result;
}

std::any SyReCCustomVisitor::visitUnaryExpression(SyReCParser::UnaryExpressionContext* context) {
    const auto userDefinedOperation = getDefinedOperation(context->unaryOperation);
    if (!userDefinedOperation.has_value() || (* userDefinedOperation != syrec_operation::operation::bitwise_negation && *userDefinedOperation != syrec_operation::operation::logical_negation)) { 
        createErrorAtTokenPosition(context->unaryOperation, InvalidUnaryOperation);
    }
    
    const auto userDefinedExpression = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult>(context->expression());
    // TOD: Add either error or warning that unary expressions are not supported
    return std::nullopt;
}

std::any SyReCCustomVisitor::visitShiftExpression(SyReCParser::ShiftExpressionContext* context) {
    const auto expressionToShift = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->expression());
    const auto definedShiftOperation    = context->shiftOperation != nullptr ? getDefinedOperation(context->shiftOperation) : std::nullopt;

    if (!definedShiftOperation.has_value() || (*definedShiftOperation != syrec_operation::operation::shift_left && *definedShiftOperation != syrec_operation::operation::shift_right)) {
        createErrorAtTokenPosition(context->shiftOperation,InvalidShiftOperation);
    }

    const auto shiftAmount = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->number());

    if (!expressionToShift.has_value() || !shiftAmount.has_value()) {
        return std::nullopt;
    }

    const auto shiftAmountEvaluated = tryEvaluateNumber(*shiftAmount);
    const auto expressionToShiftEvaluated = (*expressionToShift)->getAsConstant();

    if (shiftAmountEvaluated.has_value() && expressionToShiftEvaluated.has_value()) {
        const auto shiftOperationApplicationResult = apply(*definedShiftOperation, *expressionToShiftEvaluated, *shiftAmountEvaluated);
        if (shiftOperationApplicationResult.has_value()) {
            return std::make_optional(ExpressionEvaluationResult::createFromConstantValue(*shiftOperationApplicationResult, (*(*expressionToShift)->getAsExpression())->bitwidth()));
        }

        // TODO: GEN_ERROR
        // TODO: Error position
        createErrorAtTokenPosition(context->expression()->start, "TODO: SHIFT CALC ERROR");
        return std::nullopt;   
    }

    // TODO: Mapping from operation to internal operation 'integer' value
    const auto lhsShiftOperationOperandExpr     = *(*expressionToShift)->getAsExpression();

    /* TODO: Refactor after loop variable lookup is fixed
    const auto rhsShiftOperationOperandBitwidth = ExpressionEvaluationResult::getRequiredBitWidthToStoreSignal((*shiftAmount)->evaluate(loopVariableMappingLookup));
    const auto shiftExpressionBitwidth          = std::max(lhsShiftOperationOperandExpr->bitwidth(), rhsShiftOperationOperandBitwidth);
    */

    // TODO: Handling of shift expression bitwidth, i.e. both operands will be "promoted" to a bitwidth of the larger expression
    const auto createdShiftExpression = std::make_shared<syrec::ShiftExpression>(*(*expressionToShift)->getAsExpression(), *ParserUtilities::mapOperationToInternalFlag(*definedShiftOperation), *shiftAmount);
    return std::make_optional(ExpressionEvaluationResult::createFromExpression(createdShiftExpression));
}

/*
 * Statment production visitors
 */
// TODO: Store number of calls done in defined statements and check if any remained open (i.e. if any uncall is missing)
std::any SyReCCustomVisitor::visitStatementList(SyReCParser::StatementListContext* context) {
    // TODO: If custom casting utility function supports polymorphism as described in the description of the function, this stack is no longer needed
    statementListContainerStack.push({});
    //std::vector<syrec::Statement::ptr> validUserDefinedStatements;
    for (const auto& userDefinedStatement : context->stmts) {
        if (userDefinedStatement != nullptr) {
            visit(userDefinedStatement);   
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

    // Wrapping into std::optional is only needed because helper function to cast return type of production required this wrapper
    return validUserDefinedStatements.empty() ? std::nullopt : std::make_optional(validUserDefinedStatements); 
}

// TODO: Add tests for ambiguity, etc.
std::any SyReCCustomVisitor::visitCallStatement(SyReCParser::CallStatementContext* context) {
    bool isValidCallOperationDefined = context->OP_CALL() != nullptr || context->OP_UNCALL() != nullptr;
    const std::optional<bool> isCallOperation                    = isValidCallOperationDefined ? std::make_optional(context->OP_CALL() != nullptr) : std::nullopt;
    const auto                moduleIdent                 = context->moduleIdent != nullptr ? context->moduleIdent->getText() : "<undefined>";
    
    // TODO: Check for previous uncalls, etc.
    // TODO: Refactoring
    if (isCallOperation.has_value()) {
        if (!callStack.empty() && *isCallOperation) {
            // TODO: Error position
            createErrorAtTokenPosition(context->moduleIdent, PreviousCallWasNotUncalled);
            isValidCallOperationDefined = false;
        }
        else if (!(*isCallOperation)) {
            if (callStack.empty()) {
                // TODO: Error position
                createErrorAtTokenPosition(context->moduleIdent, UncallWithoutPreviousCall);
                isValidCallOperationDefined = false;
            }
            else if (callStack.top().first != moduleIdent) {
                // TODO: Error position
                createErrorAtTokenPosition(context->moduleIdent, fmt::format(MissmatchOfModuleIdentBetweenCalledAndUncall, callStack.top().first, moduleIdent));
                isValidCallOperationDefined = false;
            }
        }
    }

    const auto potentialModulesToCall = ModuleCallGuess::fetchPotentialMatchesForMethodIdent(currentSymbolTableScope, moduleIdent);
    if (!potentialModulesToCall.has_value()) {
        // TODO: Error position
        createErrorAtTokenPosition(context->moduleIdent, fmt::format(UndeclaredIdent, moduleIdent));
        isValidCallOperationDefined = false;
    }

    std::size_t              numActualCalleeArguments = 0;
    std::vector<std::string> calleeArguments;
    for (const auto& userDefinedCallArgument : context->calleeArguments) {
        if (userDefinedCallArgument != nullptr && checkIfSignalWasDeclaredOrLogError(userDefinedCallArgument)) {
            const auto symTabEntryForCalleeArgument = *currentSymbolTableScope->getVariable(userDefinedCallArgument->getText());
            isValidCallOperationDefined &= std::holds_alternative<syrec::Variable::ptr>(symTabEntryForCalleeArgument);
            if (isValidCallOperationDefined) {
                (*potentialModulesToCall)->refineGuessWithNextParameter(std::get<syrec::Variable::ptr>(symTabEntryForCalleeArgument));
                calleeArguments.emplace_back(userDefinedCallArgument->getText());
            }
        }
        else {
            isValidCallOperationDefined = false;
        }

        // TODO: Handling / Error creation in else branches
        numActualCalleeArguments++;
    }

    // TODO: What should happen with previously pushed entries on the call stack if an invalid uncall statement is defined ?
    if (!isValidCallOperationDefined) {
        return 0;
    }

    // TODO: Invalid calls should still be pushed/pop from the callstack ?
    // I.e. call test(c); uncall test(c,d) [Without pushing the first call, the uncall does not know which call is reference]

    // TODO: Checks for correct number of arguments
    (*potentialModulesToCall)->discardGuessesWithMoreThanNParameters(numActualCalleeArguments);
    if (!(*potentialModulesToCall)->hasSomeMatches()) {
        // TODO: Non of the potential calls acceps x arguments
        // TODO: Error position
        createErrorAtTokenPosition(context->moduleIdent, fmt::format(NoMatchForGuessWithNActualParameters, moduleIdent, numActualCalleeArguments));
        return 0;
    }

    if ((*potentialModulesToCall)->getMatchesForGuess().size() > 1) {
        // TODO: GEN_ERROR Ambigous call, more than one match for given arguments
        // TODO: Error position
        createErrorAtTokenPosition(context->moduleIdent, fmt::format(AmbigousCall, moduleIdent));
        return 0;
    }

    // TODO: Refactoring in own function
    if (!callStack.empty()) {
        const auto argumentsOfPreviousCallOperation = callStack.top().second;
        bool       parametersBetweenCallAndUncalledMatched = true;

        if (calleeArguments.size() != argumentsOfPreviousCallOperation.size()) {
            createErrorAtTokenPosition(context->moduleIdent, fmt::format(NumberOfParametersMissmatchBetweenCallAndUncall, moduleIdent, argumentsOfPreviousCallOperation.size(), calleeArguments.size()));
            parametersBetweenCallAndUncalledMatched = false;
        }

        std::vector<std::string> missmatchedParameterValues;
        for (std::size_t parameterIdx = 0; parameterIdx < numActualCalleeArguments; ++parameterIdx) {
            if (calleeArguments.at(parameterIdx) != argumentsOfPreviousCallOperation.at(parameterIdx)) {
                missmatchedParameterValues.emplace_back(fmt::format(ParameterValueMissmatch, parameterIdx, calleeArguments.at(parameterIdx), argumentsOfPreviousCallOperation.at(parameterIdx)));
            }
        }

        if (!missmatchedParameterValues.empty()) {
            std::ostringstream errorsConcatinatedBuffer;
            std::copy(missmatchedParameterValues.cbegin(), missmatchedParameterValues.cend(), infix_ostream_iterator<std::string>(errorsConcatinatedBuffer, ","));
    
            // TODO: Error position
            createErrorAtTokenPosition(context->moduleIdent, fmt::format(CallAndUncallArgumentsMissmatch, moduleIdent, errorsConcatinatedBuffer.str()));
            parametersBetweenCallAndUncalledMatched = false;
        }

        if (!parametersBetweenCallAndUncalledMatched) {
            return 0;
        }
    }

    const auto moduleMatchingCalleeArguments = (*potentialModulesToCall)->getMatchesForGuess().at(0);
    if (isValidCallOperationDefined) {
        if (*isCallOperation) {
            addStatementToOpenContainer(std::make_shared<syrec::CallStatement>(moduleMatchingCalleeArguments, calleeArguments));
            callStack.emplace(moduleIdent, calleeArguments);
        }
        else {
            addStatementToOpenContainer(std::make_shared<syrec::UncallStatement>(moduleMatchingCalleeArguments, calleeArguments));
            callStack.pop();
        }
    }

    return 0;
}

std::any SyReCCustomVisitor::visitForStatement(SyReCParser::ForStatementContext* context) {
    const auto loopVariableIdent = tryVisitAndConvertProductionReturnValue<std::string>(context->loopVariableDefinition());
    bool       loopHeaderValid = true;
    if (loopVariableIdent.has_value()) {
        if (currentSymbolTableScope->contains(*loopVariableIdent)) {
            createErrorAtTokenPosition(context->loopVariableDefinition()->variableIdent, fmt::format(DuplicateDeclarationOfIdent, *loopVariableIdent));
            loopHeaderValid = false;
        }
        else {
            SymbolTable::openScope(currentSymbolTableScope);
            currentSymbolTableScope->addEntry(std::make_shared<syrec::Number>(*loopVariableIdent));
        }
    }

    const bool       wasStartValueExplicitlyDefined = context->startValue != nullptr;
    const auto rangeStartValue = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->startValue);
    loopHeaderValid &= wasStartValueExplicitlyDefined ? rangeStartValue.has_value() : true;

    const auto rangeEndValue = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->endValue);
    loopHeaderValid &= rangeEndValue.has_value();

    const bool       wasStepSizeExplicitlyDefined = context->loopStepsizeDefinition() != nullptr;
    std::optional <syrec::Number::ptr> stepSize;
    if (wasStepSizeExplicitlyDefined) {
        stepSize = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->loopStepsizeDefinition());
        loopHeaderValid &= stepSize.has_value();
    } else {
        stepSize.emplace(std::make_shared<syrec::Number>(1));
    }
    
    if (loopHeaderValid) {
        const unsigned int iterationRangeEndValueEvaluated   = (*rangeEndValue)->evaluate(loopVariableMappingLookup);
        const unsigned int iterationRangeStartValueEvaluated = wasStartValueExplicitlyDefined ? (*rangeStartValue)->evaluate(loopVariableMappingLookup) : iterationRangeEndValueEvaluated;
        const unsigned int stepSizeEvaluated                 = (*stepSize)->evaluate(loopVariableMappingLookup);
        const bool         negativeStepSizeDefined           = wasStepSizeExplicitlyDefined ? stepSizeEvaluated < 0 : false;

        // TODO: Error generation if semantic range check fails
        unsigned int numIterations = 0;
        if ((negativeStepSizeDefined && iterationRangeStartValueEvaluated < iterationRangeEndValueEvaluated) || (!negativeStepSizeDefined && iterationRangeStartValueEvaluated > iterationRangeEndValueEvaluated) || stepSizeEvaluated == 0) {
            // TODO: Error position
            createErrorAtTokenPosition(wasStartValueExplicitlyDefined ? context->startValue->start : context->endValue->start, "TODO: Error generation if semantic range check fails");
            loopHeaderValid = false;
        } else {
            numIterations = (negativeStepSizeDefined ? (iterationRangeStartValueEvaluated - iterationRangeEndValueEvaluated) : (iterationRangeEndValueEvaluated - iterationRangeStartValueEvaluated)) + 1;
            numIterations /= stepSizeEvaluated;
        }
    }

    // TODO: Is check context->statementList() == nullptr a required check
    // It seems like with syntax error in the components of the loop header the statementList will be null
    const auto loopBody = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(context->statementList());
    const bool       isValidLoopBody = loopBody.has_value() && !(*loopBody).empty();

    // TODO: Instead of opening and closing a new scope simply insert and remove the entry from the symbol table
    if (loopVariableIdent.has_value()) {
        SymbolTable::closeScope(currentSymbolTableScope);
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

    return 0;
}

std::any SyReCCustomVisitor::visitLoopStepsizeDefinition(SyReCParser::LoopStepsizeDefinitionContext* context) {
    // TODO: Handling of negative step size
    const bool isNegativeStepSize = context->OP_MINUS() != nullptr;
    return tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->number());
}

// TODO: Check that loop variable was not declared before
std::any SyReCCustomVisitor::visitLoopVariableDefinition(SyReCParser::LoopVariableDefinitionContext* context) {
    return context->variableIdent != nullptr ? std::make_optional("$" + context->IDENT()->getText()) : std::nullopt;
}


// TODO: Empty branches (i.e. statement lists) do not throw an error, if the statment is not syntacically correct the statement list will be null
std::any SyReCCustomVisitor::visitIfStatement(SyReCParser::IfStatementContext* context) {
    const auto guardExpression = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->guardCondition);
    const auto trueBranchStmts       = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(context->trueBranchStmts);
    const auto falseBranchStmts = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(context->falseBranchStmts);
    const auto closingGuardExpression = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->matchingGuardExpression);

    // TODO: Replace pointer comparison with actual equality check of expressions
    if (!guardExpression.has_value() 
        || (!trueBranchStmts.has_value() || (*trueBranchStmts).empty()) 
        || (!falseBranchStmts.has_value() || (*falseBranchStmts).empty()) 
        || !closingGuardExpression.has_value()) {
        return 0;        
    }

    // TODO: Error position
    if (!areExpressionsEqual(*guardExpression, *closingGuardExpression)) {
        createErrorAtTokenPosition(context->getStart(), IfAndFiConditionMissmatch);
        return 0;
    }

    const auto ifStatement = std::make_shared<syrec::IfStatement>();
    ifStatement->condition      = (*guardExpression)->getAsExpression().value();
    ifStatement->thenStatements = *trueBranchStmts;
    ifStatement->elseStatements = *falseBranchStmts;
    ifStatement->fiCondition    = (*closingGuardExpression)->getAsExpression().value();
    addStatementToOpenContainer(ifStatement);
    return 0;
}

std::any SyReCCustomVisitor::visitUnaryStatement(SyReCParser::UnaryStatementContext* context) {
    bool       allSemanticChecksOk = true;
    const auto unaryOperation = getDefinedOperation(context->unaryOp);

    if (!unaryOperation.has_value() 
        || (syrec_operation::operation::invert_assign != *unaryOperation
            && syrec_operation::operation::increment_assign != *unaryOperation
            && syrec_operation::operation::decrement_assign != *unaryOperation)
        ) {
        allSemanticChecksOk = false;
        // TODO: Error position
        createErrorAtTokenPosition(context->unaryOp, InvalidUnaryOperation);
    }

    const auto accessedSignal = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->signal());
    allSemanticChecksOk &= accessedSignal.has_value() && (*accessedSignal)->isVariableAccess() && isSignalAssignableOtherwiseCreateError(context->signal()->IDENT()->getSymbol(), *(*accessedSignal)->getAsVariableAccess());
    if (allSemanticChecksOk) {
        // TODO: Add mapping from custom operation enum to internal "numeric" flag
        addStatementToOpenContainer(std::make_shared<syrec::UnaryStatement>(*ParserUtilities::mapOperationToInternalFlag(*unaryOperation), *(*accessedSignal)->getAsVariableAccess()));
    }

    return 0;
}

std::any SyReCCustomVisitor::visitAssignStatement(SyReCParser::AssignStatementContext* context) {
    const auto assignedToSignal = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->signal());
    bool       allSemanticChecksOk = assignedToSignal.has_value()
        && (*assignedToSignal)->isVariableAccess() && isSignalAssignableOtherwiseCreateError(context->signal()->IDENT()->getSymbol(), *(*assignedToSignal)->getAsVariableAccess());

    if (allSemanticChecksOk) {
        optionalExpectedExpressionSignalWidth.emplace((*(*assignedToSignal)->getAsVariableAccess())->bitwidth());
    }

    const auto definedAssignmentOperation = getDefinedOperation(context->assignmentOp);
    if (!definedAssignmentOperation.has_value()
        || (syrec_operation::operation::xor_assign != *definedAssignmentOperation
            && syrec_operation::operation::add_assign != *definedAssignmentOperation
            && syrec_operation::operation::minus_assign != *definedAssignmentOperation)) {
        // TODO: Error position
        createErrorAtTokenPosition(context->assignmentOp, InvalidAssignOperation);
        allSemanticChecksOk = false;
    }

    // TODO: Check if signal widht of left and right operand of assign statement match (similar to swap statement ?)
    const auto assignmentOpRhsOperand = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->expression());
    allSemanticChecksOk &= assignmentOpRhsOperand.has_value();

    if (allSemanticChecksOk) {
        // TODO: Add mapping from custom operation enum to internal "numeric" 
        addStatementToOpenContainer(
                std::make_shared<syrec::AssignStatement>(
                        *(*assignedToSignal)->getAsVariableAccess(),
                        *ParserUtilities::mapOperationToInternalFlag(*definedAssignmentOperation),
                        (*assignmentOpRhsOperand)->getAsExpression().value()));
    }

    if (optionalExpectedExpressionSignalWidth.has_value()) {
        optionalExpectedExpressionSignalWidth.reset();
    }

    return 0;
}

// TODO:
std::any SyReCCustomVisitor::visitSwapStatement(SyReCParser::SwapStatementContext* context) {
    const auto swapLhsOperand = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->lhsOperand);
    const bool lhsOperandOk            = swapLhsOperand.has_value() && (*swapLhsOperand)->isVariableAccess() && isSignalAssignableOtherwiseCreateError(context->lhsOperand->IDENT()->getSymbol(), *(*swapLhsOperand)->getAsVariableAccess());
    
    const auto swapRhsOperand  = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->rhsOperand);
    const bool rhsOperandOk               = swapRhsOperand.has_value() && (*swapRhsOperand)->isVariableAccess() && isSignalAssignableOtherwiseCreateError(context->rhsOperand->IDENT()->getSymbol(), *(*swapRhsOperand)->getAsVariableAccess());

    if (!lhsOperandOk || !rhsOperandOk) {
        return 0;
    }

    const auto lhsAccessedSignal = *(*swapLhsOperand)->getAsVariableAccess();
    const auto rhsAccessedSignal = *(*swapRhsOperand)->getAsVariableAccess();

    bool         allSemanticChecksOk      = true;

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
        createErrorAtTokenPosition(context->start, fmt::format(InvalidSwapNumDimensionsMissmatch, lhsNumAffectedDimensions, rhsNumAffectedDimensions));
        allSemanticChecksOk = false;
    }
    else if (lhsAccessedSignal->indexes.empty() && rhsAccessedSignal->indexes.empty()) {
        const auto& lhsSignalDimensions = lhsAccessedSignal->getVar()->dimensions;
        const auto& rhsSignalDimensions = rhsAccessedSignal->getVar()->dimensions;

        bool continueCheck = true;
        for (size_t dimensionIdx = 0; continueCheck && dimensionIdx < lhsSignalDimensions.size(); ++dimensionIdx) {
            continueCheck = lhsSignalDimensions.at(dimensionIdx) == rhsSignalDimensions.at(dimensionIdx);
            if (!continueCheck) {
                // TODO: Error position
                createErrorAtTokenPosition(context->start, fmt::format(InvalidSwapValueForDimensionMissmatch, dimensionIdx, lhsSignalDimensions.at(dimensionIdx), rhsSignalDimensions.at(dimensionIdx)));
                allSemanticChecksOk = false;
            }
        }
    }

    if (!allSemanticChecksOk) {
        return 0;
    }

    size_t lhsAccessedSignalWidth = lhsAccessedSignal->getVar()->bitwidth;
    size_t rhsAccessedSignalWidth = rhsAccessedSignal->getVar()->bitwidth;
    if (lhsAccessedSignal->range.has_value()) {
        lhsAccessedSignalWidth = *tryEvaluateNumber((*lhsAccessedSignal->range).second) - *tryEvaluateNumber((*lhsAccessedSignal->range).first) + 1;
    }
    if (rhsAccessedSignal->range.has_value()) {
        rhsAccessedSignalWidth = *tryEvaluateNumber((*rhsAccessedSignal->range).second) - *tryEvaluateNumber((*rhsAccessedSignal->range).first) + 1;
    }

    if (lhsAccessedSignalWidth != rhsAccessedSignalWidth) {
        // TODO: Error position
        createErrorAtTokenPosition(context->start, fmt::format(InvalidSwapSignalWidthMissmatch, lhsAccessedSignalWidth, rhsAccessedSignalWidth));
        return 0;
    }

    addStatementToOpenContainer(std::make_shared<syrec::SwapStatement>(lhsAccessedSignal, rhsAccessedSignal));
    return 0;
}

std::any SyReCCustomVisitor::visitSkipStatement(SyReCParser::SkipStatementContext* context) {
    if (context->start != nullptr) {
        addStatementToOpenContainer(std::make_shared<syrec::SkipStatement>());   
    }
    return 0;
}

/*
 * Utility functions for error and warning creation
 */

void SyReCCustomVisitor::createErrorAtTokenPosition(const antlr4::Token* token, const std::string& errorMessage) {
    const std::size_t errorLine = token != nullptr ? token->getLine() : -1;
    const std::size_t errorColumn = token != nullptr ? token->getCharPositionInLine() : -1;
    return createError(errorLine, errorColumn, errorMessage);
}


// TODO:
void SyReCCustomVisitor::createError(const std::size_t line, const std::size_t column, const std::string& errorMessage) {
    errors.emplace_back(ParserUtilities::createError(line, column, errorMessage));
}

// TODO:
void SyReCCustomVisitor::createWarning(const std::string& warningMessage) {
    warnings.emplace_back(ParserUtilities::createWarning(0, 0, warningMessage));
}

std::optional<syrec::Variable::Types> SyReCCustomVisitor::getParameterType(const antlr4::Token* token) {
    std::optional<syrec::Variable::Types> parameterType;
    if (token == nullptr) {
        return parameterType;
    }

    switch (token->getType()) {
        case SyReCParser::VAR_TYPE_IN:
            parameterType.emplace(syrec::Variable::Types::In);
            break;
        case SyReCParser::VAR_TYPE_OUT:
            parameterType.emplace(syrec::Variable::Types::Out);
            break;
        case SyReCParser::VAR_TYPE_INOUT:
            parameterType.emplace(syrec::Variable::Types::Inout);
            break;
        default:
            // TODO: Error position
            createErrorAtTokenPosition(token, InvalidParameterType);
            break;
    }
    return parameterType;
}

std::optional<syrec::Variable::Types> SyReCCustomVisitor::getSignalType(const antlr4::Token* token) {
    std::optional<syrec::Variable::Types> signalType;
    if (token == nullptr) {
        return signalType;
    }

    switch (token->getType()) {
        case SyReCParser::VAR_TYPE_WIRE:
            signalType.emplace(syrec::Variable::Types::Wire);
            break;
        case SyReCParser::VAR_TYPE_STATE:
            signalType.emplace(syrec::Variable::Types::State);
            break;
        default:
            // TODO: Error position
            createErrorAtTokenPosition(token, InvalidLocalType);
            break;
    }
    return signalType;
}

/*
 * Utility functions performing semantic checks, can be maybe refactoring into separate class
 */

bool SyReCCustomVisitor::checkIfSignalWasDeclaredOrLogError(const antlr4::Token* signalIdentToken, bool isLoopVariable) {
    const std::string signalIdent = isLoopVariable ? "$" + signalIdentToken->getText() : signalIdentToken->getText();
    if (!currentSymbolTableScope->contains(signalIdent)) {
        createErrorAtTokenPosition(signalIdentToken, fmt::format(UndeclaredIdent, signalIdent));
        return false;
    }
    return true;
}

// TODO: Rename to a better fitting name (i.e. isValidDimensionAccess)
[[nodiscard]] bool SyReCCustomVisitor::validateSemanticChecksIfDimensionExpressionIsConstant(const antlr4::Token* dimensionToken, const size_t accessedDimensionIdx, const syrec::Variable::ptr& accessedSignal, const std::optional<ExpressionEvaluationResult::ptr>& expressionEvaluationResult) {
    if (!expressionEvaluationResult.has_value()) {
        return false;
    }

    if (!(*expressionEvaluationResult)->isConstantValue()) {
        return true;
    }

    const auto expressionResultAsConstant = (*expressionEvaluationResult)->getAsConstant().value();
    const bool accessOnCurrentDimensionOk = isValidDimensionAccess(accessedSignal, accessedDimensionIdx, expressionResultAsConstant, true);
    if (!accessOnCurrentDimensionOk) {
        // TODO: Using global flag indicating zero_based indexing or not
        const auto constraintForCurrentDimension = getConstraintsForValidDimensionAccess(accessedSignal, accessedDimensionIdx, true).value();

        // TODO: GEN_ERROR: Index out of range for dimension i
        // TODO: Error position
        createErrorAtTokenPosition(dimensionToken,
                fmt::format(
                        DimensionValueOutOfRange,
                        expressionResultAsConstant,
                        accessedDimensionIdx,
                        accessedSignal->name,
                        constraintForCurrentDimension.minimumValidValue, constraintForCurrentDimension.maximumValidValue)
                );
    }

    return accessOnCurrentDimensionOk;
}

[[nodiscard]] std::optional<std::pair<syrec::Number::ptr, syrec::Number::ptr>> SyReCCustomVisitor::isBitOrRangeAccessDefined(SyReCParser::NumberContext* bitRangeStart, SyReCParser::NumberContext* bitRangeEnd) {
    if (bitRangeStart == nullptr && bitRangeEnd == nullptr) {
        return std::nullopt;
    }

    const auto bitRangeStartEvaluated = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(bitRangeStart);
    const auto bitRangeEndEvaluated   = bitRangeEnd != nullptr ? tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(bitRangeEnd) : bitRangeStartEvaluated;

    return (bitRangeStartEvaluated.has_value() && bitRangeEndEvaluated.has_value()) ? std::make_optional(std::make_pair(*bitRangeStartEvaluated, *bitRangeEndEvaluated)) : std::nullopt;
}

std::optional<syrec_operation::operation> SyReCCustomVisitor::getDefinedOperation(const antlr4::Token* definedOperationToken) {
    if (definedOperationToken == nullptr) {
        return std::nullopt;
    }

    std::optional<syrec_operation::operation> definedOperation;
    switch (definedOperationToken->getType()) {
        case SyReCParser::OP_PLUS:
            definedOperation.emplace(syrec_operation::operation::addition);
            break;
        case SyReCParser::OP_MINUS:
            definedOperation.emplace(syrec_operation::operation::subtraction);
            break;
        case SyReCParser::OP_MULTIPLY:
            definedOperation.emplace(syrec_operation::operation::multiplication);
            break;
        case SyReCParser::OP_UPPER_BIT_MULTIPLY:
            definedOperation.emplace(syrec_operation::operation::upper_bits_multiplication);
            break;
        case SyReCParser::OP_DIVISION:
            definedOperation.emplace(syrec_operation::operation::division);
            break;
        case SyReCParser::OP_MODULO:
            definedOperation.emplace(syrec_operation::operation::modulo);
            break;
        case SyReCParser::OP_GREATER_THAN:
            definedOperation.emplace(syrec_operation::operation::greater_than);
            break;
        case SyReCParser::OP_GREATER_OR_EQUAL:
            definedOperation.emplace(syrec_operation::operation::greater_equals);
            break;
        case SyReCParser::OP_LESS_THAN:
            definedOperation.emplace(syrec_operation::operation::less_than);
            break;
        case SyReCParser::OP_LESS_OR_EQUAL:
            definedOperation.emplace(syrec_operation::operation::less_equals);
            break;
        case SyReCParser::OP_EQUAL:
            definedOperation.emplace(syrec_operation::operation::equals);
            break;
        case SyReCParser::OP_NOT_EQUAL:
            definedOperation.emplace(syrec_operation::operation::not_equals);
            break;
        case SyReCParser::OP_BITWISE_AND:
            definedOperation.emplace(syrec_operation::operation::bitwise_and);
            break;
        case SyReCParser::OP_BITWISE_NEGATION:
            definedOperation.emplace(syrec_operation::operation::bitwise_negation);
            break;
        case SyReCParser::OP_BITWISE_OR:
            definedOperation.emplace(syrec_operation::operation::bitwise_or);
            break;
        case SyReCParser::OP_BITWISE_XOR:
            definedOperation.emplace(syrec_operation::operation::bitwise_xor);
            break;
        case SyReCParser::OP_LOGICAL_AND:
            definedOperation.emplace(syrec_operation::operation::logical_and);
            break;
        case SyReCParser::OP_LOGICAL_OR:
            definedOperation.emplace(syrec_operation::operation::logical_or);
            break;
        case SyReCParser::OP_LOGICAL_NEGATION:
            definedOperation.emplace(syrec_operation::operation::logical_negation);
            break;
        case SyReCParser::OP_LEFT_SHIFT:
            definedOperation.emplace(syrec_operation::operation::shift_left);
            break;
        case SyReCParser::OP_RIGHT_SHIFT:
            definedOperation.emplace(syrec_operation::operation::shift_right);
            break;
        case SyReCParser::OP_INCREMENT_ASSIGN:
            definedOperation.emplace(syrec_operation::operation::increment_assign);
            break;
        case SyReCParser::OP_DECREMENT_ASSIGN:
            definedOperation.emplace(syrec_operation::operation::decrement_assign);
            break;
        case SyReCParser::OP_INVERT_ASSIGN:
            definedOperation.emplace(syrec_operation::operation::invert_assign);
            break;
        case SyReCParser::OP_ADD_ASSIGN:
            definedOperation.emplace(syrec_operation::operation::add_assign);
            break;
        case SyReCParser::OP_SUB_ASSIGN:
            definedOperation.emplace(syrec_operation::operation::minus_assign);
            break;
        case SyReCParser::OP_XOR_ASSIGN:
            definedOperation.emplace(syrec_operation::operation::xor_assign);
            break;
        default:
            // TODO: Error position
            createErrorAtTokenPosition(definedOperationToken, "TODO: No mapping for operation");
            break;
    }
    return definedOperation;
}

// TODO: Naming
[[nodiscard]] std::optional<unsigned int> SyReCCustomVisitor::tryEvaluateNumber(const syrec::Number::ptr& numberContainer) {
    if (numberContainer->isLoopVariable()) {
        const std::string& loopVariableIdent = numberContainer->variableName();
        if (loopVariableMappingLookup.find(loopVariableIdent) == loopVariableMappingLookup.end()) {
            return std::nullopt;
        }
    }

    return std::make_optional(numberContainer->evaluate(loopVariableMappingLookup));
}

[[nodiscard]] std::optional<unsigned int> SyReCCustomVisitor::applyBinaryOperation(const syrec_operation::operation operation, const unsigned int leftOperand, const unsigned int rightOperand) {
    if (operation == syrec_operation::operation::division && rightOperand == 0) {
        // TODO: GEN_ERROR
        // TODO: Error position
        createErrorAtTokenPosition(nullptr, DivisionByZero);
        return std::nullopt;
    }

    return apply(operation, leftOperand, rightOperand);
}

bool SyReCCustomVisitor::isValidBinaryOperation(syrec_operation::operation userDefinedOperation) {
    bool isValid;
    switch (userDefinedOperation) {
        case syrec_operation::operation::addition:
        case syrec_operation::operation::subtraction:
        case syrec_operation::operation::bitwise_xor:
        case syrec_operation::operation::multiplication:
        case syrec_operation::operation::division:
        case syrec_operation::operation::modulo:
        case syrec_operation::operation::upper_bits_multiplication:
        case syrec_operation::operation::logical_and:
        case syrec_operation::operation::logical_or:
        case syrec_operation::operation::bitwise_and:
        case syrec_operation::operation::bitwise_or:
        case syrec_operation::operation::less_than:
        case syrec_operation::operation::greater_than:
        case syrec_operation::operation::equals:
        case syrec_operation::operation::not_equals:
        case syrec_operation::operation::less_equals:
        case syrec_operation::operation::greater_equals:
            isValid = true;
            break;
        default:
            isValid = false;
            break;
    }
    return isValid;
}

// TODO:
[[nodiscard]] bool SyReCCustomVisitor::isSignalAssignableOtherwiseCreateError(const antlr4::Token* signalIdentToken, const syrec::VariableAccess::ptr& assignedToVariable) {
    if (syrec::Variable::Types::In == assignedToVariable->getVar()->type) {
        // TODO: Error position
        createErrorAtTokenPosition(signalIdentToken, fmt::format(AssignmentToReadonlyVariable, assignedToVariable->getVar()->name));
        return false;
    }
    return true;
}


void SyReCCustomVisitor::addStatementToOpenContainer(const syrec::Statement::ptr& statement) {
    statementListContainerStack.top().emplace_back(statement);
}

[[nodiscard]] bool SyReCCustomVisitor::areExpressionsEqual(const ExpressionEvaluationResult::ptr& firstExpr, const ExpressionEvaluationResult::ptr& otherExpr) {
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