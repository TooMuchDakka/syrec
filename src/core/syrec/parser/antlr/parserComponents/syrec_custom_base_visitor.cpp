#include "core/syrec/parser/antlr/parserComponents/syrec_custom_base_visitor.hpp"
#include <fmt/format.h>

#include "core/syrec/parser/custom_semantic_errors.hpp"
#include "core/syrec/parser/expression_evaluation_result.hpp"
#include "core/syrec/parser/infix_iterator.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/parser_utilities.hpp"
#include "core/syrec/parser/range_check.hpp"
#include "core/syrec/parser/signal_evaluation_result.hpp"
#include "core/syrec/parser/value_broadcaster.hpp"
#include "core/syrec/parser/optimizations/reassociate_expression.hpp"
#include "core/syrec/parser/utils/binary_expression_simplifier.hpp"
#include "core/syrec/parser/utils/signal_access_utils.hpp"

using namespace parser;

bool SyReCCustomBaseVisitor::checkIfSignalWasDeclaredOrLogError(const std::string& signalIdent, const bool isLoopVariable, const messageUtils::Message::Position& signalIdentTokenPosition) {
    const std::string signalIdentTransformed = isLoopVariable ? "$" + signalIdent : signalIdent;
    if (!sharedData->currentSymbolTableScope->contains(signalIdentTransformed)) {
        createMessage(signalIdentTokenPosition, messageUtils::Message::Severity::Error, UndeclaredIdent, signalIdentTransformed);
        return false;
    }
    return true;
}

bool SyReCCustomBaseVisitor::isSignalAssignableOtherwiseCreateError(const antlr4::Token* signalIdentToken, const syrec::VariableAccess::ptr& assignedToVariable) {
    if (syrec::Variable::Types::In == assignedToVariable->getVar()->type) {
        // TODO: Error position
        createMessage(mapAntlrTokenPosition(signalIdentToken), messageUtils::Message::Severity::Error, AssignmentToReadonlyVariable, assignedToVariable->getVar()->name);
        return false;
    }
    return true;
}

inline messageUtils::Message::Position SyReCCustomBaseVisitor::determineContextStartTokenPositionOrUseDefaultOne(const antlr4::ParserRuleContext* context) {
    return context == nullptr ? messageUtils::Message::Position(SharedVisitorData::fallbackErrorLinePosition, SharedVisitorData::fallbackErrorColumnPosition) : mapAntlrTokenPosition(context->start);
}

std::optional<syrec_operation::operation> SyReCCustomBaseVisitor::getDefinedOperation(const antlr4::Token* definedOperationToken) {
    if (definedOperationToken == nullptr) {
        return std::nullopt;
    }

    std::optional<syrec_operation::operation> definedOperation;
    switch (definedOperationToken->getType()) {
        case SyReCParser::OP_PLUS:
            definedOperation.emplace(syrec_operation::operation::Addition);
            break;
        case SyReCParser::OP_MINUS:
            definedOperation.emplace(syrec_operation::operation::Subtraction);
            break;
        case SyReCParser::OP_MULTIPLY:
            definedOperation.emplace(syrec_operation::operation::Multiplication);
            break;
        case SyReCParser::OP_UPPER_BIT_MULTIPLY:
            definedOperation.emplace(syrec_operation::operation::UpperBitsMultiplication);
            break;
        case SyReCParser::OP_DIVISION:
            definedOperation.emplace(syrec_operation::operation::Division);
            break;
        case SyReCParser::OP_MODULO:
            definedOperation.emplace(syrec_operation::operation::Modulo);
            break;
        case SyReCParser::OP_GREATER_THAN:
            definedOperation.emplace(syrec_operation::operation::GreaterThan);
            break;
        case SyReCParser::OP_GREATER_OR_EQUAL:
            definedOperation.emplace(syrec_operation::operation::GreaterEquals);
            break;
        case SyReCParser::OP_LESS_THAN:
            definedOperation.emplace(syrec_operation::operation::LessThan);
            break;
        case SyReCParser::OP_LESS_OR_EQUAL:
            definedOperation.emplace(syrec_operation::operation::LessEquals);
            break;
        case SyReCParser::OP_EQUAL:
            definedOperation.emplace(syrec_operation::operation::Equals);
            break;
        case SyReCParser::OP_NOT_EQUAL:
            definedOperation.emplace(syrec_operation::operation::NotEquals);
            break;
        case SyReCParser::OP_BITWISE_AND:
            definedOperation.emplace(syrec_operation::operation::BitwiseAnd);
            break;
        case SyReCParser::OP_BITWISE_NEGATION:
            definedOperation.emplace(syrec_operation::operation::BitwiseNegation);
            break;
        case SyReCParser::OP_BITWISE_OR:
            definedOperation.emplace(syrec_operation::operation::BitwiseOr);
            break;
        case SyReCParser::OP_BITWISE_XOR:
            definedOperation.emplace(syrec_operation::operation::BitwiseXor);
            break;
        case SyReCParser::OP_LOGICAL_AND:
            definedOperation.emplace(syrec_operation::operation::LogicalAnd);
            break;
        case SyReCParser::OP_LOGICAL_OR:
            definedOperation.emplace(syrec_operation::operation::LogicalOr);
            break;
        case SyReCParser::OP_LOGICAL_NEGATION:
            definedOperation.emplace(syrec_operation::operation::LogicalNegation);
            break;
        case SyReCParser::OP_LEFT_SHIFT:
            definedOperation.emplace(syrec_operation::operation::ShiftLeft);
            break;
        case SyReCParser::OP_RIGHT_SHIFT:
            definedOperation.emplace(syrec_operation::operation::ShiftRight);
            break;
        case SyReCParser::OP_INCREMENT_ASSIGN:
            definedOperation.emplace(syrec_operation::operation::IncrementAssign);
            break;
        case SyReCParser::OP_DECREMENT_ASSIGN:
            definedOperation.emplace(syrec_operation::operation::DecrementAssign);
            break;
        case SyReCParser::OP_INVERT_ASSIGN:
            definedOperation.emplace(syrec_operation::operation::InvertAssign);
            break;
        case SyReCParser::OP_ADD_ASSIGN:
            definedOperation.emplace(syrec_operation::operation::AddAssign);
            break;
        case SyReCParser::OP_SUB_ASSIGN:
            definedOperation.emplace(syrec_operation::operation::MinusAssign);
            break;
        case SyReCParser::OP_XOR_ASSIGN:
            definedOperation.emplace(syrec_operation::operation::XorAssign);
            break;
        default:
            // TODO: Error position
            createMessage(mapAntlrTokenPosition(definedOperationToken), messageUtils::Message::Severity::Error , "TODO: No mapping for operation");
            break;
    }
    return definedOperation;
}

inline messageUtils::Message::Position SyReCCustomBaseVisitor::mapAntlrTokenPosition(const antlr4::Token* token) {
    if (token == nullptr) {
        return messageUtils::Message::Position(SharedVisitorData::fallbackErrorLinePosition, SharedVisitorData::fallbackErrorColumnPosition);
    }
    return messageUtils::Message::Position(token->getLine(), token->getCharPositionInLine());
}

std::any SyReCCustomBaseVisitor::visitSignal(SyReCParser::SignalContext* context) {
    std::string                               signalIdent;
    std::optional<syrec::VariableAccess::ptr> accessedSignal;
    bool                                      isValidSignalAccess = context->IDENT() != nullptr && checkIfSignalWasDeclaredOrLogError(context->IDENT()->getText(), false, messageUtils::Message::Position(context->IDENT()->getSymbol()->getLine(), context->IDENT()->getSymbol()->getCharPositionInLine()));

    if (isValidSignalAccess) {
        signalIdent                  = context->IDENT()->getText();
        const auto signalSymTabEntry = sharedData->currentSymbolTableScope->getVariable(signalIdent);
        isValidSignalAccess          = signalSymTabEntry.has_value() && std::holds_alternative<syrec::Variable::ptr>(*signalSymTabEntry);

        if (isValidSignalAccess) {
            const syrec::VariableAccess::ptr container = std::make_shared<syrec::VariableAccess>();
            container->setVar(std::get<syrec::Variable::ptr>(*signalSymTabEntry));
            accessedSignal.emplace(container);
            updateReferenceCountOfSignal(signalIdent, Increment);
        }
    }

    // TODO: Handling of compile time expressions in semantic checks for dimension / bit / range access
    /*
     * Problem: Value of loop variables is potentially not known => accessed dimension / bit / range might not be known
     * Solution:
     * I. Add lookup for min/max value of loop variable (that also includes the defined step size)
     * II. Evaluate compile time expression and determine value range to determine required bits to store potential value of range
     * III. Check if potential value range lies outside of value one
     */

    const size_t numDimensionsOfAccessSignal = accessedSignal.has_value() ? (*accessedSignal)->getVar()->dimensions.size() : SIZE_MAX;
    const size_t numUserDefinedDimensions    = context->accessedDimensions.size();
    if (isValidSignalAccess) {
        const size_t numElementsWithinRange = std::min(numUserDefinedDimensions, numDimensionsOfAccessSignal);
        for (size_t dimensionIdx = 0; dimensionIdx < numElementsWithinRange; ++dimensionIdx) {
            const auto accessedDimensionExpression = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->accessedDimensions.at(dimensionIdx));
            isValidSignalAccess                    = validateSemanticChecksIfDimensionExpressionIsConstant(
                                       context->accessedDimensions.at(dimensionIdx)->start,
                                       dimensionIdx,
                                       (*accessedSignal)->getVar(),
                                       accessedDimensionExpression);

            if (isValidSignalAccess) {
                // TODO: Set correct bit width of expression
                (*accessedSignal)->indexes.emplace_back((*accessedDimensionExpression)->getAsExpression().value());
            }
        }

        isValidSignalAccess &= numUserDefinedDimensions <= numElementsWithinRange;
        for (size_t dimensionOutOfRangeIdx = numElementsWithinRange; dimensionOutOfRangeIdx < numUserDefinedDimensions; ++dimensionOutOfRangeIdx) {
            createMessage(determineContextStartTokenPositionOrUseDefaultOne(context->accessedDimensions.at(dimensionOutOfRangeIdx)), messageUtils::Message::Severity::Error, DimensionOutOfRange, dimensionOutOfRangeIdx, signalIdent, numDimensionsOfAccessSignal);
        }
    }

    const auto bitOrRangeAccessEvaluated = isBitOrRangeAccessDefined(context->bitStart, context->bitRangeEnd);
    if (accessedSignal.has_value() && bitOrRangeAccessEvaluated.has_value()) {
        const auto  accessedVariable     = (*accessedSignal)->getVar();
        const auto& bitOrRangeAccessPair = bitOrRangeAccessEvaluated.value();

        const bool isBitAccess = bitOrRangeAccessPair.first == bitOrRangeAccessPair.second;
        if (canEvaluateNumber(bitOrRangeAccessPair.first) && (!isBitAccess ? canEvaluateNumber(bitOrRangeAccessPair.second) : true)) {
            isValidSignalAccess = validateBitOrRangeAccessOnSignal(context->bitStart->start, accessedVariable, bitOrRangeAccessPair);
        }

        if (isValidSignalAccess) {
            accessedSignal.value()->range.emplace(bitOrRangeAccessPair);
        }
    }

    /*
     * TODO: Valid accesses
     *
     * a[0] += a[1]
     * a[0].5 += (a[0].6 + 2)
     * a[0].6:11 += (a[0].0:5 + 2)
     * a[2] += (a[0] + a[1])
     *
     * TODO: Prohibited accesses
     * a[0] += a[0]
     * a[i].0 += a[0].0
     * a[i].0:10 += (a[0].2:5 + 5)
     * a[0][i] += (a[0][2].2:10 + 10)
     * a[0] += (a[i].2:10 + a[i][0])
     *
     */

    if (isValidSignalAccess) {
        const auto& restrictionErrorPosition = determineContextStartTokenPositionOrUseDefaultOne(context);
        if (!sharedData->shouldSkipSignalAccessRestrictionCheck && isAccessToAccessedSignalPartRestricted(*accessedSignal, restrictionErrorPosition)) {
            // TODO: GEN_ERROR
            // TODO: Create correct error message
            createMessage(restrictionErrorPosition, messageUtils::Message::Severity::Error, AccessingRestrictedPartOfSignal, signalIdent);
        } else {
            if (const auto& fetchedValueByConstantPropagation = tryPerformConstantPropagationForSignalAccess(*accessedSignal); fetchedValueByConstantPropagation.has_value()) {
                updateReferenceCountOfSignal(accessedSignal.value()->var->name, Decrement);
                return std::make_optional(std::make_shared<SignalEvaluationResult>(std::make_shared<syrec::Number>(*fetchedValueByConstantPropagation)));
            }
            return std::make_optional(std::make_shared<SignalEvaluationResult>(*accessedSignal));
        }
    }
    return std::nullopt;
}

// TODO: GENERAL Check that defined number can be stored in data type used for numbers
std::any SyReCCustomBaseVisitor::visitNumberFromConstant(SyReCParser::NumberFromConstantContext* context) {
    if (context->INT() == nullptr) {
        return std::nullopt;
    }

    return std::make_optional(std::make_shared<syrec::Number>(convertToNumber(context->INT()->getText()).value()));
}

std::any SyReCCustomBaseVisitor::visitNumberFromSignalwidth(SyReCParser::NumberFromSignalwidthContext* context) {
    if (context->IDENT() == nullptr) {
        return std::nullopt;
    }

    const std::string signalIdent = context->IDENT()->getText();
    if (!checkIfSignalWasDeclaredOrLogError(
        context->IDENT()->getText(),
        false,
        messageUtils::Message::Position(context->IDENT()->getSymbol()->getLine(), context->IDENT()->getSymbol()->getCharPositionInLine()))) {
        return std::nullopt;
    }

    std::optional<syrec::Number::ptr> signalWidthOfSignal;
    const auto&                       symTableEntryForSignal = sharedData->currentSymbolTableScope->getVariable(signalIdent);
    if (symTableEntryForSignal.has_value() && std::holds_alternative<syrec::Variable::ptr>(*symTableEntryForSignal)) {
        signalWidthOfSignal.emplace(std::make_shared<syrec::Number>(std::get<syrec::Variable::ptr>(*symTableEntryForSignal)->bitwidth));
    } else {
        // TODO: GEN_ERROR, this should not happen
        // TODO: Error position
        createMessageAtUnknownPosition(messageUtils::Message::Severity::Error, "TODO");
    }

    return signalWidthOfSignal;
}

std::any SyReCCustomBaseVisitor::visitNumberFromExpression(SyReCParser::NumberFromExpressionContext* context) {
    const auto lhsOperand = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->lhsOperand);

    // TODO: What is the return value if an invalid number operation is defined, we know that a parsing error will be created
    const auto operation = getDefinedOperation(context->op);
    if (!operation.has_value()) {
        // TODO: Error position
        createMessage(mapAntlrTokenPosition(context->op), messageUtils::Message::Severity::Error, InvalidOrMissingNumberExpressionOperation);
    }

    const auto rhsOperand = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->rhsOperand);
    if (!lhsOperand.has_value() || !operation.has_value() || !rhsOperand.has_value()) {
        return std::nullopt;
    }

    /*
     * If one of the operands is either a loop variable that currently cannot be evaluated (i.e. because it is depends on a value that is currently not known
     * or is a compile time expression that can also not be evaluated because the condition above holds for one of its operands,
     * we skip evaluation and instead create a new expression
     */
    syrec::Number::CompileTimeConstantExpression::Operation mappedOperation;
    switch (*operation) {
        case syrec_operation::operation::Addition: {
            mappedOperation = syrec::Number::CompileTimeConstantExpression::Operation::Addition;
            break;
        }
        case syrec_operation::operation::Subtraction: {
            mappedOperation = syrec::Number::CompileTimeConstantExpression::Operation::Subtraction;
            break;
        }
        case syrec_operation::operation::Multiplication: {
            mappedOperation = syrec::Number::CompileTimeConstantExpression::Operation::Multiplication;
            break;
        }
        case syrec_operation::operation::Division: {
            mappedOperation = syrec::Number::CompileTimeConstantExpression::Operation::Division;
            break;
        }
        default:
            createMessage(mapAntlrTokenPosition(context->op), messageUtils::Message::Severity::Error, NoMappingForNumberOperation, context->op->getText());
            return std::nullopt;
    }

    if (!canEvaluateNumber(*lhsOperand) ||!canEvaluateNumber(*rhsOperand)) {
        const auto compileTimeConstantExpression = syrec::Number::CompileTimeConstantExpression(*lhsOperand, mappedOperation, *rhsOperand);
        if (sharedData->performingReadOnlyParsingOfLoopBody) {
            return std::make_optional(std::make_shared<syrec::Number>(compileTimeConstantExpression));
        }

        syrec::expression::ptr simplifiedToExpr;
        if (const auto& compileTimeConstantExprAsBinaryOne = tryConvertCompileTimeConstantExpressionToBinaryExpr(compileTimeConstantExpression, sharedData->currentSymbolTableScope); compileTimeConstantExprAsBinaryOne.has_value()) {
            std::vector<syrec::VariableAccess::ptr> optimizedAwaySignalAccesses;
            if (const auto& simplificationResultOfBinaryExpr = optimizations::simplifyBinaryExpression(*compileTimeConstantExprAsBinaryOne, sharedData->parserConfig->operationStrengthReductionEnabled, sharedData->optionalMultiplicationSimplifier, sharedData->currentSymbolTableScope, optimizedAwaySignalAccesses); simplificationResultOfBinaryExpr != nullptr) {
                simplifiedToExpr = simplificationResultOfBinaryExpr;
            } else if (const auto& basicSimplificationResultOfBinaryExpr = optimizations::trySimplify(*compileTimeConstantExprAsBinaryOne, sharedData->parserConfig->operationStrengthReductionEnabled, sharedData->currentSymbolTableScope, optimizedAwaySignalAccesses); basicSimplificationResultOfBinaryExpr.couldSimplify) {
                simplifiedToExpr = basicSimplificationResultOfBinaryExpr.simplifiedExpression;
            }
            
            if (!optimizedAwaySignalAccesses.empty()) {
                for (const auto& optimizedAwaySignalAccess : optimizedAwaySignalAccesses) {
                    updateReferenceCountOfSignal(optimizedAwaySignalAccess->var->name, Decrement);
                }
            }
        }

        if (const auto& reverseMappingFromBinaryExprToCompileTimeConstant = tryConvertExpressionToCompileTimeConstantOne(simplifiedToExpr); reverseMappingFromBinaryExprToCompileTimeConstant.has_value()) {
            return reverseMappingFromBinaryExprToCompileTimeConstant;
        }
        return std::make_optional(std::make_shared<syrec::Number>(compileTimeConstantExpression));
    }

    const auto lhsOperandEvaluated = tryEvaluateNumber(*lhsOperand, determineContextStartTokenPositionOrUseDefaultOne(context->lhsOperand));
    const auto rhsOperandEvaluated = tryEvaluateNumber(*rhsOperand, determineContextStartTokenPositionOrUseDefaultOne(context->rhsOperand));

    if (lhsOperandEvaluated.has_value() && rhsOperandEvaluated.has_value()) {
        const auto binaryOperationResult = applyBinaryOperation(*operation, *lhsOperandEvaluated, *rhsOperandEvaluated, determineContextStartTokenPositionOrUseDefaultOne(context->rhsOperand));
        if (binaryOperationResult.has_value()) {
            const auto resultContainer = std::make_shared<syrec::Number>(*binaryOperationResult);
            return std::make_optional<syrec::Number::ptr>(resultContainer);
        }
    }

    return std::nullopt;
}

// TODO: Fetching of value for loop variable from loop variable mapping or from signal value lookup in symbol table ?
std::any SyReCCustomBaseVisitor::visitNumberFromLoopVariable(SyReCParser::NumberFromLoopVariableContext* context) {
    if (context->IDENT() == nullptr) {
        return std::nullopt;
    }

    const std::string signalIdent = "$" + context->IDENT()->getText();
    if (!checkIfSignalWasDeclaredOrLogError(
                context->IDENT()->getText(),
                true,
                messageUtils::Message::Position(context->IDENT()->getSymbol()->getLine(), context->IDENT()->getSymbol()->getCharPositionInLine()))) {
        return std::nullopt;
    }
    updateReferenceCountOfSignal(signalIdent, Increment);
    if (sharedData->lastDeclaredLoopVariable.has_value() && *sharedData->lastDeclaredLoopVariable == signalIdent) {
        createMessage(mapAntlrTokenPosition(context->IDENT()->getSymbol()), messageUtils::Message::Severity::Error, CannotReferenceLoopVariableInInitalValueDefinition, signalIdent);
        return std::nullopt;
    }

    std::optional<syrec::Number::ptr> valueOfLoopVariable;
    const auto&                       symTableEntryForSignal = sharedData->currentSymbolTableScope->getVariable(signalIdent);
    if (symTableEntryForSignal.has_value() && std::holds_alternative<syrec::Number::ptr>(symTableEntryForSignal.value())) {
        const auto& symTableValueContainerOfLoopVariable = std::get<syrec::Number::ptr>(symTableEntryForSignal.value());
        if (const auto& evaluatedLoopVariableValue = tryEvaluateNumber(symTableValueContainerOfLoopVariable, determineContextStartTokenPositionOrUseDefaultOne(nullptr)); evaluatedLoopVariableValue.has_value()) {
            valueOfLoopVariable.emplace(std::make_shared<syrec::Number>(*evaluatedLoopVariableValue));
        }
        else {
            valueOfLoopVariable.emplace(symTableValueContainerOfLoopVariable);    
        }
    } else {
        // TODO: GEN_ERROR, this should not happen but check anyways
        // TODO: Error position
        createMessageAtUnknownPosition(messageUtils::Message::Severity::Error , "TODO");
    }

    return valueOfLoopVariable;
}

/*
 * We are currently assuming that defining a loop unroll config with a subsequent partial or full unroll of a loop being done will make the value of the loop variable (if defined)
 * available regardless of the value of the constant propagation flag
 */
inline bool SyReCCustomBaseVisitor::canEvaluateNumber(const syrec::Number::ptr& number) const {
    if (number->isLoopVariable()) {
        return !sharedData->performingReadOnlyParsingOfLoopBody
            && sharedData->currentSymbolTableScope->contains(number->variableName())
            && sharedData->loopVariableMappingLookup.count(number->variableName()) != 0;
    }
    return number->isCompileTimeConstantExpression() ? canEvaluateCompileTimeExpression(number->getExpression()) : true;
}

inline bool SyReCCustomBaseVisitor::canEvaluateCompileTimeExpression(const syrec::Number::CompileTimeConstantExpression& compileTimeExpression) const {
    return canEvaluateNumber(compileTimeExpression.lhsOperand) && canEvaluateNumber(compileTimeExpression.rhsOperand);
}

// TODO: CONSTANT_PROPAGATION: Only use symbol table as source for value of loop variables
std::optional<unsigned> SyReCCustomBaseVisitor::tryEvaluateNumber(const syrec::Number::ptr& numberContainer, const messageUtils::Message::Position& evaluationErrorPositionHelper) {
    if (numberContainer->isLoopVariable() && !canEvaluateNumber(numberContainer)) {
        return std::nullopt;
    }
    if (numberContainer->isCompileTimeConstantExpression()) {
        return tryEvaluateCompileTimeExpression(numberContainer->getExpression(), evaluationErrorPositionHelper);
    }

    return std::make_optional(numberContainer->evaluate(sharedData->loopVariableMappingLookup));
}


std::optional<unsigned> SyReCCustomBaseVisitor::tryEvaluateCompileTimeExpression(const syrec::Number::CompileTimeConstantExpression& compileTimeExpression, const messageUtils::Message::Position& evaluationErrorPositionHelper) {
    const auto lhsEvaluated = canEvaluateNumber(compileTimeExpression.lhsOperand) ? tryEvaluateNumber(compileTimeExpression.lhsOperand, evaluationErrorPositionHelper) : std::nullopt;
    const auto rhsEvaluated = (lhsEvaluated.has_value() && canEvaluateNumber(compileTimeExpression.rhsOperand)) ? tryEvaluateNumber(compileTimeExpression.rhsOperand, evaluationErrorPositionHelper) : std::nullopt;
    if (rhsEvaluated.has_value() && !*rhsEvaluated && compileTimeExpression.operation == syrec::Number::CompileTimeConstantExpression::Division) {
        createMessage(evaluationErrorPositionHelper, messageUtils::Message::Severity::Error, DivisionByZero);
        return std::nullopt;
    }

    std::optional<syrec_operation::operation> mappedToBinaryOperation;
    switch (compileTimeExpression.operation) {
        case syrec::Number::CompileTimeConstantExpression::Addition:
            mappedToBinaryOperation.emplace(syrec_operation::operation::Addition);
            break;
        case syrec::Number::CompileTimeConstantExpression::Subtraction:
            mappedToBinaryOperation.emplace(syrec_operation::operation::Subtraction);
            break;
        case syrec::Number::CompileTimeConstantExpression::Multiplication:
            mappedToBinaryOperation.emplace(syrec_operation::operation::Multiplication);
            break;
        case syrec::Number::CompileTimeConstantExpression::Division:
            mappedToBinaryOperation.emplace(syrec_operation::operation::Division);
            break;
    }

    if (!mappedToBinaryOperation.has_value()) {
        createMessage(evaluationErrorPositionHelper, messageUtils::Message::Severity::Error, NoMappingForNumberOperation, stringifyCompileTimeConstantExpressionOperation(compileTimeExpression.operation));
        return std::nullopt;
    }
    return syrec_operation::apply(*mappedToBinaryOperation, lhsEvaluated, rhsEvaluated);
}

std::optional<unsigned> SyReCCustomBaseVisitor::applyBinaryOperation(syrec_operation::operation operation, unsigned leftOperand, unsigned rightOperand, const messageUtils::Message::Position& potentialErrorPosition) {
    if (operation == syrec_operation::operation::Division && rightOperand == 0) {
        // TODO: GEN_ERROR
        // TODO: Error position
        createMessage(potentialErrorPosition, messageUtils::Message::Severity::Error, DivisionByZero);
        return std::nullopt;
    }

    return syrec_operation::apply(operation, leftOperand, rightOperand);
}

void SyReCCustomBaseVisitor::insertSkipStatementIfStatementListIsEmpty(syrec::Statement::vec& statementList) const {
    if (statementList.empty()) {
        statementList.emplace_back(std::make_shared<syrec::SkipStatement>());
    }
}

std::optional<SignalAccessRestriction::SignalAccess> SyReCCustomBaseVisitor::tryEvaluateBitOrRangeAccess(const std::pair<syrec::Number::ptr, syrec::Number::ptr>& accessedBits, const messageUtils::Message::Position& optionalEvaluationErrorPosition) {
    if (!canEvaluateNumber(accessedBits.first) || !canEvaluateNumber(accessedBits.second)) {
        return std::nullopt;
    }

    const auto bitRangeStartEvaluated = tryEvaluateNumber(accessedBits.first, optionalEvaluationErrorPosition);
    const auto bitRangeEndEvaluated   = tryEvaluateNumber(accessedBits.second, optionalEvaluationErrorPosition);

    // TODO: If we cannot determine the bit range that was accessed we will default to blocking access on the whole signal
    if (!bitRangeStartEvaluated.has_value() || !bitRangeEndEvaluated.has_value()) {
        return std::nullopt;
    }

    return SignalAccessRestriction::SignalAccess(*bitRangeStartEvaluated, *bitRangeEndEvaluated);
}

std::optional<unsigned> SyReCCustomBaseVisitor::tryDetermineBitwidthAfterVariableAccess(const syrec::VariableAccess::ptr& variableAccess, const messageUtils::Message::Position& evaluationErrorPositionHelper) {
    std::optional<unsigned int> bitWidthAfterVariableAccess = std::make_optional(variableAccess->getVar()->bitwidth);

    if (!variableAccess->range.has_value()) {
        return bitWidthAfterVariableAccess;
    }

    const auto& rangeStart = (*variableAccess->range).first;
    const auto& rangeEnd   = (*variableAccess->range).second;

    if (rangeStart == rangeEnd) {
        bitWidthAfterVariableAccess.emplace(1);
        return bitWidthAfterVariableAccess;
    }

    if (canEvaluateNumber(rangeStart) && canEvaluateNumber(rangeEnd)) {
        const unsigned int bitRangeStart = *tryEvaluateNumber(rangeStart, evaluationErrorPositionHelper);
        const unsigned int bitRangeEnd   = *tryEvaluateNumber(rangeEnd, evaluationErrorPositionHelper);

        bitWidthAfterVariableAccess.emplace((bitRangeEnd - bitRangeStart) + 1);
    } else {
        bitWidthAfterVariableAccess.reset();
    }
    return bitWidthAfterVariableAccess;
}

std::optional<unsigned> SyReCCustomBaseVisitor::tryDetermineExpressionBitwidth(const syrec::expression::ptr& expression, const messageUtils::Message::Position& evaluationErrorPosition) {
    if (auto const* numericExpression = dynamic_cast<syrec::NumericExpression*>(expression.get())) {
        if (canEvaluateNumber(numericExpression->value)) {
            return tryEvaluateNumber(numericExpression->value, evaluationErrorPosition);
        }

        // TODO: LOOP_UNROLLING: What should the default bitwidth of the expression for a loop variable be if we cannot determine the maximum value of the later ?
        if (numericExpression->value->isLoopVariable()) {
            return std::make_optional(UINT_MAX);
        }
        return std::nullopt;
    }

    if (auto const* binaryExpression = dynamic_cast<syrec::BinaryExpression*>(expression.get())) {
        switch (binaryExpression->op) {
            case syrec::BinaryExpression::LogicalAnd:
            case syrec::BinaryExpression::LogicalOr:
            case syrec::BinaryExpression::LessThan:
            case syrec::BinaryExpression::GreaterThan:
            case syrec::BinaryExpression::Equals:
            case syrec::BinaryExpression::NotEquals:
            case syrec::BinaryExpression::LessEquals:
            case syrec::BinaryExpression::GreaterEquals:
                return 1;
            default:
                const auto& lhsBitWidthEvaluated = tryDetermineExpressionBitwidth(binaryExpression->lhs, evaluationErrorPosition);
                const auto& rhsBitWidthEvaluated = lhsBitWidthEvaluated.has_value() ? lhsBitWidthEvaluated : tryDetermineExpressionBitwidth(binaryExpression->rhs, evaluationErrorPosition);

                if (lhsBitWidthEvaluated.has_value() && rhsBitWidthEvaluated.has_value()) {
                    return std::max(*lhsBitWidthEvaluated, *rhsBitWidthEvaluated);
                }

                return lhsBitWidthEvaluated.has_value() ? *lhsBitWidthEvaluated : *rhsBitWidthEvaluated;
        }
    }

    if (auto const* shiftExpression = dynamic_cast<syrec::ShiftExpression*>(expression.get())) {
        return tryDetermineExpressionBitwidth(shiftExpression->lhs, evaluationErrorPosition);
    }

    if (auto const* variableExpression = dynamic_cast<syrec::VariableExpression*>(expression.get())) {
        if (!variableExpression->var->range.has_value()) {
            return variableExpression->bitwidth();
        }

        const auto& bitRangeStart = variableExpression->var->range->first;
        const auto& bitRangeEnd   = variableExpression->var->range->second;

        if (bitRangeStart->isLoopVariable() && bitRangeEnd->isLoopVariable() && bitRangeStart->variableName() == bitRangeEnd->variableName()) {
            return 1;
        }

        if (bitRangeStart->isConstant() && bitRangeEnd->isConstant()) {
            return (bitRangeEnd->evaluate(sharedData->loopVariableMappingLookup) - bitRangeStart->evaluate(sharedData->loopVariableMappingLookup)) + 1;
        }

        return variableExpression->var->bitwidth();
    }

    createMessage(evaluationErrorPosition, messageUtils::Message::Severity::Error, "Could not determine bitwidth for unknown type of expression");
    return std::nullopt;
}

bool SyReCCustomBaseVisitor::checkIfNumberOfValuesPerDimensionMatchOrLogError(const messageUtils::Message::Position& positionOfOptionalError, const std::vector<unsigned>& lhsOperandNumValuesPerDimension, const std::vector<unsigned>& rhsOperandNumValuesPerDimension) {
    if (const auto dimensionsWithMissmatchedNumValues = getDimensionsWithMissmatchedNumberOfValues(lhsOperandNumValuesPerDimension, rhsOperandNumValuesPerDimension); !dimensionsWithMissmatchedNumValues.empty()) {
        std::vector<std::string> perDimensionErrorBuffer;
        for (const auto& dimensionValueMissmatch: dimensionsWithMissmatchedNumValues) {
            perDimensionErrorBuffer.emplace_back(fmt::format(MissmatchedNumberOfValuesForDimension, dimensionValueMissmatch.dimensionIdx, dimensionValueMissmatch.expectedNumberOfValues, dimensionValueMissmatch.actualNumberOfValues));
        }

        std::ostringstream errorsConcatinatedBuffer;
        std::copy(perDimensionErrorBuffer.cbegin(), perDimensionErrorBuffer.cend(), InfixIterator<std::string>(errorsConcatinatedBuffer, ","));
        createMessage(positionOfOptionalError, messageUtils::Message::Severity::Error, MissmatchedNumberOfValuesForDimensionsBetweenOperands, errorsConcatinatedBuffer.str());
        return false;
    }
    return true;
}

std::optional<std::pair<syrec::Number::ptr, syrec::Number::ptr>> SyReCCustomBaseVisitor::isBitOrRangeAccessDefined(SyReCParser::NumberContext* bitRangeStartToken, SyReCParser::NumberContext* bitRangeEndToken) {
    if (bitRangeStartToken == nullptr && bitRangeEndToken == nullptr) {
        return std::nullopt;
    }

    const auto bitRangeStartEvaluated = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(bitRangeStartToken);
    const auto bitRangeEndEvaluated   = bitRangeEndToken != nullptr ? tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(bitRangeEndToken) : bitRangeStartEvaluated;

    return (bitRangeStartEvaluated.has_value() && bitRangeEndEvaluated.has_value()) ? std::make_optional(std::make_pair(*bitRangeStartEvaluated, *bitRangeEndEvaluated)) : std::nullopt;
}

bool SyReCCustomBaseVisitor::validateBitOrRangeAccessOnSignal(const antlr4::Token* bitOrRangeStartToken, const syrec::Variable::ptr& accessedVariable, const std::pair<syrec::Number::ptr, syrec::Number::ptr>& bitOrRangeAccess) {
    const auto& bitOrRangeAccessPairEvaluated = std::make_pair(bitOrRangeAccess.first->evaluate({}), bitOrRangeAccess.second->evaluate({}));
    const auto  signalIdent                   = accessedVariable->name;

    bool isValidSignalAccess = true;
    if (bitOrRangeAccess.first == bitOrRangeAccess.second) {
        if (!isValidBitAccess(accessedVariable, bitOrRangeAccessPairEvaluated.first, true)) {
            isValidSignalAccess                                      = false;
            const IndexAccessRangeConstraint constraintsForBitAccess = getConstraintsForValidBitAccess(accessedVariable, true);
            // TODO: GEN_ERROR: Bit access out of range
            createMessage(mapAntlrTokenPosition(bitOrRangeStartToken), messageUtils::Message::Severity::Error, BitAccessOutOfRange, bitOrRangeAccessPairEvaluated.first, signalIdent, constraintsForBitAccess.minimumValidValue, constraintsForBitAccess.maximumValidValue);
        }
    } else {
        if (bitOrRangeAccessPairEvaluated.first > bitOrRangeAccessPairEvaluated.second) {
            isValidSignalAccess = false;
            // TODO: GEN_ERROR: Bit range start larger than end
            createMessage(mapAntlrTokenPosition(bitOrRangeStartToken), messageUtils::Message::Severity::Error,BitRangeStartLargerThanEnd, bitOrRangeAccessPairEvaluated.first, bitOrRangeAccessPairEvaluated.second);
        } else if (!isValidBitRangeAccess(accessedVariable, bitOrRangeAccessPairEvaluated, true)) {
            isValidSignalAccess                                           = false;
            const IndexAccessRangeConstraint constraintsForBitRangeAccess = getConstraintsForValidBitAccess(accessedVariable, true);
            // TODO: GEN_ERROR: Bit range out of range
            createMessage(mapAntlrTokenPosition(bitOrRangeStartToken), messageUtils::Message::Severity::Error, BitRangeOutOfRange, bitOrRangeAccessPairEvaluated.first, bitOrRangeAccessPairEvaluated.second, signalIdent, constraintsForBitRangeAccess.minimumValidValue, constraintsForBitRangeAccess.maximumValidValue);
        }
    }
    return isValidSignalAccess;
}

bool SyReCCustomBaseVisitor::validateSemanticChecksIfDimensionExpressionIsConstant(const antlr4::Token* dimensionToken, size_t accessedDimensionIdx, const syrec::Variable::ptr& accessedSignal, const std::optional<ExpressionEvaluationResult::ptr>& expressionEvaluationResult) {
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
        createMessage(mapAntlrTokenPosition(dimensionToken),
                      messageUtils::Message::Severity::Error,

                      DimensionValueOutOfRange,
                      expressionResultAsConstant,
                      accessedDimensionIdx,
                      accessedSignal->name,
                      constraintForCurrentDimension.minimumValidValue, constraintForCurrentDimension.maximumValidValue);
    }

    return accessOnCurrentDimensionOk;
}

// TODO: Handling of partial bit ranges (i.e. .2:$i) when evaluating bit range !!!
bool SyReCCustomBaseVisitor::isAccessToAccessedSignalPartRestricted(const syrec::VariableAccess::ptr& accessedSignalPart, const messageUtils::Message::Position& optionalEvaluationErrorPosition) const {
    const auto& existingSignalAccessRestriction = sharedData->getSignalAccessRestriction();
    return existingSignalAccessRestriction.has_value()
        ? (*existingSignalAccessRestriction)->isAccessRestrictedTo(accessedSignalPart)
        : false;
}

/*
 * TODO: First condition can probably be removed since readonly parsing should already disable modifications of reference counts
 */
void SyReCCustomBaseVisitor::updateReferenceCountOfSignal(const std::string_view& signalIdent, SyReCCustomBaseVisitor::ReferenceCountUpdate typeOfUpdate) const {
    if ((sharedData->performingReadOnlyParsingOfLoopBody && sharedData->loopNestingLevel > 0) || sharedData->modificationsOfReferenceCountsDisabled) {
        return;
    }

    if (typeOfUpdate == ReferenceCountUpdate::Increment) {
        sharedData->currentSymbolTableScope->incrementLiteralReferenceCount(signalIdent);   
    }
    else {
        sharedData->currentSymbolTableScope->decrementLiteralReferenceCount(signalIdent);
    }
}

std::string SyReCCustomBaseVisitor::stringifyCompileTimeConstantExpressionOperation(syrec::Number::CompileTimeConstantExpression::Operation ctcOperation) {
    switch (ctcOperation) {
        case syrec::Number::CompileTimeConstantExpression::Operation::Addition:
            return "+";
        case syrec::Number::CompileTimeConstantExpression::Operation::Subtraction:
            return "-";
        case syrec::Number::CompileTimeConstantExpression::Operation::Multiplication:
            return "*";
        case syrec::Number::CompileTimeConstantExpression::Operation::Division:
            return "/";
        default:
            return "<unknown>";
    }
}

std::optional<syrec::BinaryExpression::ptr> SyReCCustomBaseVisitor::tryConvertCompileTimeConstantExpressionToBinaryExpr(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpression, const parser::SymbolTable::ptr& symbolTable) {
    const auto& lhsOperandConverted = tryConvertNumberToExpr(compileTimeConstantExpression.lhsOperand, symbolTable);
    std::optional<syrec_operation::operation> mappedToBinaryOperation;
    switch (compileTimeConstantExpression.operation) {
        case syrec::Number::CompileTimeConstantExpression::Addition:
            mappedToBinaryOperation.emplace(syrec_operation::operation::Addition);
            break;
        case syrec::Number::CompileTimeConstantExpression::Subtraction:
            mappedToBinaryOperation.emplace(syrec_operation::operation::Subtraction);
            break;
        case syrec::Number::CompileTimeConstantExpression::Multiplication:
            mappedToBinaryOperation.emplace(syrec_operation::operation::Multiplication);
            break;
        case syrec::Number::CompileTimeConstantExpression::Division:
            mappedToBinaryOperation.emplace(syrec_operation::operation::Division);
            break;
    }
    const auto& rhsOperandConverted = tryConvertNumberToExpr(compileTimeConstantExpression.rhsOperand,symbolTable);
    if (lhsOperandConverted.has_value() && mappedToBinaryOperation.has_value() && rhsOperandConverted.has_value()) {
        return std::make_optional(
            std::make_shared<syrec::BinaryExpression>(*lhsOperandConverted, *syrec_operation::tryMapBinaryOperationEnumToFlag(*mappedToBinaryOperation), *rhsOperandConverted)
        );
    }
    return std::nullopt;
}

std::optional<syrec::Number::ptr> SyReCCustomBaseVisitor::tryConvertExpressionToCompileTimeConstantOne(const syrec::expression::ptr& expr) {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        const auto& convertedLhsOperand = tryConvertExpressionToCompileTimeConstantOne(exprAsBinaryExpr->lhs);
        const auto& convertedBinaryExpr = syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
        const auto& convertedRhsOperand = tryConvertExpressionToCompileTimeConstantOne(exprAsBinaryExpr->rhs);

        if (!convertedLhsOperand.has_value() || !convertedBinaryExpr.has_value() || !convertedRhsOperand.has_value()) {
            return std::nullopt;
        }

        syrec::Number::CompileTimeConstantExpression::Operation compileTimeConstantExpressionOperation;
        switch (*convertedBinaryExpr) {
            case syrec_operation::operation::Addition:
                compileTimeConstantExpressionOperation = syrec::Number::CompileTimeConstantExpression::Operation::Addition;
                break;
            case syrec_operation::operation::Subtraction:
                compileTimeConstantExpressionOperation = syrec::Number::CompileTimeConstantExpression::Operation::Subtraction;
                break;
            case syrec_operation::operation::Multiplication:
                compileTimeConstantExpressionOperation = syrec::Number::CompileTimeConstantExpression::Operation::Multiplication;
                break;
            case syrec_operation::operation::Division:
                compileTimeConstantExpressionOperation = syrec::Number::CompileTimeConstantExpression::Operation::Division;
                break;
            default:
                return std::nullopt;
        }
        return std::make_optional(std::make_shared<syrec::Number>(syrec::Number::CompileTimeConstantExpression(
                *convertedLhsOperand,
                compileTimeConstantExpressionOperation,
                *convertedRhsOperand)));
    }
    if (const auto& exprAsVariableExpr =std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsVariableExpr != nullptr) {
        if (exprAsVariableExpr->var->indexes.empty() && !exprAsVariableExpr->var->range.has_value()) {
            return std::make_optional(std::make_shared<syrec::Number>(exprAsVariableExpr->var->getVar()->name));
        }
    }
    if (const auto& exprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(expr); exprAsNumericExpr != nullptr) {
        return std::make_optional(exprAsNumericExpr->value);
    }
    return std::nullopt;
}

std::optional<syrec::expression::ptr> SyReCCustomBaseVisitor::tryConvertNumberToExpr(const syrec::Number::ptr& number, const parser::SymbolTable::ptr& symbolTable) {
    if (number->isConstant()) {
        if (const auto& constantValueOfNumber = SignalAccessUtils::tryEvaluateNumber(number, symbolTable); constantValueOfNumber.has_value()) {
            return std::make_shared<syrec::NumericExpression>(std::make_shared<syrec::Number>(*constantValueOfNumber), BitHelpers::getRequiredBitsToStoreValue(*constantValueOfNumber));
        }
        return std::nullopt;
    }
    if (number->isLoopVariable()) {
        // TODO: Is it ok to assume a bitwidth of 1 for the given loop variable
        return std::make_shared<syrec::NumericExpression>(std::make_shared<syrec::Number>(number->variableName()), 1);
    }
    if (number->isCompileTimeConstantExpression()) {
        return tryConvertCompileTimeConstantExpressionToBinaryExpr(number->getExpression(), symbolTable);
    }
    return std::nullopt;
}

std::optional<unsigned> SyReCCustomBaseVisitor::tryPerformConstantPropagationForSignalAccess(const syrec::VariableAccess::ptr& accessedSignal) const {
    /*
     * In addition to the user defined flag for the constant propagation optimization we also use the internal flag indicating
     * whether we are currently parsing a signal access for which no signal value lookup should be done (i.e. for the operands of a swap or unary assignment statement)
     */
    if (!sharedData->performPotentialValueLookupForCurrentlyAccessedSignal || !sharedData->parserConfig->performConstantPropagation || sharedData->performingReadOnlyParsingOfLoopBody) {
        return std::nullopt;
    }

    const auto& optionalExistingConstantPropagationRestrictions = !sharedData->loopBodyValuePropagationBlockers.empty() ? std::make_optional(sharedData->loopBodyValuePropagationBlockers.top()) : std::nullopt;
    if (optionalExistingConstantPropagationRestrictions.has_value() && optionalExistingConstantPropagationRestrictions.value()->isAccessBlockedFor(accessedSignal)) {

        return std::nullopt;
    }

    return sharedData->currentSymbolTableScope->tryFetchValueForLiteral(accessedSignal);
}

std::optional<unsigned> SyReCCustomBaseVisitor::convertToNumber(const std::string& text) {
    try {
        return std::stoul(text) & UINT_MAX;
    } catch (std::invalid_argument&) {
        return std::nullopt;
    } catch (std::out_of_range&) {
        return std::nullopt;
    }
}

