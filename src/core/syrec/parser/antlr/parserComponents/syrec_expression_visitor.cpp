#include "core/syrec/parser/antlr/parserComponents/syrec_expression_visitor.hpp"
#include <fmt/format.h>

#include "core/syrec/parser/custom_semantic_errors.hpp"
#include "core/syrec/parser/expression_evaluation_result.hpp"
#include "core/syrec/parser/parser_utilities.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/signal_evaluation_result.hpp"
#include "core/syrec/parser/value_broadcaster.hpp"
#include "core/syrec/parser/utils/bit_helpers.hpp"

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
            bitWidthOfSignal                        = static_cast<unsigned int>(BitHelpers::getRequiredBitsToStoreValue(constantSignalValueEvaluated));
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

                syrec::BinaryExpression::ptr createdBinaryExpr = std::make_shared<syrec::BinaryExpression>(*lhsOperandAsExpression, *syrec_operation::tryMapBinaryOperationEnumToFlag(*userDefinedOperation), *rhsOperandAsExpression);
                if (const auto typecastedBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(createdBinaryExpr); typecastedBinaryExpr != nullptr && sharedData->optionalMultiplicationSimplifier.has_value()) {
                    const auto exprWithPotentiallySimplifiedMultiplications = sharedData->optionalMultiplicationSimplifier.value()->trySimplify(typecastedBinaryExpr);
                    if (exprWithPotentiallySimplifiedMultiplications.has_value()) {
                        createdBinaryExpr = *exprWithPotentiallySimplifiedMultiplications;
                    }
                }

                if (sharedData->parserConfig->reassociateExpressionsEnabled) {
                    createdBinaryExpr = optimizations::simplifyBinaryExpression(createdBinaryExpr, sharedData->parserConfig->operationStrengthReductionEnabled, sharedData->optionalMultiplicationSimplifier);
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
    if (!userDefinedOperation.has_value() || (*userDefinedOperation != syrec_operation::operation::BitwiseNegation && *userDefinedOperation != syrec_operation::operation::LogicalNegation)) {
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

    if (!definedShiftOperation.has_value() || (*definedShiftOperation != syrec_operation::operation::ShiftLeft && *definedShiftOperation != syrec_operation::operation::ShiftRight)) {
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
    syrec::expression::ptr createdShiftExpression = std::make_shared<syrec::ShiftExpression>(*(*expressionToShift)->getAsExpression(), *syrec_operation::tryMapShiftOperationEnumToFlag(*definedShiftOperation), *shiftAmount);
    if (sharedData->parserConfig->reassociateExpressionsEnabled) {
        createdShiftExpression = optimizations::simplifyBinaryExpression(createdShiftExpression, sharedData->parserConfig->operationStrengthReductionEnabled, sharedData->optionalMultiplicationSimplifier);
    }
    return std::make_optional(ExpressionEvaluationResult::createFromExpression(createdShiftExpression, numValuesPerDimensionOfExpressiontoShift));
}

bool SyReCExpressionVisitor::isValidBinaryOperation(syrec_operation::operation userDefinedOperation) {
    switch (userDefinedOperation) {
        case syrec_operation::operation::Addition:
        case syrec_operation::operation::Subtraction:
        case syrec_operation::operation::BitwiseXor:
        case syrec_operation::operation::Multiplication:
        case syrec_operation::operation::Division:
        case syrec_operation::operation::Modulo:
        case syrec_operation::operation::UpperBitsMultiplication:
        case syrec_operation::operation::LogicalAnd:
        case syrec_operation::operation::LogicalOr:
        case syrec_operation::operation::BitwiseAnd:
        case syrec_operation::operation::BitwiseOr:
        case syrec_operation::operation::LessThan:
        case syrec_operation::operation::GreaterThan:
        case syrec_operation::operation::Equals:
        case syrec_operation::operation::NotEquals:
        case syrec_operation::operation::LessEquals:
        case syrec_operation::operation::GreaterEquals:
            return true;
        default:
            return false;
    }
}