#include "core/syrec/parser/antlr/parserComponents/syrec_expression_visitor.hpp"
#include <fmt/format.h>

#include "core/syrec/parser/custom_semantic_errors.hpp"
#include "core/syrec/parser/expression_evaluation_result.hpp"
#include "core/syrec/parser/parser_utilities.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/signal_evaluation_result.hpp"
#include "core/syrec/parser/value_broadcaster.hpp"

#include "core/syrec/parser/optimizations/reassociate_expression.hpp"

using namespace parser;

/*
* Expression production visitors
*/
std::any SyReCExpressionVisitor::visitExpressionFromNumber(SyReCParser::ExpressionFromNumberContext* context)
{
    const auto definedNumber = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->number());
    if (definedNumber.has_value()) {
        if (canEvaluateNumber(*definedNumber)) {
            const auto& valueOfNumberEvaluated = tryEvaluateNumber(*definedNumber, determineContextStartTokenPositionOrUseDefaultOne(context->number()));
            return std::make_optional(ExpressionEvaluationResult::createFromConstantValue(*valueOfNumberEvaluated, sharedData->optionalExpectedExpressionSignalWidth));
        }

        // TODO: Determine correct bitwidth of number
        /*
         * Problem: Value of loop variables is potentially not known => bit width of expression is not known
         * Solution:
         * I. Add lookup for min/max value of loop variable (that also includes the defined step size)
         * II. Evaluate compile time expression and determine value range to determine required bits to store potential value of range
         *
         */
        const unsigned int bitWidthOfNumber         = 1;
        const auto&        containerForLoopVariable = std::make_shared<syrec::NumericExpression>(*definedNumber, bitWidthOfNumber);
        return std::make_optional(ExpressionEvaluationResult::createFromExpression(containerForLoopVariable, {1}));
    }

    return std::nullopt;
}

std::any SyReCExpressionVisitor::visitExpressionFromSignal(SyReCParser::ExpressionFromSignalContext* context)
{
    std::optional<ExpressionEvaluationResult::ptr> expressionFromSignal;

    const auto definedSignal = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->signal());
    if (!definedSignal.has_value()) {
        return expressionFromSignal;
    }

    syrec::expression::ptr expressionWrapperForSignal;
    if ((*definedSignal)->isVariableAccess()) {
        const auto& accessOnDefinedSignal = *(*definedSignal)->getAsVariableAccess();
        expressionWrapperForSignal        = std::make_shared<syrec::VariableExpression>(accessOnDefinedSignal);
        // TODO: getSizeOfSignalAccess handling of CompileTimeConstantExpression
        expressionFromSignal.emplace(ExpressionEvaluationResult::createFromExpression(expressionWrapperForSignal, getSizeOfSignalAfterAccess(accessOnDefinedSignal)));
    } else if ((*definedSignal)->isConstant()) {
        // TODO: Is this the correct calculation and is it both safe and does it return the expected result
        // TODO: Correct handling of loop variable
        auto constantSignalValue = *(*definedSignal)->getAsNumber();

        unsigned int bitWidthOfSignal = 0;
        if (canEvaluateNumber(constantSignalValue)) {
            const auto constantSignalValueEvaluated = *tryEvaluateNumber(constantSignalValue, determineContextStartTokenPositionOrUseDefaultOne(context->signal()));
            bitWidthOfSignal                        = ExpressionEvaluationResult::getRequiredBitWidthToStoreSignal(constantSignalValueEvaluated);
            constantSignalValue                     = std::make_shared<syrec::Number>(constantSignalValueEvaluated);
            return std::make_optional(ExpressionEvaluationResult::createFromConstantValue(constantSignalValueEvaluated, std::make_optional(bitWidthOfSignal)));
        }

        expressionWrapperForSignal = std::make_shared<syrec::NumericExpression>(constantSignalValue, bitWidthOfSignal);
        expressionFromSignal.emplace(ExpressionEvaluationResult::createFromExpression(expressionWrapperForSignal, {1}));
    }

    return expressionFromSignal;
}

std::any SyReCExpressionVisitor::visitBinaryExpression(SyReCParser::BinaryExpressionContext* context)
{
    const auto lhsOperand           = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->lhsOperand);
    const auto userDefinedOperation = getDefinedOperation(context->binaryOperation);
    if (!userDefinedOperation.has_value() || !isValidBinaryOperation(userDefinedOperation.value())) {
        createError(mapAntlrTokenPosition(context->binaryOperation), InvalidBinaryOperation);
    }

    // TODO:
    bool modifiedExpectedExpressionSignalWidth = false;
    /*if (!optionalExpectedExpressionSignalWidth.has_value() && lhsOperand.has_value()) {
        optionalExpectedExpressionSignalWidth = tryDetermineExpressionBitwidth(*(*lhsOperand)->getAsExpression(), determineContextStartTokenPositionOrUseDefaultOne(context));
        modifiedExpectedExpressionSignalWidth = true;
    }*/

    if (lhsOperand.has_value()) {
        const auto bitWidthOfLhsOperand = tryDetermineExpressionBitwidth(*(*lhsOperand)->getAsExpression(), determineContextStartTokenPositionOrUseDefaultOne(context));
        if (bitWidthOfLhsOperand.has_value()) {
            sharedData->optionalExpectedExpressionSignalWidth.emplace(*bitWidthOfLhsOperand);
            modifiedExpectedExpressionSignalWidth = true;
        }
    }

    std::optional<ExpressionEvaluationResult::ptr> rhsOperand = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->rhsOperand);
    std::optional<ExpressionEvaluationResult::ptr> result = std::nullopt;
    if (lhsOperand.has_value() && userDefinedOperation.has_value() && rhsOperand.has_value()) {
        const auto lhsOperandConversionToConstant = (*lhsOperand)->getAsConstant();
        const auto rhsOperandConversionToConstant = (*rhsOperand)->getAsConstant();

        if (lhsOperandConversionToConstant.has_value() && rhsOperandConversionToConstant.has_value()) {
            const auto binaryExpressionEvaluated = applyBinaryOperation(*userDefinedOperation, *lhsOperandConversionToConstant, *rhsOperandConversionToConstant, determineContextStartTokenPositionOrUseDefaultOne(context->rhsOperand));
            if (binaryExpressionEvaluated.has_value()) {
                result.emplace(ExpressionEvaluationResult::createFromConstantValue(*binaryExpressionEvaluated, (*(*lhsOperand)->getAsExpression())->bitwidth()));
            } else {
                // TODO: Error creation
                // TODO: Error position
                createError(TokenPosition::createFallbackPosition(), "TODO: Calculation of binary expression for constant values failed");
            }
        } else {
            const auto lhsOperandNumValuesPerDimension = (*lhsOperand)->numValuesPerDimension;
            const auto rhsOperandNumValuesPerDimension = (*rhsOperand)->numValuesPerDimension;

            // TODO: Can the check and the error creation be refactored into one function
            // TODO: Handling of broadcasting
            if (requiresBroadcasting(lhsOperandNumValuesPerDimension, rhsOperandNumValuesPerDimension)) {
                if (!sharedData->parserConfig->supportBroadcastingExpressionOperands) {
                    createError(determineContextStartTokenPositionOrUseDefaultOne(context->lhsOperand), fmt::format(MissmatchedNumberOfDimensionsBetweenOperands, lhsOperandNumValuesPerDimension.size(), rhsOperandNumValuesPerDimension.size()));
                } else {
                    createError(determineContextStartTokenPositionOrUseDefaultOne(context->lhsOperand), "Broadcasting of operands is currently not supported");
                }
            } else if (checkIfNumberOfValuesPerDimensionMatchOrLogError(
                               determineContextStartTokenPositionOrUseDefaultOne(context->lhsOperand),
                               lhsOperandNumValuesPerDimension,
                               rhsOperandNumValuesPerDimension)) {
                const auto lhsOperandAsExpression = (*lhsOperand)->getAsExpression();
                const auto rhsOperandAsExpression = (*rhsOperand)->getAsExpression();
                // TODO: Call to bitwidth does not work with loop variables here
                //const auto binaryExpressionBitwidth = std::max((*lhsOperandAsExpression)->bitwidth(), (*rhsOperandAsExpression)->bitwidth());

                syrec::BinaryExpression::ptr createdBinaryExpr = std::make_shared<syrec::BinaryExpression>(*lhsOperandAsExpression, *ParserUtilities::mapOperationToInternalFlag(*userDefinedOperation), *rhsOperandAsExpression);

                /*
                 * Since the synthesis without additional lines optimization could optimize a binary expression of the form e_left op e_right to e_left op = e_right
                 * and in case our binary expression is of the form e_left - constant or constant - e_right, we need to prevent the optimization of the operand of the expression
                 * that evaluated to a constant, in case a variable access was optimized per constant propagation, to prevent invalid assignment statements of the form constant -= e_left
                 * which cannot be corrected due to the missing commutative property of the subtraction operation.
                 *
                */
                if (sharedData->currentlyParsingAssignmentStmtRhs && sharedData->parserConfig->noAdditionalLineOptimizationEnabled 
                    && sharedData->parserConfig->performConstantPropagation 
                    && *userDefinedOperation == syrec_operation::operation::subtraction
                    && (lhsOperandConversionToConstant.has_value() || rhsOperandConversionToConstant.has_value())) {

                    const bool isLhsConstant = lhsOperandConversionToConstant.has_value();
                    const auto& notConstantOperandAsVariableAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(isLhsConstant ? *rhsOperandAsExpression : *lhsOperandAsExpression);

                    sharedData->parserConfig->performConstantPropagation            = false;
                    const auto constantOperandReevaluatedWithoutConstantPropagation = *tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(isLhsConstant ? context->lhsOperand : context->rhsOperand);
                    sharedData->parserConfig->performConstantPropagation            = true;

                    std::shared_ptr<syrec::VariableExpression> constantOperandReevaluatedAsVariableAccess = nullptr;
                    if (!constantOperandReevaluatedWithoutConstantPropagation->isConstantValue()) {
                        constantOperandReevaluatedAsVariableAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(*constantOperandReevaluatedWithoutConstantPropagation->getAsExpression());
                    }

                    if (notConstantOperandAsVariableAccess != nullptr && constantOperandReevaluatedAsVariableAccess != nullptr) {
                        createdBinaryExpr = std::make_shared<syrec::BinaryExpression>(notConstantOperandAsVariableAccess, *ParserUtilities::mapOperationToInternalFlag(*userDefinedOperation), constantOperandReevaluatedAsVariableAccess);   
                    }
                } else {
                    if (sharedData->parserConfig->reassociateExpressionsEnabled) {
                        createdBinaryExpr = optimizations::simplifyBinaryExpression(createdBinaryExpr, sharedData->parserConfig->operationStrengthReductionEnabled);
                    }    
                }

                // TODO: Handling of binary expression bitwidth, i.e. both operands will be "promoted" to a bitwidth of the larger expression
                result.emplace(ExpressionEvaluationResult::createFromExpression(createdBinaryExpr, (*lhsOperand)->numValuesPerDimension));
            }
        }
    }

    if (modifiedExpectedExpressionSignalWidth) {
        sharedData->optionalExpectedExpressionSignalWidth.reset();
    }

    return result;
}

std::any SyReCExpressionVisitor::visitUnaryExpression(SyReCParser::UnaryExpressionContext* context)
{
    const auto userDefinedOperation = getDefinedOperation(context->unaryOperation);
    if (!userDefinedOperation.has_value() || (*userDefinedOperation != syrec_operation::operation::bitwise_negation && *userDefinedOperation != syrec_operation::operation::logical_negation)) {
        createError(mapAntlrTokenPosition(context->unaryOperation), InvalidUnaryOperation);
    }

    const auto userDefinedExpression = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult>(context->expression());
    // TOD: Add either error or warning that unary expressions are not supported
    return std::nullopt;
}

std::any SyReCExpressionVisitor::visitShiftExpression(SyReCParser::ShiftExpressionContext* context)
{
    const auto expressionToShift     = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->expression());
    const auto definedShiftOperation = context->shiftOperation != nullptr ? getDefinedOperation(context->shiftOperation) : std::nullopt;

    if (!definedShiftOperation.has_value() || (*definedShiftOperation != syrec_operation::operation::shift_left && *definedShiftOperation != syrec_operation::operation::shift_right)) {
        createError(mapAntlrTokenPosition(context->shiftOperation), InvalidShiftOperation);
    }

    const auto shiftAmount = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->number());

    if (!expressionToShift.has_value() || !shiftAmount.has_value()) {
        return std::nullopt;
    }

    const auto numValuesPerDimensionOfExpressiontoShift = (*expressionToShift)->numValuesPerDimension;
    // TODO: Handling of broadcasting
    if (requiresBroadcasting(numValuesPerDimensionOfExpressiontoShift, {1})) {
        if (!sharedData->parserConfig->supportBroadcastingExpressionOperands) {
            createError(determineContextStartTokenPositionOrUseDefaultOne(context->expression()), fmt::format(MissmatchedNumberOfDimensionsBetweenOperands, numValuesPerDimensionOfExpressiontoShift.size(), 1));
        } else {
            createError(determineContextStartTokenPositionOrUseDefaultOne(context->expression()), "Broadcasting of operands is currently not supported");
        }
        return std::nullopt;
    }

    if (!checkIfNumberOfValuesPerDimensionMatchOrLogError(determineContextStartTokenPositionOrUseDefaultOne(context->expression()), numValuesPerDimensionOfExpressiontoShift, {1})) {
        return std::nullopt;
    }

    const auto shiftAmountEvaluated = tryEvaluateNumber(*shiftAmount, determineContextStartTokenPositionOrUseDefaultOne(context->number()));
    // TODO: getAsConstant(...) handling of compileTimeExpression ?
    const auto expressionToShiftEvaluated = (*expressionToShift)->getAsConstant();

    if (shiftAmountEvaluated.has_value() && expressionToShiftEvaluated.has_value()) {
        const auto shiftOperationApplicationResult = syrec_operation::apply(*definedShiftOperation, *expressionToShiftEvaluated, *shiftAmountEvaluated);
        if (shiftOperationApplicationResult.has_value()) {
            return std::make_optional(ExpressionEvaluationResult::createFromConstantValue(*shiftOperationApplicationResult, (*(*expressionToShift)->getAsExpression())->bitwidth()));
        }

        // TODO: GEN_ERROR
        // TODO: Error position
        createError(determineContextStartTokenPositionOrUseDefaultOne(context->expression()), "TODO: SHIFT CALC ERROR");
        return std::nullopt;
    }

    // TODO: Mapping from operation to internal operation 'integer' value
    const auto lhsShiftOperationOperandExpr = *(*expressionToShift)->getAsExpression();

    /* TODO: Refactor after loop variable lookup is fixed
    const auto rhsShiftOperationOperandBitwidth = ExpressionEvaluationResult::getRequiredBitWidthToStoreSignal((*shiftAmount)->evaluate(loopVariableMappingLookup));
    const auto shiftExpressionBitwidth          = std::max(lhsShiftOperationOperandExpr->bitwidth(), rhsShiftOperationOperandBitwidth);
    */

    // TODO: Handling of shift expression bitwidth, i.e. both operands will be "promoted" to a bitwidth of the larger expression
    syrec::expression::ptr createdShiftExpression = std::make_shared<syrec::ShiftExpression>(*(*expressionToShift)->getAsExpression(), *ParserUtilities::mapOperationToInternalFlag(*definedShiftOperation), *shiftAmount);
    if (sharedData->parserConfig->reassociateExpressionsEnabled) {
        createdShiftExpression = optimizations::simplifyBinaryExpression(createdShiftExpression, sharedData->parserConfig->operationStrengthReductionEnabled);
    }
    return std::make_optional(ExpressionEvaluationResult::createFromExpression(createdShiftExpression, numValuesPerDimensionOfExpressiontoShift));
}

bool SyReCExpressionVisitor::isValidBinaryOperation(syrec_operation::operation userDefinedOperation) {
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