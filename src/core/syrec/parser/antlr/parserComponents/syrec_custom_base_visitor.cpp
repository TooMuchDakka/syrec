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

using namespace parser;


bool SyReCCustomBaseVisitor::checkIfSignalWasDeclaredOrLogError(const std::string& signalIdent, const bool isLoopVariable, const TokenPosition& signalIdentTokenPosition) {
    const std::string signalIdentTransformed = isLoopVariable ? "$" + signalIdent : signalIdent;
    if (!sharedData->currentSymbolTableScope->contains(signalIdentTransformed)) {
        createError(signalIdentTokenPosition, fmt::format(UndeclaredIdent, signalIdentTransformed));
        return false;
    }
    return true;
}

bool SyReCCustomBaseVisitor::isSignalAssignableOtherwiseCreateError(const antlr4::Token* signalIdentToken, const syrec::VariableAccess::ptr& assignedToVariable) {
    if (syrec::Variable::Types::In == assignedToVariable->getVar()->type) {
        // TODO: Error position
        createError(mapAntlrTokenPosition(signalIdentToken), fmt::format(AssignmentToReadonlyVariable, assignedToVariable->getVar()->name));
        return false;
    }
    return true;
}

inline void SyReCCustomBaseVisitor::createError(const TokenPosition& tokenPosition, const std::string& errorMessage) {
    sharedData->errors.emplace_back(ParserUtilities::createError(tokenPosition.line, tokenPosition.column, errorMessage));
}

inline void SyReCCustomBaseVisitor::createWarning(const TokenPosition& tokenPosition, const std::string& warningMessage) {
    sharedData->warnings.emplace_back(ParserUtilities::createWarning(tokenPosition.line, tokenPosition.column, warningMessage));
}

inline SyReCCustomBaseVisitor::TokenPosition SyReCCustomBaseVisitor::determineContextStartTokenPositionOrUseDefaultOne(const antlr4::ParserRuleContext* context) {
    return context == nullptr ? TokenPosition(fallbackErrorLinePosition, fallbackErrorColumnPosition) : mapAntlrTokenPosition(context->start);
}

std::optional<syrec_operation::operation> SyReCCustomBaseVisitor::getDefinedOperation(const antlr4::Token* definedOperationToken) {
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
            createError(mapAntlrTokenPosition(definedOperationToken), "TODO: No mapping for operation");
            break;
    }
    return definedOperation;
}

inline SyReCCustomBaseVisitor::TokenPosition SyReCCustomBaseVisitor::mapAntlrTokenPosition(const antlr4::Token* token) {
    if (token == nullptr) {
        return TokenPosition(fallbackErrorLinePosition, fallbackErrorColumnPosition);
    }
    return TokenPosition(token->getLine(), token->getCharPositionInLine());
}

std::any SyReCCustomBaseVisitor::visitSignal(SyReCParser::SignalContext* context) {
    std::string                               signalIdent;
    std::optional<syrec::VariableAccess::ptr> accessedSignal;
    bool                                      isValidSignalAccess = context->IDENT() != nullptr && checkIfSignalWasDeclaredOrLogError(context->IDENT()->getText(), false, TokenPosition(context->IDENT()->getSymbol()->getLine(), context->IDENT()->getSymbol()->getCharPositionInLine()));

    if (isValidSignalAccess) {
        signalIdent                  = context->IDENT()->getText();
        const auto signalSymTabEntry = sharedData->currentSymbolTableScope->getVariable(signalIdent);
        isValidSignalAccess          = signalSymTabEntry.has_value() && std::holds_alternative<syrec::Variable::ptr>(*signalSymTabEntry);

        if (isValidSignalAccess) {
            const syrec::VariableAccess::ptr container = std::make_shared<syrec::VariableAccess>();
            container->setVar(std::get<syrec::Variable::ptr>(*signalSymTabEntry));
            accessedSignal.emplace(container);

            if (!sharedData->currentlyInOmittedRegion) {
                // TODO: UNUSED_REFERENCE - Marked as used
                sharedData->currentSymbolTableScope->incrementLiteralReferenceCount(signalIdent);   
            }
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
            createError(determineContextStartTokenPositionOrUseDefaultOne(context->accessedDimensions.at(dimensionOutOfRangeIdx)), fmt::format(DimensionOutOfRange, dimensionOutOfRangeIdx, signalIdent, numDimensionsOfAccessSignal));
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
            createError(restrictionErrorPosition, fmt::format(AccessingRestrictedPartOfSignal, signalIdent));
        } else {
            if (sharedData->parserConfig->performConstantPropagation) {
                const auto& fetchedValueForSignal = sharedData->currentSymbolTableScope->tryFetchValueForLiteral(*accessedSignal);
                if (fetchedValueForSignal.has_value()) {
                    return std::make_optional(std::make_shared<SignalEvaluationResult>(std::make_shared<syrec::Number>(*fetchedValueForSignal)));
                }
            }
            return std::make_optional(std::make_shared<SignalEvaluationResult>(*accessedSignal));                
        }
    }
    return std::nullopt;
}

std::any SyReCCustomBaseVisitor::visitNumberFromConstant(SyReCParser::NumberFromConstantContext* context) {
    if (context->INT() == nullptr) {
        return std::nullopt;
    }

    return std::make_optional(std::make_shared<syrec::Number>(ParserUtilities::convertToNumber(context->INT()->getText()).value()));
}

std::any SyReCCustomBaseVisitor::visitNumberFromSignalwidth(SyReCParser::NumberFromSignalwidthContext* context) {
    if (context->IDENT() == nullptr) {
        return std::nullopt;
    }

    const std::string signalIdent = context->IDENT()->getText();
    if (!checkIfSignalWasDeclaredOrLogError(
        context->IDENT()->getText(),
        false,
        TokenPosition(context->IDENT()->getSymbol()->getLine(), context->IDENT()->getSymbol()->getCharPositionInLine()))) {
        return std::nullopt;
    }

    std::optional<syrec::Number::ptr> signalWidthOfSignal;
    const auto&                       symTableEntryForSignal = sharedData->currentSymbolTableScope->getVariable(signalIdent);
    if (symTableEntryForSignal.has_value() && std::holds_alternative<syrec::Variable::ptr>(*symTableEntryForSignal)) {
        signalWidthOfSignal.emplace(std::make_shared<syrec::Number>(std::get<syrec::Variable::ptr>(*symTableEntryForSignal)->bitwidth));
    } else {
        // TODO: GEN_ERROR, this should not happen
        // TODO: Error position
        createError(TokenPosition::createFallbackPosition(), "TODO");
    }

    return signalWidthOfSignal;
}

std::any SyReCCustomBaseVisitor::visitNumberFromExpression(SyReCParser::NumberFromExpressionContext* context) {
    const auto lhsOperand = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->lhsOperand);

    // TODO: What is the return value if an invalid number operation is defined, we know that a parsing error will be created
    const auto operation = getDefinedOperation(context->op);
    if (!operation.has_value()) {
        // TODO: Error position
        createError(mapAntlrTokenPosition(context->op), InvalidOrMissingNumberExpressionOperation);
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
    if (!canEvaluateNumber(*lhsOperand) || !canEvaluateNumber(*rhsOperand)) {
        syrec::Number::Operation mappedOperation;
        switch (*operation) {
            case syrec_operation::operation::addition: {
                mappedOperation = syrec::Number::Operation::Addition;
                break;
            }
            case syrec_operation::operation::subtraction: {
                mappedOperation = syrec::Number::Operation::Subtraction;
                break;
            }
            case syrec_operation::operation::multiplication: {
                mappedOperation = syrec::Number::Operation::Multiplication;
                break;
            }
            case syrec_operation::operation::division: {
                mappedOperation = syrec::Number::Operation::Division;
                break;
            }
            default:
                createError(mapAntlrTokenPosition(context->op), fmt::format(NoMappingForNumberOperation, context->op->getText()));
                return std::nullopt;
        }
        return std::make_optional(std::make_shared<syrec::Number>(syrec::Number::CompileTimeConstantExpression(*lhsOperand, mappedOperation, *rhsOperand)));
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
                TokenPosition(context->IDENT()->getSymbol()->getLine(), context->IDENT()->getSymbol()->getCharPositionInLine()))) {
        return std::nullopt;
    }

    // TODO: UNUSED_REFERENCE - Marked as used
    sharedData->currentSymbolTableScope->incrementLiteralReferenceCount(signalIdent);

    if (sharedData->lastDeclaredLoopVariable.has_value() && *sharedData->lastDeclaredLoopVariable == signalIdent) {
        createError(mapAntlrTokenPosition(context->IDENT()->getSymbol()), fmt::format(CannotReferenceLoopVariableInInitalValueDefinition, signalIdent));
        return std::nullopt;
    }

    std::optional<syrec::Number::ptr> valueOfLoopVariable;
    const auto&                       symTableEntryForSignal = sharedData->currentSymbolTableScope->getVariable(signalIdent);
    if (symTableEntryForSignal.has_value() && std::holds_alternative<syrec::Number::ptr>(symTableEntryForSignal.value())) {
        valueOfLoopVariable.emplace(std::get<syrec::Number::ptr>(symTableEntryForSignal.value()));
    } else {
        // TODO: GEN_ERROR, this should not happen but check anyways
        // TODO: Error position
        createError(TokenPosition::createFallbackPosition(), "TODO");
    }

    return valueOfLoopVariable;
}

inline bool SyReCCustomBaseVisitor::canEvaluateNumber(const syrec::Number::ptr& number) const {
    if (number->isLoopVariable()) {
        return sharedData->evaluableLoopVariables.find(number->variableName()) != sharedData->evaluableLoopVariables.end();
    }
    return number->isCompileTimeConstantExpression() ? canEvaluateCompileTimeExpression(number->getExpression()) : true;
}

inline bool SyReCCustomBaseVisitor::canEvaluateCompileTimeExpression(const syrec::Number::CompileTimeConstantExpression& compileTimeExpression) const {
    return canEvaluateNumber(compileTimeExpression.lhsOperand) && canEvaluateNumber(compileTimeExpression.rhsOperand);
}

std::optional<unsigned> SyReCCustomBaseVisitor::tryEvaluateNumber(const syrec::Number::ptr& numberContainer, const TokenPosition& evaluationErrorPositionHelper) {
    if (numberContainer->isLoopVariable()) {
        const std::string& loopVariableIdent = numberContainer->variableName();
        if (sharedData->evaluableLoopVariables.find(loopVariableIdent) == sharedData->evaluableLoopVariables.end() ||
            sharedData->loopVariableMappingLookup.find(loopVariableIdent) == sharedData->loopVariableMappingLookup.end()) {
            return std::nullopt;
        }
    } else if (numberContainer->isCompileTimeConstantExpression()) {
        return tryEvaluateCompileTimeExpression(numberContainer->getExpression(), evaluationErrorPositionHelper);
    }

    return std::make_optional(numberContainer->evaluate(sharedData->loopVariableMappingLookup));
}


std::optional<unsigned> SyReCCustomBaseVisitor::tryEvaluateCompileTimeExpression(const syrec::Number::CompileTimeConstantExpression& compileTimeExpression, const TokenPosition& evaluationErrorPositionHelper) {
    std::optional<unsigned int> evaluationResult;

    const auto lhsEvaluated = canEvaluateNumber(compileTimeExpression.lhsOperand) ? tryEvaluateNumber(compileTimeExpression.lhsOperand, evaluationErrorPositionHelper) : std::nullopt;
    const auto rhsEvaluated = (lhsEvaluated.has_value() && canEvaluateNumber(compileTimeExpression.rhsOperand)) ? tryEvaluateNumber(compileTimeExpression.rhsOperand, evaluationErrorPositionHelper) : std::nullopt;
    if (rhsEvaluated.has_value()) {
        switch (compileTimeExpression.operation) {
            case syrec::Number::Addition:
                evaluationResult.emplace(*lhsEvaluated + *rhsEvaluated);
            case syrec::Number::Subtraction:
                evaluationResult.emplace(*lhsEvaluated - *rhsEvaluated);
            case syrec::Number::Multiplication:
                evaluationResult.emplace(*lhsEvaluated * *rhsEvaluated);
            default:
                if (*rhsEvaluated != 0) {
                    evaluationResult.emplace(*lhsEvaluated / *rhsEvaluated);
                } else {
                    createError(evaluationErrorPositionHelper, DivisionByZero);
                }
        }
    }
    return evaluationResult;
}

std::optional<unsigned> SyReCCustomBaseVisitor::applyBinaryOperation(syrec_operation::operation operation, unsigned leftOperand, unsigned rightOperand, const TokenPosition& potentialErrorPosition) {
    if (operation == syrec_operation::operation::division && rightOperand == 0) {
        // TODO: GEN_ERROR
        // TODO: Error position
        createError(potentialErrorPosition, DivisionByZero);
        return std::nullopt;
    }

    return syrec_operation::apply(operation, leftOperand, rightOperand);
}

void SyReCCustomBaseVisitor::insertSkipStatementIfStatementListIsEmpty(syrec::Statement::vec& statementList) const {
    if (statementList.empty()) {
        statementList.emplace_back(std::make_shared<syrec::SkipStatement>());
    }
}

std::optional<SignalAccessRestriction::SignalAccess> SyReCCustomBaseVisitor::tryEvaluateBitOrRangeAccess(const std::pair<syrec::Number::ptr, syrec::Number::ptr>& accessedBits, const TokenPosition& optionalEvaluationErrorPosition) {
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

std::optional<unsigned> SyReCCustomBaseVisitor::tryDetermineBitwidthAfterVariableAccess(const syrec::VariableAccess::ptr& variableAccess, const TokenPosition& evaluationErrorPositionHelper) {
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

std::optional<unsigned> SyReCCustomBaseVisitor::tryDetermineExpressionBitwidth(const syrec::expression::ptr& expression, const TokenPosition& evaluationErrorPosition) {
    if (auto const* numericExpression = dynamic_cast<syrec::NumericExpression*>(expression.get())) {
        if (canEvaluateNumber(numericExpression->value)) {
            return tryEvaluateNumber(numericExpression->value, evaluationErrorPosition);
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

    createError(evaluationErrorPosition, "Could not determine bitwidth for unknown type of expression");
    return std::nullopt;
}

bool SyReCCustomBaseVisitor::checkIfNumberOfValuesPerDimensionMatchOrLogError(const TokenPosition& positionOfOptionalError, const std::vector<unsigned>& lhsOperandNumValuesPerDimension, const std::vector<unsigned>& rhsOperandNumValuesPerDimension) {
    if (const auto dimensionsWithMissmatchedNumValues = getDimensionsWithMissmatchedNumberOfValues(lhsOperandNumValuesPerDimension, rhsOperandNumValuesPerDimension); !dimensionsWithMissmatchedNumValues.empty()) {
        std::vector<std::string> perDimensionErrorBuffer;
        for (const auto& dimensionValueMissmatch: dimensionsWithMissmatchedNumValues) {
            perDimensionErrorBuffer.emplace_back(fmt::format(MissmatchedNumberOfValuesForDimension, dimensionValueMissmatch.dimensionIdx, dimensionValueMissmatch.expectedNumberOfValues, dimensionValueMissmatch.actualNumberOfValues));
        }

        std::ostringstream errorsConcatinatedBuffer;
        std::copy(perDimensionErrorBuffer.cbegin(), perDimensionErrorBuffer.cend(), infix_ostream_iterator<std::string>(errorsConcatinatedBuffer, ","));
        createError(positionOfOptionalError, fmt::format(MissmatchedNumberOfValuesForDimensionsBetweenOperands, errorsConcatinatedBuffer.str()));
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
            createError(mapAntlrTokenPosition(bitOrRangeStartToken), fmt::format(BitAccessOutOfRange, bitOrRangeAccessPairEvaluated.first, signalIdent, constraintsForBitAccess.minimumValidValue, constraintsForBitAccess.maximumValidValue));
        }
    } else {
        if (bitOrRangeAccessPairEvaluated.first > bitOrRangeAccessPairEvaluated.second) {
            isValidSignalAccess = false;
            // TODO: GEN_ERROR: Bit range start larger than end
            createError(mapAntlrTokenPosition(bitOrRangeStartToken), fmt::format(BitRangeStartLargerThanEnd, bitOrRangeAccessPairEvaluated.first, bitOrRangeAccessPairEvaluated.second));
        } else if (!isValidBitRangeAccess(accessedVariable, bitOrRangeAccessPairEvaluated, true)) {
            isValidSignalAccess                                           = false;
            const IndexAccessRangeConstraint constraintsForBitRangeAccess = getConstraintsForValidBitAccess(accessedVariable, true);
            // TODO: GEN_ERROR: Bit range out of range
            createError(mapAntlrTokenPosition(bitOrRangeStartToken), fmt::format(BitRangeOutOfRange, bitOrRangeAccessPairEvaluated.first, bitOrRangeAccessPairEvaluated.second, signalIdent, constraintsForBitRangeAccess.minimumValidValue, constraintsForBitRangeAccess.maximumValidValue));
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
        createError(mapAntlrTokenPosition(dimensionToken),
                    fmt::format(
                            DimensionValueOutOfRange,
                            expressionResultAsConstant,
                            accessedDimensionIdx,
                            accessedSignal->name,
                            constraintForCurrentDimension.minimumValidValue, constraintForCurrentDimension.maximumValidValue));
    }

    return accessOnCurrentDimensionOk;
}

// TODO: Handling of partial bit ranges (i.e. .2:$i) when evaluating bit range !!!
bool SyReCCustomBaseVisitor::isAccessToAccessedSignalPartRestricted(const syrec::VariableAccess::ptr& accessedSignalPart, const TokenPosition& optionalEvaluationErrorPosition) const {
    const auto& existingSignalAccessRestriction = sharedData->getSignalAccessRestriction();
    return existingSignalAccessRestriction.has_value()
        ? (*existingSignalAccessRestriction)->isAccessRestrictedTo(accessedSignalPart)
        : false;
}