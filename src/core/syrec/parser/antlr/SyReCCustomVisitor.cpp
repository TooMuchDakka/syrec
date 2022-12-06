#include <climits>
#include <cstdint>
#include <optional>
#include <string>
#include <fmt/format.h>
#include "core/syrec/parser/custom_semantic_errors.hpp"

#include "SyReCCustomVisitor.h"

#include "core/syrec/parser/expression_evaluation_result.hpp"
#include "core/syrec/parser/range_check.hpp"

using namespace parser;

// https://stackoverflow.com/questions/60420005/programmatically-access-antlr-grammar-rule-string-literals-outside-of-a-parse-co

std::any SyReCCustomVisitor::visitProgram(SyReCParser::ProgramContext* context) {
    SymbolTable::openScope(currentSymbolTableScope);
    for (const auto& module : context->module()) {
        const auto moduleParseResult = tryConvertProductionReturnValue<syrec::Module::ptr>(visit(module));
        if (moduleParseResult.has_value()) {
            if (currentSymbolTableScope->contains(moduleParseResult.value())) {
                createError(fmt::format(DuplicateModuleIdentDeclaration, moduleParseResult.value()->name));   
            }
            else {
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

    bool                                isDeclaredModuleValid = true;
    std::optional<syrec::Variable::vec> declaredParameters;
    if (context->parameterList() != nullptr) {
        declaredParameters = tryConvertProductionReturnValue<syrec::Variable::vec>(visit(context->parameterList()));
        isDeclaredModuleValid &= declaredParameters.has_value();
    }
    
    if (isDeclaredModuleValid && declaredParameters.has_value()) {
        for (const auto& declaredParameter : declaredParameters.value()) {
            module->addParameter(declaredParameter);
        }
    }

    for (const auto& declaredSignals: context->signalList()) {
        const auto parsedDeclaredSignals = tryConvertProductionReturnValue<syrec::Variable::vec>(visit(declaredSignals));
        isDeclaredModuleValid &= parsedDeclaredSignals.has_value();
        if (isDeclaredModuleValid) {
            for (const auto& declaredSignal : parsedDeclaredSignals.value()) {
                module->addVariable(declaredSignal);   
            }
        }
    } 

    // TODO: Parsed statements

    SymbolTable::closeScope(this->currentSymbolTableScope);
    return isDeclaredModuleValid ? std::make_optional(module) : std::nullopt;
}

std::any SyReCCustomVisitor::visitParameterList(SyReCParser::ParameterListContext* context) {
    bool                                             allParametersValid = true;
    syrec::Variable::vec                             parametersContainer;
    std::optional<syrec::Variable::vec>              parameters;

    for (const auto& parameter : context->parameter()) {
        const auto parsedParameterDefinition = tryConvertProductionReturnValue<syrec::Variable::ptr>(visit(parameter));
        allParametersValid &= parsedParameterDefinition.has_value();
        if (allParametersValid) {
            parametersContainer.emplace_back(parsedParameterDefinition.value());
        }
    }

    if (allParametersValid) {
        parameters.emplace(parametersContainer);
    }

    return parameters;
}

std::any SyReCCustomVisitor::visitParameter(SyReCParser::ParameterContext* context) {
     const auto parameterType = tryConvertProductionReturnValue<syrec::Variable::Types>(getParameterType(context->start));
    if (!parameterType.has_value()) {
        createError(InvalidParameterType);
    } 

    auto declaredParameter = tryConvertProductionReturnValue<syrec::Variable::ptr>(visit(context->signalDeclaration()));
    if (declaredParameter.has_value()) {
        declaredParameter.value()->type = parameterType.value();
        currentSymbolTableScope->addEntry(declaredParameter.value());
    }
    return declaredParameter;
}

std::any SyReCCustomVisitor::visitSignalList(SyReCParser::SignalListContext* context) {
    bool                                isValidSignalListDeclaration = true;

    const auto declaredSignalsType = tryConvertProductionReturnValue<syrec::Variable::Types>(getSignalType(context->start));
    if (!declaredSignalsType.has_value()) {
        createError(InvalidLocalType);
        isValidSignalListDeclaration = false;
    }

    syrec::Variable::vec declaredSignalsOfType{};
    for (const auto& signal : context->signalDeclaration()) {
        const auto declaredSignal = tryConvertProductionReturnValue<syrec::Variable::ptr>(visit(signal));
        isValidSignalListDeclaration &= declaredSignal.has_value();

        if (isValidSignalListDeclaration) {
            declaredSignal.value()->type = declaredSignalsType.value();
            declaredSignalsOfType.emplace_back(declaredSignal.value());
            currentSymbolTableScope->addEntry(declaredSignal.value());
        }
    }
    
    return std::make_optional(declaredSignalsOfType);
}

std::any SyReCCustomVisitor::visitSignalDeclaration(SyReCParser::SignalDeclarationContext* context) {
    std::optional<syrec::Variable::ptr> signal;
    std::vector<unsigned int> dimensions {};
    unsigned int              signalWidth = this->config.cDefaultSignalWidth;
    bool              isValidSignalDeclaration = true;

    const std::string signalIdent = context->IDENT()->getText();
    if (currentSymbolTableScope->contains(signalIdent)) {
        isValidSignalDeclaration = false;
        createError(fmt::format(DuplicateDeclarationOfIdent, signalIdent));
    }

    for (const auto& dimensionToken: context->dimensionTokens) {
        std::optional<unsigned int> dimensionTokenValueAsNumber = convertToNumber(dimensionToken);
        if (dimensionTokenValueAsNumber.has_value()) {
            dimensions.emplace_back(dimensionTokenValueAsNumber.value());
        } else {
            isValidSignalDeclaration = false;
        }
    }

    if (context->signalWidthToken != nullptr) {
        const std::optional<unsigned int> customSignalWidth = convertToNumber(context->signalWidthToken);
        if (customSignalWidth.has_value()) {
            signalWidth = customSignalWidth.value();
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
    bool isValidSignalAccess = context->IDENT() != nullptr && checkIfSignalWasDeclaredOrLogError(context->IDENT()->getText());

    
    if (isValidSignalAccess) {
        signalIdent = context->IDENT()->getText();
        const auto signalSymTabEntry = currentSymbolTableScope->getVariable(signalIdent);
        isValidSignalAccess          = signalSymTabEntry.has_value() && std::holds_alternative<syrec::Variable::ptr>(signalSymTabEntry.value());

        if (isValidSignalAccess) {
            const syrec::VariableAccess::ptr container = std::make_shared<syrec::VariableAccess>();
            container->setVar(std::get<syrec::Variable::ptr>(signalSymTabEntry.value()));
            accessedSignal.emplace(container);
        }
    }

    const size_t numDimensionsOfAccessSignal = accessedSignal.has_value() ? accessedSignal.value()->getVar()->dimensions.size() : SIZE_MAX;
    const size_t numUserDefinedDimensions    = context->accessedDimensions.size();
    if (isValidSignalAccess) {
        const size_t numElementsWithinRange = std::min(numUserDefinedDimensions, numDimensionsOfAccessSignal);
        for (size_t dimensionIdx = 0; dimensionIdx < numElementsWithinRange; ++dimensionIdx) {
            const auto& definedDimensionAccess      = context->accessedDimensions.at(dimensionIdx);
            const auto accessedDimensionExpression = std::any_cast<ExpressionEvaluationResult::ptr>(visit(definedDimensionAccess));

            if (accessedSignal.has_value()) {
                isValidSignalAccess = validateSemanticChecksIfDimensionExpressionIsConstant(dimensionIdx, accessedSignal.value()->getVar(), accessedDimensionExpression);
                if (isValidSignalAccess) {
                    // TODO: Set correct bit width of expression
                    accessedSignal.value()->indexes.emplace_back(accessedDimensionExpression->getAsExpression().value());
                }
            }
        }

        isValidSignalAccess &= numUserDefinedDimensions > numElementsWithinRange;
        for (size_t dimensionOutOfRangeIdx = numElementsWithinRange; dimensionOutOfRangeIdx < numUserDefinedDimensions; ++dimensionOutOfRangeIdx) {
            createError(fmt::format(DimensionOutOfRange, dimensionOutOfRangeIdx, signalIdent, numDimensionsOfAccessSignal));
        }
    }

    const auto bitOrRangeAccessEvaluated = isBitOrRangeAccessDefined(context->bitStart, context->bitRangeEnd);
    if (accessedSignal.has_value() && bitOrRangeAccessEvaluated.has_value()) {
        const auto  accessVariable                = accessedSignal.value()->getVar();
        const auto& bitOrRangeAccessPair = bitOrRangeAccessEvaluated.value();
        const auto& bitOrRangeAccessPairEvaluated = std::make_pair(bitOrRangeAccessPair.first->evaluate({}), bitOrRangeAccessPair.second->evaluate({}));

        if (bitOrRangeAccessPair.first == bitOrRangeAccessPair.second) {
            if (!isValidBitAccess(accessVariable, bitOrRangeAccessPairEvaluated.first, true)) {
                isValidSignalAccess                                      = false;
                const IndexAccessRangeConstraint constraintsForBitAccess = getConstraintsForValidBitAccess(accessVariable, true);
                // TODO: GEN_ERROR: Bit access out of range
                createError(fmt::format(BitAccessOutOfRange, bitOrRangeAccessPairEvaluated.first, signalIdent, constraintsForBitAccess.minimumValidValue, constraintsForBitAccess.maximumValidValue));
            }
        }
        else {
            if (bitOrRangeAccessPairEvaluated.first > bitOrRangeAccessPairEvaluated.second) {
                isValidSignalAccess = false;
                // TODO: GEN_ERROR: Bit range start larger than end
                createError(fmt::format(BitRangeStartLargerThanEnd, bitOrRangeAccessPairEvaluated.first, bitOrRangeAccessPairEvaluated.second));
            }
            else if (!isValidBitRangeAccess(accessVariable, bitOrRangeAccessPairEvaluated, true)) {
                isValidSignalAccess                                           = false;
                const IndexAccessRangeConstraint constraintsForBitRangeAccess = getConstraintsForValidBitAccess(accessedSignal.value()->getVar(), true);
                // TODO: GEN_ERROR: Bit range out of range
                createError(fmt::format(BitRangeOutOfRange, bitOrRangeAccessPairEvaluated.first, bitOrRangeAccessPairEvaluated.second, signalIdent, constraintsForBitRangeAccess.minimumValidValue, constraintsForBitRangeAccess.maximumValidValue));
            }
        }

        if (isValidSignalAccess) {
            accessedSignal.value()->range.emplace(bitOrRangeAccessPair);
        }
    }

    SignalEvaluationResult::ptr result = std::make_shared<SignalEvaluationResult>();
    if (isValidSignalAccess) {
        result->updateResultToVariableAccess(accessedSignal.value());   
    }
    return result;
}

/*
 * Number production variants
 */
std::any SyReCCustomVisitor::visitNumberFromConstant(SyReCParser::NumberFromConstantContext* context) {
    if (context->INT() == nullptr) {
        return std::nullopt;   
    }

    return std::make_optional(std::make_shared<syrec::Number>(convertToNumber(context->INT()->getText()).value()));
}

std::any SyReCCustomVisitor::visitNumberFromSignalwidth(SyReCParser::NumberFromSignalwidthContext* context) {
    if (context->IDENT() == nullptr) {
        return std::nullopt;
    }
    
    const std::string                 signalIdent = context->IDENT()->getText();
    if (!checkIfSignalWasDeclaredOrLogError(context->IDENT()->getText())) {
        return std::nullopt;
    }

    std::optional<syrec::Number::ptr> signalWidthOfSignal;
    const auto& symTableEntryForSignal = currentSymbolTableScope->getVariable(signalIdent);
    if (symTableEntryForSignal.has_value() && std::holds_alternative<syrec::Variable::ptr>(symTableEntryForSignal.value())) {
        signalWidthOfSignal.emplace(std::make_shared<syrec::Number>(std::get<syrec::Variable::ptr>(symTableEntryForSignal.value())->bitwidth));
    }
    else {
        // TODO: GEN_ERROR, this should not happen
        createError("TODO");
    }

    return signalWidthOfSignal;
}

std::any SyReCCustomVisitor::visitNumberFromLoopVariable(SyReCParser::NumberFromLoopVariableContext* context) {
    if (context->IDENT() == nullptr) {
        return std::nullopt;
    }

    const std::string signalIdent = "$" + context->IDENT()->getText();
    if (!checkIfSignalWasDeclaredOrLogError(context->IDENT()->getText())) {
        return std::nullopt;
    }

    std::optional<syrec::Number::ptr> valueOfLoopVariable;
    const auto&                       symTableEntryForSignal = currentSymbolTableScope->getVariable(signalIdent);
    if (symTableEntryForSignal.has_value() && std::holds_alternative<syrec::Number::ptr>(symTableEntryForSignal.value())) {
        valueOfLoopVariable.emplace(std::get<syrec::Number::ptr>(symTableEntryForSignal.value()));
    } else {
        // TODO: GEN_ERROR, this should not happen but check anyways
        createError("TODO");
    }

    return valueOfLoopVariable;
}

std::any SyReCCustomVisitor::visitNumberFromExpression(SyReCParser::NumberFromExpressionContext* context) {
    const auto lhsOperand = context->lhsOperand != nullptr ? tryConvertProductionReturnValue<syrec::Number::ptr>(visit(context->lhsOperand)) : std::nullopt;

    const auto operation  = getDefinedOperation(context->op);
    if (!operation.has_value()) {
        createError(InvalidOrMissingNumberExpressionOperation);
    }

    const auto rhsOperand = context->rhsOperand != nullptr ? tryConvertProductionReturnValue<syrec::Number::ptr > (visit(context->rhsOperand)) : std::nullopt;
        if (!lhsOperand.has_value() || !operation.has_value() || !rhsOperand.has_value()) {
        return std::nullopt;    
    }

    const auto lhsOperandEvaluated = evaluateNumber(lhsOperand.value());
    const auto rhsOperandEvaluated = evaluateNumber(rhsOperand.value());

    if (lhsOperandEvaluated.has_value() && rhsOperandEvaluated.has_value()) {
        const auto binaryOperationResult = applyBinaryOperation(operation.value(), lhsOperandEvaluated.value(), rhsOperandEvaluated.value());
        if (binaryOperationResult.has_value()) {
            const auto resultContainer = std::make_shared<syrec::Number>(binaryOperationResult.value());
            return std::make_optional<syrec::Number::ptr>(resultContainer);
        }
    }

    return std::nullopt;
}


/*
 * Expression production visitors
 */

std::any SyReCCustomVisitor::visitExpressionFromNumber(SyReCParser::ExpressionFromNumberContext* context) {
    const auto definedNumber = context->number() != nullptr ? tryConvertProductionReturnValue<syrec::Number::ptr>(visit(context->number())) : std::nullopt;
    if (definedNumber.has_value()) {
        // TODO: Is this the correct calculation and is it both safe and does it return the expected result
        const auto valueOfDefinedNumber = definedNumber.value()->evaluate(loopVariableMappingLookup);
        const auto numericExpression = std::make_shared<syrec::NumericExpression>(definedNumber.value(), ExpressionEvaluationResult::getRequiredBitWidthToStoreSignal(valueOfDefinedNumber));
        return std::make_optional(ExpressionEvaluationResult::createFromExpression(numericExpression));
    }

    return std::nullopt;
}

std::any SyReCCustomVisitor::visitExpressionFromSignal(SyReCParser::ExpressionFromSignalContext* context) {
    std::optional<ExpressionEvaluationResult::ptr> expressionFromSignal;

    const auto definedSignal = context->signal() != nullptr ? tryConvertProductionReturnValue<SignalEvaluationResult::ptr>(visit(context->signal())) : std::nullopt;
    if (!definedSignal.has_value() || !definedSignal.value()->isValid()) {
        return expressionFromSignal;   
    }

    syrec::expression::ptr expressionWrapperForSignal;
    if (definedSignal.value()->isVariableAccess()) {
        expressionWrapperForSignal = std::make_shared<syrec::VariableExpression>(definedSignal.value()->getAsVariableAccess().value());
        expressionFromSignal.emplace(ExpressionEvaluationResult::createFromExpression(expressionWrapperForSignal));
    } else if (definedSignal.value()->isConstant()) {
        // TODO: Is this the correct calculation and is it both safe and does it return the expected result
        const auto constantValueOfDefinedSignal = definedSignal.value()->getAsNumber().value()->evaluate(loopVariableMappingLookup);
        expressionWrapperForSignal = std::make_shared<syrec::NumericExpression>(definedSignal.value()->getAsNumber().value(), ExpressionEvaluationResult::getRequiredBitWidthToStoreSignal(constantValueOfDefinedSignal));
        expressionFromSignal.emplace(ExpressionEvaluationResult::createFromExpression(expressionWrapperForSignal));
    }

    return expressionFromSignal;
}

std::any SyReCCustomVisitor::visitBinaryExpression(SyReCParser::BinaryExpressionContext* context) {
    const auto lhsOperand = context->lhsOperand != nullptr ? tryConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(visit(context->lhsOperand)) : std::nullopt;
    const auto userDefinedOperation = getDefinedOperation(context->binaryOperation);
    if (!userDefinedOperation.has_value() || !isValidBinaryOperation(userDefinedOperation.value())) {
        createError(InvalidBinaryOperation);
    }

    const auto rhsOperand = context->rhsOperand != nullptr ? tryConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->rhsOperand) : std::nullopt;

    if (!lhsOperand.has_value() || !userDefinedOperation.has_value() || !rhsOperand.has_value()) {
        return std::nullopt;
    }

    const auto lhsOperandConversionToConstant = lhsOperand.value()->getAsConstant();
    const auto rhsOperandConversionToConstant = rhsOperand.value()->getAsConstant();

    std::optional<syrec::expression::ptr> finalBinaryExpression;
    if (lhsOperandConversionToConstant.has_value() && rhsOperandConversionToConstant.has_value()) {
        const auto binaryExpressionEvaluated = applyBinaryOperation(userDefinedOperation.value(), *lhsOperandConversionToConstant, *rhsOperandConversionToConstant);
        if (binaryExpressionEvaluated.has_value()) {
            // TODO
            return ExpressionEvaluationResult::createFromConstantValue(binaryExpressionEvaluated.value());
        }
        else {
            // TODO: Error creation
            createError("TODO: Calculation of binary expression for constant values failed");
            return std::nullopt;
        }
    } else {
        const auto lhsOperandAsExpression = lhsOperand.value()->getAsExpression().value();
        const auto rhsOperandAsExpression = rhsOperand.value()->getAsExpression().value();
        
        // TODO: Add mapping from enum to internal "operation" flag value
        return std::make_optional(ExpressionEvaluationResult::createFromExpression(std::make_shared<syrec::BinaryExpression>(lhsOperandAsExpression, 0, rhsOperandAsExpression)));
    }
}

std::any SyReCCustomVisitor::visitUnaryExpression(SyReCParser::UnaryExpressionContext* context) {
    const auto userDefinedOperation = getDefinedOperation(context->unaryOperation);
    if (!userDefinedOperation.has_value() || (* userDefinedOperation != syrec_operation::operation::bitwise_negation && *userDefinedOperation != syrec_operation::operation::logical_negation)) { 
        createError(InvalidUnaryOperation);
    }
    
    const auto userDefinedExpression = context->expression() != nullptr ? tryConvertProductionReturnValue<ExpressionEvaluationResult>(visit(context->expression())) : std::nullopt;
    return std::nullopt;
}

std::any SyReCCustomVisitor::visitShiftExpression(SyReCParser::ShiftExpressionContext* context) {
    std::optional<ExpressionEvaluationResult::ptr> parsedShiftExpression;

    const auto expressionToShift = context->expression() != nullptr ? tryConvertProductionReturnValue<ExpressionEvaluationResult>(visit(context->expression())) : std::nullopt;
    const auto definedShiftOperation    = context->shiftOperation != nullptr ? getDefinedOperation(context->shiftOperation) : std::nullopt;

    if (!definedShiftOperation.has_value() || (definedShiftOperation.value() != syrec_operation::operation::shift_left && definedShiftOperation.value() != syrec_operation::operation::shift_right)) {
        createError(InvalidShiftOperation);
    }

    const auto shiftAmount = context->number() != nullptr ? tryConvertProductionReturnValue<syrec::Number::ptr>(visit(context->number())) : std::nullopt;

    if (!expressionToShift.has_value() || !definedShiftOperation.has_value() || !shiftAmount.has_value()) {
        return parsedShiftExpression;
    }

    const auto shiftAmountEvaluated = evaluateNumber(shiftAmount.value());
    const auto expressionToShiftEvaluated = expressionToShift->getAsConstant();

    if (shiftAmountEvaluated.has_value() && expressionToShiftEvaluated.has_value()) {
        const auto shiftOperationApplicationResult = apply(definedShiftOperation.value(), expressionToShiftEvaluated.value(), shiftAmountEvaluated.value());
        if (shiftOperationApplicationResult.has_value()) {
            parsedShiftExpression.emplace(ExpressionEvaluationResult::createFromConstantValue(shiftOperationApplicationResult.value()));
        } else {
            // TODO: GEN_ERROR
            createError("TODO: SHIFT CALC ERROR");
            return std::nullopt;
        }
    } else {
        // TODO: Mapping from operation to internal operation 'integer' value
        const auto createdShiftExpression = std::make_shared<syrec::ShiftExpression>(expressionToShift->getAsExpression().value(), 0, shiftAmount.value());
        parsedShiftExpression.emplace(ExpressionEvaluationResult::createFromExpression(createdShiftExpression));
    }

    return parsedShiftExpression;
}

/*
 * Utility functions for error and warning creation
 */

// TODO:
void SyReCCustomVisitor::createError(size_t line, size_t column, const std::string& errorMessage) {
    this->errors.emplace_back(fmt::format(messageFormat, line, column, errorMessage));
}

// TODO:
void SyReCCustomVisitor::createError(const std::string& errorMessage) {
    createError(0, 0, errorMessage);
}

// TODO:
void SyReCCustomVisitor::createWarning(const std::string& warningMessage) {
    createWarning(0, 0, warningMessage);
}

// TODO:
void SyReCCustomVisitor::createWarning(size_t line, size_t column, const std::string& warningMessage) {
    this->warnings.emplace_back(fmt::format(messageFormat, line, column, warningMessage));
}

// TODO:
std::optional<unsigned> SyReCCustomVisitor::convertToNumber(const antlr4::Token* token) const {
    if (token == nullptr) {
        return std::nullopt;
    }

    return convertToNumber(token->getText());
}

std::optional<unsigned int> SyReCCustomVisitor::convertToNumber(const std::string& tokenText) const {
    try {
        return std::stoul(tokenText) & UINT_MAX;
    } catch (std::invalid_argument&) {
        return std::nullopt;
    } catch (std::out_of_range&) {
        return std::nullopt;
    }
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
            createError(InvalidParameterType);
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
            break;
    }
    return signalType;
}

/*
 * Utility functions performing semantic checks, can be maybe refactoring into separate class
 */

bool SyReCCustomVisitor::checkIfSignalWasDeclaredOrLogError(const std::string_view& signalIdent) {
    if (!currentSymbolTableScope->contains(signalIdent)) {
        createError(fmt::format(UndeclaredIdent, signalIdent));
        return false;
    }
    return true;
}

[[nodiscard]] bool SyReCCustomVisitor::validateSemanticChecksIfDimensionExpressionIsConstant(const size_t accessedDimensionIdx, const syrec::Variable::ptr& accessedSignal, const std::optional<ExpressionEvaluationResult::ptr>& expressionEvaluationResult) {
    if (!expressionEvaluationResult.has_value()) {
        return false;
    }

    if (!expressionEvaluationResult.value()->isConstantValue()) {
        return true;
    }

    const auto expressionResultAsConstant = expressionEvaluationResult.value()->getAsConstant().value();
    const bool accessOnCurrentDimensionOk = isValidDimensionAccess(accessedSignal, accessedDimensionIdx, expressionResultAsConstant, true);
    if (!accessOnCurrentDimensionOk) {
        // TODO: Using global flag indicating zero_based indexing or not
        const auto constraintForCurrentDimension = getConstraintsForValidDimensionAccess(accessedSignal, accessedDimensionIdx, true).value();

        // TODO: GEN_ERROR: Index out of range for dimension i
        createError(
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

    const auto bitRangeStartEvaluated = tryConvertProductionReturnValue<syrec::Number::ptr>(visit(bitRangeStart));
    const auto bitRangeEndEvaluated = bitRangeEnd != nullptr ? tryConvertProductionReturnValue<syrec::Number::ptr>(visit(bitRangeEnd)) : bitRangeStartEvaluated;

    return (bitRangeStartEvaluated.has_value() && bitRangeEndEvaluated.has_value()) ? std::make_optional(std::make_pair(bitRangeStartEvaluated.value(), bitRangeEndEvaluated.value())) : std::nullopt;
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
        case SyReCParser::OP_DIVISION:
            definedOperation.emplace(syrec_operation::operation::division);
            break;
        default:
            createError("TODO: No mapping for operation");
            break;
    }
    return definedOperation;
}

// TODO: Naming
[[nodiscard]] std::optional<unsigned int> SyReCCustomVisitor::evaluateNumber(const syrec::Number::ptr& numberContainer) {
    if (numberContainer->isLoopVariable()) {
        const std::string& loopVariableIdent = numberContainer->variableName();
        if (loopVariableMappingLookup.find(loopVariableIdent) == loopVariableMappingLookup.end()) {
            createError(fmt::format(NoMappingForLoopVariable, loopVariableIdent));
            return std::nullopt;
        }
    }

    return std::make_optional(numberContainer->evaluate(loopVariableMappingLookup));
}

[[nodiscard]] std::optional<unsigned int> SyReCCustomVisitor::applyBinaryOperation(const syrec_operation::operation operation, const unsigned int leftOperand, const unsigned int rightOperand) {
    if (operation == syrec_operation::operation::division && rightOperand == 0) {
        // TODO: GEN_ERROR
        createError(DivisionByZero);
        return std::nullopt;
    } else {
        return apply(operation, leftOperand, rightOperand);
    }
}

bool SyReCCustomVisitor::isValidBinaryOperation(syrec_operation::operation userDefinedOperation) const {
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