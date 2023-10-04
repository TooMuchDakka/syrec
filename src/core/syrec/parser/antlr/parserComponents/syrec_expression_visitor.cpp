#include "core/syrec/parser/antlr/parserComponents/syrec_expression_visitor.hpp"

#include "core/syrec/parser/custom_semantic_errors.hpp"
#include "core/syrec/parser/expression_evaluation_result.hpp"
#include "core/syrec/parser/signal_evaluation_result.hpp"
#include "core/syrec/parser/value_broadcaster.hpp"
#include "core/syrec/parser/utils/bit_helpers.hpp"

using namespace parser;

/*
* Expression production visitors
*/
std::any SyReCExpressionVisitor::visitExpressionFromNumber(SyReCParser::ExpressionFromNumberContext* context)
{
    const auto userDefinedNumberContainer = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->number());
    if (!userDefinedNumberContainer) {
        return std::nullopt;
    }

    const auto& potentialErrorPositionForNumberEvaluation = determineContextStartTokenPositionOrUseDefaultOne(context->number());
    if (const auto& evaluationResultOfUserDefinedNumber = tryEvaluateNumber(**userDefinedNumberContainer, &potentialErrorPositionForNumberEvaluation); evaluationResultOfUserDefinedNumber.has_value()) {
        return std::make_optional(ExpressionEvaluationResult::createFromConstantValue(*evaluationResultOfUserDefinedNumber, sharedData->optionalExpectedExpressionSignalWidth));
    }   
    
    /*
     * TODO: Determine correct bitwidth of number
     * Problem: Value of loop variables is potentially not known => bit width of expression is not known
     * Solution:
     * I. Add lookup for min/max value of loop variable (that also includes the defined step size)
     * II. Evaluate compile time expression and determine value range to determine required bits to store potential value of range
     *
     */
    const auto bitWidthOfDefinedNumber = sharedData->optionalExpectedExpressionSignalWidth.value_or(1);
    const auto& exprContainerForNumber  = std::make_shared<syrec::NumericExpression>(*userDefinedNumberContainer, bitWidthOfDefinedNumber);
    return std::make_optional(ExpressionEvaluationResult::createFromExpression(exprContainerForNumber, {1}));
}

std::any SyReCExpressionVisitor::visitExpressionFromSignal(SyReCParser::ExpressionFromSignalContext* context)
{
    const auto definedSignal = tryVisitAndConvertProductionReturnValue<SignalEvaluationResult::ptr>(context->signal());
    if (!definedSignal.has_value()) {
        return std::nullopt;
    }

    if (const auto& userDefinedAccessedSignalParts = definedSignal->get()->getAsVariableAccess(); userDefinedAccessedSignalParts.has_value()) {
        const auto& containerForAccessedSignalParts = std::make_shared<syrec::VariableExpression>(*userDefinedAccessedSignalParts);
        // TODO: Size after access
        // TODO: explicitly accessed values per dimension
        return std::make_optional(ExpressionEvaluationResult::createFromExpression(containerForAccessedSignalParts, determineAccessedValuePerDimensionFromSignalAccess(**userDefinedAccessedSignalParts)));
    }
    if (const auto& simplifiedToNumberContainer = definedSignal->get()->getAsNumber(); simplifiedToNumberContainer.has_value()) {
        const auto& potentialErrorPositionForNumberEvaluation = determineContextStartTokenPositionOrUseDefaultOne(context->signal());
        if (const auto& compileTimeConstantValueOfNumber = tryEvaluateNumber(**simplifiedToNumberContainer, &potentialErrorPositionForNumberEvaluation); compileTimeConstantValueOfNumber.has_value()) {
            const auto& requiredBitsToStoreConstantValue = static_cast<unsigned int>(BitHelpers::getRequiredBitsToStoreValue(*compileTimeConstantValueOfNumber));
            return std::make_optional(ExpressionEvaluationResult::createFromConstantValue(*compileTimeConstantValueOfNumber, std::make_optional(requiredBitsToStoreConstantValue)));
        }

        const auto exprContainerForUserDefinedNumber = std::make_shared<syrec::NumericExpression>(*simplifiedToNumberContainer, sharedData->optionalExpectedExpressionSignalWidth.value_or(1));
        return std::make_optional(ExpressionEvaluationResult::createFromExpression(exprContainerForUserDefinedNumber, {1}));
    }
    return std::nullopt;
}

std::any SyReCExpressionVisitor::visitBinaryExpression(SyReCParser::BinaryExpressionContext* context)
{
    const auto& lhsOperand           = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->lhsOperand);
    const auto userDefinedOperation = getDefinedOperation(context->binaryOperation);
    if (!userDefinedOperation.has_value() || !isValidBinaryOperation(userDefinedOperation.value())) {
        createMessage(mapAntlrTokenPosition(context->binaryOperation), messageUtils::Message::Severity::Error, InvalidBinaryOperation);
    }

    const auto isExpectedExpressionSignalWidthAlreadySet = sharedData->optionalExpectedExpressionSignalWidth.has_value();
    if (lhsOperand.has_value()) {
        if (const auto& bitWidthOfLhsOperand = tryDetermineExpressionBitwidth(**lhsOperand->get()->getAsExpression(), determineContextStartTokenPositionOrUseDefaultOne(context)); bitWidthOfLhsOperand.has_value() && !isExpectedExpressionSignalWidthAlreadySet) {
            sharedData->optionalExpectedExpressionSignalWidth = *bitWidthOfLhsOperand;
        }
    }

    const auto& rhsOperand = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->rhsOperand);
    if (!lhsOperand.has_value() || !userDefinedOperation.has_value() || !rhsOperand.has_value()) {
        if (!isExpectedExpressionSignalWidthAlreadySet && sharedData->optionalExpectedExpressionSignalWidth.has_value()) {
            sharedData->optionalExpectedExpressionSignalWidth.reset();
        }
        return std::nullopt;
    }

    std::optional<ExpressionEvaluationResult::ptr> generatedBinaryExpr;
    const auto& compileTimeConstantValueOfLhsOperand = lhsOperand->get()->getAsConstant();
    const auto& compileTimeConstantValueOfRhsOperand = rhsOperand->get()->getAsConstant();

    if (compileTimeConstantValueOfLhsOperand.has_value() && compileTimeConstantValueOfRhsOperand.has_value()) {
        if (const auto& evaluationResultOfBinaryOperation = applyBinaryOperation(*userDefinedOperation, *compileTimeConstantValueOfLhsOperand, *compileTimeConstantValueOfRhsOperand, determineContextStartTokenPositionOrUseDefaultOne(context->rhsOperand)); evaluationResultOfBinaryOperation.has_value()) {
            const auto& bitWidth = sharedData->optionalExpectedExpressionSignalWidth.value_or((*((*lhsOperand)->getAsExpression()))->bitwidth());
            generatedBinaryExpr =ExpressionEvaluationResult::createFromConstantValue(*evaluationResultOfBinaryOperation, bitWidth);
        }
        else {
            createMessageAtUnknownPosition(messageUtils::Message::Severity::Error, "TODO: Calculation of binary expression for constant values failed");    
        }
    } else {
        const auto& lhsOperandSizeInformation = lhsOperand->get()->determineOperandSize();
        const auto& rhsOperandSizeInformation = rhsOperand->get()->determineOperandSize();
        // TODO:
        const auto& broadcastingErrorPosition              = determineContextStartTokenPositionOrUseDefaultOne(context->lhsOperand);
        if (!doOperandsRequireBroadcastingBasedOnDimensionAccessAndLogError(
            lhsOperandSizeInformation.numDeclaredDimensionOfOperand, lhsOperandSizeInformation.explicitlyAccessedValuesPerDimension.size(), lhsOperandSizeInformation.determineNumAccessedValuesPerDimension(),
            rhsOperandSizeInformation.numDeclaredDimensionOfOperand, rhsOperandSizeInformation.explicitlyAccessedValuesPerDimension.size(), rhsOperandSizeInformation.determineNumAccessedValuesPerDimension(),
            &broadcastingErrorPosition, nullptr)) {
            const auto& lhsOperandAsExpression = lhsOperand->get()->getAsExpression();
            const auto& rhsOperandAsExpression = rhsOperand->get()->getAsExpression();
            const auto& containerForBinaryExpr = std::make_shared<syrec::BinaryExpression>(*lhsOperandAsExpression, *syrec_operation::tryMapBinaryOperationEnumToFlag(*userDefinedOperation), *rhsOperandAsExpression);
            // TODO: Size of result
            generatedBinaryExpr                = ExpressionEvaluationResult::createFromExpression(containerForBinaryExpr, lhsOperandSizeInformation.explicitlyAccessedValuesPerDimension);
        }
    }

    if (!isExpectedExpressionSignalWidthAlreadySet && sharedData->optionalExpectedExpressionSignalWidth.has_value()) {
        sharedData->optionalExpectedExpressionSignalWidth.reset();
    }
    return generatedBinaryExpr;
}

std::any SyReCExpressionVisitor::visitUnaryExpression(SyReCParser::UnaryExpressionContext* context)
{
    const auto userDefinedOperation = getDefinedOperation(context->unaryOperation);
    if (!userDefinedOperation.has_value() || (*userDefinedOperation != syrec_operation::operation::BitwiseNegation && *userDefinedOperation != syrec_operation::operation::LogicalNegation)) {
        createMessage(mapAntlrTokenPosition(context->unaryOperation), messageUtils::Message::Severity::Error, InvalidUnaryOperation);
        return std::nullopt;
    }

    auto userDefinedUnaryExpression = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult>(context->expression());
    userDefinedUnaryExpression.reset();
    return userDefinedUnaryExpression;
}

std::any SyReCExpressionVisitor::visitShiftExpression(SyReCParser::ShiftExpressionContext* context)
{
    const auto expressionToShift     = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->expression());
    const auto definedShiftOperation = getDefinedOperation(context->shiftOperation);

    if (!definedShiftOperation.has_value() || (*definedShiftOperation != syrec_operation::operation::ShiftLeft && *definedShiftOperation != syrec_operation::operation::ShiftRight)) {
        createMessage(mapAntlrTokenPosition(context->shiftOperation), messageUtils::Message::Severity::Error, InvalidShiftOperation);
    }

    const auto shiftAmount = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->number());
    const auto& broadcastingErrorPosition = determineContextStartTokenPositionOrUseDefaultOne(context->expression());

    if (!expressionToShift.has_value() || !shiftAmount.has_value()) {
        return std::nullopt;
    }

    const auto& exprToShiftSizeInformation = expressionToShift->get()->determineOperandSize(); 
    if (doOperandsRequireBroadcastingBasedOnDimensionAccessAndLogError(
            exprToShiftSizeInformation.numDeclaredDimensionOfOperand, exprToShiftSizeInformation.explicitlyAccessedValuesPerDimension.size(), exprToShiftSizeInformation.determineNumAccessedValuesPerDimension(),
                1, 1, {1}, &broadcastingErrorPosition, nullptr)) {
        return std::nullopt;
    }

    const auto& potentialErrorPositionForShiftAmountEvaluation = determineContextStartTokenPositionOrUseDefaultOne(context->number());
    const auto  shiftAmountEvaluated                           = tryEvaluateNumber(**shiftAmount, &potentialErrorPositionForShiftAmountEvaluation);
    if (const auto& expressionToShiftEvaluated = expressionToShift->get()->getAsConstant(); expressionToShiftEvaluated.has_value() && shiftAmountEvaluated.has_value()) {
        if (const auto& shiftOperationEvaluationResult = syrec_operation::apply(*definedShiftOperation, *expressionToShiftEvaluated, *shiftAmountEvaluated); shiftOperationEvaluationResult.has_value()) {
            return std::make_optional(ExpressionEvaluationResult::createFromConstantValue(*shiftOperationEvaluationResult, std::nullopt));
        }
        return std::nullopt;
    }

    const auto& lhsOperandAsExpr = *expressionToShift->get()->getAsExpression();
    const auto& shiftExprContainer = std::make_shared<syrec::ShiftExpression>(lhsOperandAsExpr, *syrec_operation::tryMapShiftOperationEnumToFlag(*definedShiftOperation), *shiftAmount);
    // TODO: Size after shift
    return std::make_optional(ExpressionEvaluationResult::createFromExpression(shiftExprContainer, exprToShiftSizeInformation.explicitlyAccessedValuesPerDimension));
}

bool SyReCExpressionVisitor::isValidBinaryOperation(syrec_operation::operation userDefinedOperation) noexcept {
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
