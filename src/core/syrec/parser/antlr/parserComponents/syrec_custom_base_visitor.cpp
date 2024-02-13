#include "core/syrec/parser/antlr/parserComponents/syrec_custom_base_visitor.hpp"

#include "core/syrec/variable.hpp"
#include "core/syrec/parser/custom_semantic_errors.hpp"
#include "core/syrec/parser/infix_iterator.hpp"
#include "core/syrec/parser/range_check.hpp"
#include "core/syrec/parser/signal_evaluation_result.hpp"
#include "core/syrec/parser/value_broadcaster.hpp"
#include "core/syrec/parser/utils/signal_access_utils.hpp"

#include <charconv>

using namespace parser;

bool SyReCCustomBaseVisitor::checkIfSignalWasDeclaredOrLogError(const std::string_view& signalIdent, const messageUtils::Message::Position& signalIdentTokenPosition) {
    if (!sharedData->currentSymbolTableScope->contains(signalIdent)) {
        createError(signalIdentTokenPosition, UndeclaredIdent, signalIdent);
        return false;
    }
    return true;
}

bool SyReCCustomBaseVisitor::isSignalAssignableOtherwiseCreateError(const unsigned int signalType, const std::string_view& signalIdent, const messageUtils::Message::Position& signalIdentTokenPosition) {
    if (syrec::Variable::Types::In == signalType) {
        // TODO: Error position
        createError(signalIdentTokenPosition,  AssignmentToReadonlyVariable, std::string(signalIdent));
        return false;
    }
    return true;
}

messageUtils::Message::Position SyReCCustomBaseVisitor::determineContextStartTokenPositionOrUseDefaultOne(const antlr4::ParserRuleContext* context) {
    return context ? mapAntlrTokenPosition(context->start) : messageUtils::Message::Position(SharedVisitorData::fallbackErrorLinePosition, SharedVisitorData::fallbackErrorColumnPosition);
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
            createError(mapAntlrTokenPosition(definedOperationToken), "TODO: No mapping for operation");
            break;
    }
    return definedOperation;
}

messageUtils::Message::Position SyReCCustomBaseVisitor::mapAntlrTokenPosition(const antlr4::Token* token) {
    if (!token) {
        return messageUtils::Message::Position(SharedVisitorData::fallbackErrorLinePosition, SharedVisitorData::fallbackErrorColumnPosition);
    }
    return messageUtils::Message::Position(token->getLine(), token->getCharPositionInLine());
}

std::any SyReCCustomBaseVisitor::visitSignal(SyReCParser::SignalContext* context) {
    if (!context->IDENT()) {
        return std::nullopt;
    }

    bool isValidSignalAccess = false;
    std::string                               signalIdent         = context->IDENT()->getText();
    syrec::Variable::ptr                      referenceToAccessedSignalFromSymbolTable;
    syrec::VariableAccess::ptr                generatedSignalAccess = std::make_shared<syrec::VariableAccess>();

    if (checkIfSignalWasDeclaredOrLogError(signalIdent, mapAntlrTokenPosition(context->IDENT()->getSymbol()))) {
        if (const auto& fetchedSymbolTableEntry = sharedData->currentSymbolTableScope->getVariable(signalIdent); fetchedSymbolTableEntry.has_value() && std::holds_alternative<syrec::Variable::ptr>(*fetchedSymbolTableEntry)) {
            isValidSignalAccess = true;
            referenceToAccessedSignalFromSymbolTable = std::get<syrec::Variable::ptr>(*fetchedSymbolTableEntry);
            generatedSignalAccess->setVar(referenceToAccessedSignalFromSymbolTable);
        }
    }

    if (isValidSignalAccess) {
        const auto numValuePerDimensionAstNodes = context->accessedDimensions.size();
        const auto numDeclaredDimensions        = referenceToAccessedSignalFromSymbolTable->dimensions.size();

        const auto numAstNodesWithinRangeOfDeclaredDimensions = std::min(numValuePerDimensionAstNodes, numDeclaredDimensions);
        generatedSignalAccess->indexes.reserve(numAstNodesWithinRangeOfDeclaredDimensions);

        for (std::size_t i = 0; i < numAstNodesWithinRangeOfDeclaredDimensions; ++i) {
            if (const auto& evaluationResultOfAccessedValueOfDimensionExpr = tryVisitAndConvertProductionReturnValue<ExpressionEvaluationResult::ptr>(context->accessedDimensions.at(i)); evaluationResultOfAccessedValueOfDimensionExpr.has_value()) {
                isValidSignalAccess &= isAccessedValueOfDimensionWithinRange(mapAntlrTokenPosition(context->accessedDimensions.at(i)->start), i, referenceToAccessedSignalFromSymbolTable, **evaluationResultOfAccessedValueOfDimensionExpr)
                    .value_or(true);
                if (const auto& exprDefiningAccessedValueOfDimension = evaluationResultOfAccessedValueOfDimensionExpr->get()->getAsExpression(); exprDefiningAccessedValueOfDimension.has_value()) {
                    generatedSignalAccess->indexes.emplace_back(*exprDefiningAccessedValueOfDimension);   
                }
            }
            else {
                isValidSignalAccess = false;
            }
        }

        if (numValuePerDimensionAstNodes > numDeclaredDimensions) {
            for (std::size_t i = numAstNodesWithinRangeOfDeclaredDimensions; i < numValuePerDimensionAstNodes; ++i) {
                createError(determineContextStartTokenPositionOrUseDefaultOne(context->accessedDimensions.at(i)),  DimensionOutOfRange, i, signalIdent, numDeclaredDimensions);
            }
        }
    }

    if (const auto& bitRangeAccessEvaluated = isBitOrRangeAccessDefined(context->bitStart, context->bitRangeEnd); bitRangeAccessEvaluated.has_value()) {
        const auto& isBitAccess                    = bitRangeAccessEvaluated->first == bitRangeAccessEvaluated->second;
        const auto& canEvaluatedBitRangeComponents = canEvaluateNumber(*bitRangeAccessEvaluated->first) && (!isBitAccess ? canEvaluateNumber(*bitRangeAccessEvaluated->second) : true);
        auto        areBitRangeAccessComponentsOk  = true;
        if (canEvaluatedBitRangeComponents) {
            areBitRangeAccessComponentsOk = validateBitOrRangeAccessOnSignal(context->bitStart->start, referenceToAccessedSignalFromSymbolTable, *bitRangeAccessEvaluated);
        }

        isValidSignalAccess &= areBitRangeAccessComponentsOk;
        generatedSignalAccess->range = bitRangeAccessEvaluated;
    }

    if (const auto& positionOfSignalAccessRestrictionError = determineContextStartTokenPositionOrUseDefaultOne(context); referenceToAccessedSignalFromSymbolTable && existsRestrictionForAccessedSignalParts(*generatedSignalAccess, &positionOfSignalAccessRestrictionError)) {
        createError(positionOfSignalAccessRestrictionError,  AccessingRestrictedPartOfSignal, signalIdent);
        isValidSignalAccess &= false;
    }

    /*
     * Since antlr omits token that were triggered syntax errors (i.e. a misplaced square bracket in the dimension access) we also
     * include into the validity status of a signal access the existence of an syntax errors in the given signal access declaration.
     * The reasoning for this step is to prevent semantic errors that would work with a signal access that was different from the declared one.
     * But this check will not always work (i.e. omitting the opening bracket for the last accessed value of the last dimension of a dimension access) makes
     * the whole last value of the dimension unavailable to this rule and thus would also not invalidate the currently parsed signal access.
     */
    isValidSignalAccess &= context->start && context->getStop() && !wasSyntaxErrorDetectedBetweenTokens(mapAntlrTokenPosition(context->start), mapAntlrTokenPosition(context->getStop()));
    if (!isValidSignalAccess) {
        return std::nullopt;
    }

    const auto& generatedSignalEvaluationResult = std::make_shared<SignalEvaluationResult>(generatedSignalAccess);
    return std::make_optional(generatedSignalEvaluationResult);
}

// TODO: GENERAL Check that defined number can be stored in data type used for numbers
std::any SyReCCustomBaseVisitor::visitNumberFromConstant(SyReCParser::NumberFromConstantContext* context) {
    if (const auto& numberToken = context->INT(); numberToken != nullptr) {
        if (const auto& tokenTextAsNumber = tryConvertTextToNumber(numberToken->getText()); tokenTextAsNumber.has_value()) {
            return std::make_optional(std::make_shared<syrec::Number>(*tokenTextAsNumber));
        }
    }
    return std::nullopt;
}

std::any SyReCCustomBaseVisitor::visitNumberFromSignalwidth(SyReCParser::NumberFromSignalwidthContext* context) {
    if (!context->IDENT()) {
        return std::nullopt;
    }

    const auto signalIdent = context->IDENT()->getText();
    if (!checkIfSignalWasDeclaredOrLogError(signalIdent, mapAntlrTokenPosition(context->IDENT()->getSymbol()))) {
        return std::nullopt;
    }

    if (const auto& symbolTableEntryForAccessedSignal = sharedData->currentSymbolTableScope->getVariable(signalIdent); symbolTableEntryForAccessedSignal.has_value() && std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForAccessedSignal)) {
        const auto& bitWidthOfAccessedSignal = std::get<syrec::Variable::ptr>(*symbolTableEntryForAccessedSignal)->bitwidth;
        return std::make_optional(std::make_shared<syrec::Number>(bitWidthOfAccessedSignal));
    }
    return std::nullopt;
}

std::any SyReCCustomBaseVisitor::visitNumberFromExpression(SyReCParser::NumberFromExpressionContext* context) {
    const auto lhsOperand = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(context->lhsOperand);

    // TODO: What is the return value if an invalid number operation is defined, we know that a parsing error will be created
    const auto operation = getDefinedOperation(context->op);
    if (!operation.has_value()) {
        // TODO: Error position
        createError(mapAntlrTokenPosition(context->op),  InvalidOrMissingNumberExpressionOperation);
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
            createError(mapAntlrTokenPosition(context->op),  NoMappingForNumberOperation, context->op->getText());
            return std::nullopt;
    }

    if (!canEvaluateNumber(**lhsOperand) ||!canEvaluateNumber(**rhsOperand)) {
        const auto compileTimeConstantExpression = syrec::Number::CompileTimeConstantExpression(*lhsOperand, mappedOperation, *rhsOperand);
        return std::make_optional(std::make_shared<syrec::Number>(compileTimeConstantExpression));
    }

    const auto& potentialErrorPositionForLhsOperand = determineContextStartTokenPositionOrUseDefaultOne(context->lhsOperand);
    const auto& potentialErrorPositionForRhsOperand = determineContextStartTokenPositionOrUseDefaultOne(context->rhsOperand);
    const auto  lhsOperandEvaluated                 = tryEvaluateNumber(**lhsOperand, &potentialErrorPositionForLhsOperand);
    const auto  rhsOperandEvaluated                 = tryEvaluateNumber(**rhsOperand, &potentialErrorPositionForRhsOperand);

    if (lhsOperandEvaluated.has_value() && rhsOperandEvaluated.has_value()) {
        if (const auto binaryOperationResult = applyBinaryOperation(*operation, *lhsOperandEvaluated, *rhsOperandEvaluated, determineContextStartTokenPositionOrUseDefaultOne(context->rhsOperand)); binaryOperationResult.has_value()) {
            const auto resultContainer = std::make_shared<syrec::Number>(*binaryOperationResult);
            return std::make_optional<syrec::Number::ptr>(resultContainer);
        }
    }

    return std::nullopt;
}

std::any SyReCCustomBaseVisitor::visitNumberFromLoopVariable(SyReCParser::NumberFromLoopVariableContext* context) {
    if (!context->IDENT()) {
        return std::nullopt;
    }

    const std::string signalIdent = context->LOOP_VARIABLE_PREFIX()->getText() + context->IDENT()->getText();
    if (!checkIfSignalWasDeclaredOrLogError(signalIdent, mapAntlrTokenPosition(context->IDENT()->getSymbol()))) {
        return std::nullopt;
    }

    if (sharedData->lastDeclaredLoopVariable.has_value() && *sharedData->lastDeclaredLoopVariable == signalIdent) {
        createError(mapAntlrTokenPosition(context->IDENT()->getSymbol()),  CannotReferenceLoopVariableInInitalValueDefinition, signalIdent);
        return std::nullopt;
    }

    const auto& generatedNumberContainer = std::make_shared<syrec::Number>(signalIdent);
    return std::make_optional(generatedNumberContainer);
}

/*
 * We are currently assuming that defining a loop unroll config with a subsequent partial or full unroll of a loop being done will make the value of the loop variable (if defined)
 * available regardless of the value of the constant propagation flag
 */
inline bool SyReCCustomBaseVisitor::canEvaluateNumber(const syrec::Number& number) const {
    return number.isCompileTimeConstantExpression() ? canEvaluateCompileTimeExpression(number.getExpression()) : number.isConstant();
}

bool SyReCCustomBaseVisitor::wasSyntaxErrorDetectedBetweenTokens(const messageUtils::Message::Position& rangeOfInterestStartPosition, const messageUtils::Message::Position& rangeOfInterestEndPosition) const {
    return std::find_if(
            sharedData->errors.cbegin(),
            sharedData->errors.cend(),
            [&rangeOfInterestStartPosition, &rangeOfInterestEndPosition](const messageUtils::Message& errorMsg) {
                       return errorMsg.category == messageUtils::Message::Syntax && errorMsg.position.compare(rangeOfInterestStartPosition) > -1 && errorMsg.position.compare(rangeOfInterestEndPosition) < 1;
                   }) != sharedData->errors.cend();
}


inline bool SyReCCustomBaseVisitor::canEvaluateCompileTimeExpression(const syrec::Number::CompileTimeConstantExpression& compileTimeExpression) const {
    return canEvaluateNumber(*compileTimeExpression.lhsOperand) && canEvaluateNumber(*compileTimeExpression.rhsOperand);
}

std::optional<unsigned> SyReCCustomBaseVisitor::tryEvaluateNumber(const syrec::Number& numberContainer, const messageUtils::Message::Position* evaluationErrorPositionHelper) {
    if (canEvaluateNumber(numberContainer)) {
        if (numberContainer.isCompileTimeConstantExpression()) {
            return tryEvaluateCompileTimeExpression(numberContainer.getExpression(), evaluationErrorPositionHelper);
        }
        return std::make_optional(numberContainer.evaluate({}));
    }
    return std::nullopt;
}


std::optional<unsigned> SyReCCustomBaseVisitor::tryEvaluateCompileTimeExpression(const syrec::Number::CompileTimeConstantExpression& compileTimeExpression, const messageUtils::Message::Position* evaluationErrorPositionHelper) {
    const auto lhsEvaluated = tryEvaluateNumber(*compileTimeExpression.lhsOperand, evaluationErrorPositionHelper);
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
        default:
            if (evaluationErrorPositionHelper) {
                createError(*evaluationErrorPositionHelper, NoMappingForNumberOperation, stringifyCompileTimeConstantExpressionOperation(compileTimeExpression.operation));   
            }
            break;
    }

    if (const auto rhsEvaluated = tryEvaluateNumber(*compileTimeExpression.rhsOperand, evaluationErrorPositionHelper); rhsEvaluated.has_value()) {
        if (!*rhsEvaluated && compileTimeExpression.operation == syrec::Number::CompileTimeConstantExpression::Division) {
            if (evaluationErrorPositionHelper) {
                createError(*evaluationErrorPositionHelper, DivisionByZero);   
            }
        } else if (lhsEvaluated.has_value() && mappedToBinaryOperation.has_value()) {
            return  syrec_operation::apply(*mappedToBinaryOperation, lhsEvaluated, rhsEvaluated);
        }
    }
    return std::nullopt;
}

std::optional<unsigned> SyReCCustomBaseVisitor::applyBinaryOperation(syrec_operation::operation operation, unsigned leftOperand, unsigned rightOperand, const messageUtils::Message::Position& potentialErrorPosition) {
    if (operation == syrec_operation::operation::Division && rightOperand == 0) {
        // TODO: GEN_ERROR
        // TODO: Error position
        createError(potentialErrorPosition,  DivisionByZero);
        return std::nullopt;
    }

    return syrec_operation::apply(operation, leftOperand, rightOperand);
}

void SyReCCustomBaseVisitor::insertSkipStatementIfStatementListIsEmpty(syrec::Statement::vec& statementList) const {
    if (statementList.empty()) {
        statementList.emplace_back(std::make_shared<syrec::SkipStatement>());
    }
}

std::optional<unsigned> SyReCCustomBaseVisitor::tryDetermineBitwidthAfterVariableAccess(const syrec::VariableAccess& variableAccess, const messageUtils::Message::Position* evaluationErrorPositionHelper) {
    if (!variableAccess.range.has_value()) {
        return std::make_optional(variableAccess.getVar()->bitwidth);
    }
    
    const auto bitRangeStartEvaluated = tryEvaluateNumber(*variableAccess.range->first, evaluationErrorPositionHelper);
    const auto bitRangeEndEvaluated   = tryEvaluateNumber(*variableAccess.range->second, evaluationErrorPositionHelper);
    if (bitRangeStartEvaluated.has_value() && bitRangeEndEvaluated.has_value()) {
        return std::make_optional((*bitRangeEndEvaluated - *bitRangeStartEvaluated) + 1);
    }
    if (variableAccess.range->first == variableAccess.range->second) {
        return std::make_optional(1);
    }
    return std::nullopt;
}

std::optional<unsigned> SyReCCustomBaseVisitor::tryDetermineExpressionBitwidth(const syrec::Expression& expression, const messageUtils::Message::Position& evaluationErrorPosition) {
    if (auto const* numericExpression = dynamic_cast<const syrec::NumericExpression*>(&expression)) {
        return tryEvaluateNumber(*numericExpression->value, &evaluationErrorPosition);
    }

    if (auto const* binaryExpression = dynamic_cast<const syrec::BinaryExpression*>(&expression)) {
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
                const auto& lhsBitWidthEvaluated = tryDetermineExpressionBitwidth(*binaryExpression->lhs, evaluationErrorPosition);
                const auto& rhsBitWidthEvaluated = lhsBitWidthEvaluated.has_value() ? lhsBitWidthEvaluated : tryDetermineExpressionBitwidth(*binaryExpression->rhs, evaluationErrorPosition);

                if (lhsBitWidthEvaluated.has_value() && rhsBitWidthEvaluated.has_value()) {
                    return std::max(*lhsBitWidthEvaluated, *rhsBitWidthEvaluated);
                }

                return lhsBitWidthEvaluated.has_value() ? *lhsBitWidthEvaluated : *rhsBitWidthEvaluated;
        }
    }

    if (auto const* shiftExpression = dynamic_cast<const syrec::ShiftExpression*>(&expression)) {
        return tryDetermineExpressionBitwidth(*shiftExpression->lhs, evaluationErrorPosition);
    }

    if (auto const* variableExpression = dynamic_cast<const syrec::VariableExpression*>(&expression)) {
        if (!variableExpression->var->range.has_value()) {
            return variableExpression->bitwidth();
        }

        const auto& bitRangeStart = variableExpression->var->range->first;
        const auto& bitRangeEnd   = variableExpression->var->range->second;

        if (bitRangeStart->isLoopVariable() && bitRangeEnd->isLoopVariable() && bitRangeStart->variableName() == bitRangeEnd->variableName()) {
            return 1;
        }
        if (bitRangeStart->isConstant() && bitRangeEnd->isConstant()) {
            return (bitRangeEnd->evaluate({}) - bitRangeStart->evaluate({})) + 1;
        }

        return variableExpression->var->bitwidth();
    }

    createError(evaluationErrorPosition,  "Could not determine bitwidth for unknown type of expression");
    return std::nullopt;
}

std::optional<std::pair<syrec::Number::ptr, syrec::Number::ptr>> SyReCCustomBaseVisitor::isBitOrRangeAccessDefined(SyReCParser::NumberContext* bitRangeStartToken, SyReCParser::NumberContext* bitRangeEndToken) {
    if (!bitRangeStartToken && !bitRangeEndToken) {
        return std::nullopt;
    }

    const auto bitRangeStartEvaluated = tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(bitRangeStartToken);
    const auto bitRangeEndEvaluated   = bitRangeEndToken? tryVisitAndConvertProductionReturnValue<syrec::Number::ptr>(bitRangeEndToken) : bitRangeStartEvaluated;

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
            createError(mapAntlrTokenPosition(bitOrRangeStartToken),  BitAccessOutOfRange, bitOrRangeAccessPairEvaluated.first, signalIdent, constraintsForBitAccess.minimumValidValue, constraintsForBitAccess.maximumValidValue);
        }
    } else {
        if (bitOrRangeAccessPairEvaluated.first > bitOrRangeAccessPairEvaluated.second) {
            isValidSignalAccess = false;
            // TODO: GEN_ERROR: Bit range start larger than end
            createError(mapAntlrTokenPosition(bitOrRangeStartToken), BitRangeStartLargerThanEnd, bitOrRangeAccessPairEvaluated.first, bitOrRangeAccessPairEvaluated.second);
        } else if (!isValidBitRangeAccess(accessedVariable, bitOrRangeAccessPairEvaluated, true)) {
            isValidSignalAccess                                           = false;
            const IndexAccessRangeConstraint constraintsForBitRangeAccess = getConstraintsForValidBitAccess(accessedVariable, true);
            // TODO: GEN_ERROR: Bit range out of range
            createError(mapAntlrTokenPosition(bitOrRangeStartToken),  BitRangeOutOfRange, bitOrRangeAccessPairEvaluated.first, bitOrRangeAccessPairEvaluated.second, signalIdent, constraintsForBitRangeAccess.minimumValidValue, constraintsForBitRangeAccess.maximumValidValue);
        }
    }
    return isValidSignalAccess;
}

std::optional<bool> SyReCCustomBaseVisitor::isAccessedValueOfDimensionWithinRange(const messageUtils::Message::Position& valueOfDimensionTokenPosition, size_t accessedDimensionIdx, const syrec::Variable::ptr& accessedSignal, const ExpressionEvaluationResult& expressionEvaluationResult) {
    if (!expressionEvaluationResult.isConstantValue()) {
        return std::nullopt;
    }

    const auto expressionResultAsConstant = expressionEvaluationResult.getAsConstant().value();
    const auto accessOnCurrentDimensionOk = isValidDimensionAccess(accessedSignal, accessedDimensionIdx, expressionResultAsConstant, true);
    if (!accessOnCurrentDimensionOk) {
        // TODO: Using global flag indicating zero_based indexing or not
        const auto constraintForCurrentDimension = getConstraintsForValidDimensionAccess(accessedSignal, accessedDimensionIdx, true).value();

        // TODO: GEN_ERROR: Index out of range for dimension i
        // TODO: Error position
        createError(valueOfDimensionTokenPosition,
                      
                      DimensionValueOutOfRange,
                      expressionResultAsConstant,
                      accessedDimensionIdx,
                      accessedSignal->name,
                      constraintForCurrentDimension.minimumValidValue, constraintForCurrentDimension.maximumValidValue);
    }

    return std::make_optional(accessOnCurrentDimensionOk);
}

bool SyReCCustomBaseVisitor::existsRestrictionForAccessedSignalParts(const syrec::VariableAccess& accessedSignalPart, const messageUtils::Message::Position* optionalEvaluationErrorPosition) {
    if (const auto& existingSignalAccessRestriction = sharedData->getSignalAccessRestriction(); existingSignalAccessRestriction.has_value()) {
        const auto& signalAccessOverlapCheckResult = SignalAccessUtils::areSignalAccessesEqual(
                accessedSignalPart,
                *existingSignalAccessRestriction,
                SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::DimensionAccess::Overlapping,
                SignalAccessUtils::SignalAccessComponentEquivalenceCriteria::BitRange::Overlapping,
                *sharedData->currentSymbolTableScope);

        if (!(signalAccessOverlapCheckResult.isResultCertain && signalAccessOverlapCheckResult.equality == SignalAccessUtils::SignalAccessEquivalenceResult::NotEqual)) {
            if (optionalEvaluationErrorPosition) {
                createError(*optionalEvaluationErrorPosition, AccessingRestrictedPartOfSignal, accessedSignalPart.var->name);
            }
            return true;
        }
    }
    return false;
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

std::optional<syrec::BinaryExpression::ptr> SyReCCustomBaseVisitor::tryConvertCompileTimeConstantExpressionToBinaryExpr(const syrec::Number::CompileTimeConstantExpression& compileTimeConstantExpression, const parser::SymbolTable& symbolTable) {
    const auto& lhsOperandConverted = tryConvertNumberToExpr(*compileTimeConstantExpression.lhsOperand, symbolTable);
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
    const auto& rhsOperandConverted = tryConvertNumberToExpr(*compileTimeConstantExpression.rhsOperand, symbolTable);
    if (lhsOperandConverted.has_value() && mappedToBinaryOperation.has_value() && rhsOperandConverted.has_value()) {
        return std::make_optional(
            std::make_shared<syrec::BinaryExpression>(*lhsOperandConverted, *syrec_operation::tryMapBinaryOperationEnumToFlag(*mappedToBinaryOperation), *rhsOperandConverted)
        );
    }
    return std::nullopt;
}

std::optional<syrec::Number::ptr> SyReCCustomBaseVisitor::tryConvertExpressionToCompileTimeConstantOne(const syrec::Expression& expr) {
    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&expr); exprAsBinaryExpr != nullptr) {
        const auto& convertedLhsOperand = tryConvertExpressionToCompileTimeConstantOne(*exprAsBinaryExpr->lhs);
        const auto& convertedBinaryExpr = syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
        const auto& convertedRhsOperand = tryConvertExpressionToCompileTimeConstantOne(*exprAsBinaryExpr->rhs);

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
    if (const auto& exprAsVariableExpr = dynamic_cast<const syrec::VariableExpression*>(&expr); exprAsVariableExpr != nullptr) {
        if (exprAsVariableExpr->var->indexes.empty() && !exprAsVariableExpr->var->range.has_value()) {
            return std::make_optional(std::make_shared<syrec::Number>(exprAsVariableExpr->var->getVar()->name));
        }
    }
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr != nullptr) {
        return std::make_optional(exprAsNumericExpr->value);
    }
    return std::nullopt;
}

std::optional<syrec::Expression::ptr> SyReCCustomBaseVisitor::tryConvertNumberToExpr(const syrec::Number& number, const parser::SymbolTable& symbolTable) {
    if (number.isConstant()) {
        if (const auto& constantValueOfNumber = SignalAccessUtils::tryEvaluateNumber(number, symbolTable); constantValueOfNumber.has_value()) {
            return std::make_shared<syrec::NumericExpression>(std::make_shared<syrec::Number>(*constantValueOfNumber), BitHelpers::getRequiredBitsToStoreValue(*constantValueOfNumber));
        }
        return std::nullopt;
    }
    if (number.isLoopVariable()) {
        // TODO: Is it ok to assume a bitwidth of 1 for the given loop variable
        return std::make_shared<syrec::NumericExpression>(std::make_shared<syrec::Number>(number.variableName()), 1);
    }
    if (number.isCompileTimeConstantExpression()) {
        return tryConvertCompileTimeConstantExpressionToBinaryExpr(number.getExpression(), symbolTable);
    }
    return std::nullopt;
}

/*
 * Usage of string_view requires a workaround to make usage of the std::from_chars(...) function, for more details see:
 * https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p2007r0.html
 */
std::optional<unsigned int> SyReCCustomBaseVisitor::tryConvertTextToNumber(const std::string_view& text) {
    unsigned int numericValueOfText = 0;
    if (const auto& conversionResult = std::from_chars(text.data(), text.data() + text.size(), numericValueOfText); conversionResult.ec == std::errc()) {
        return numericValueOfText;
    }
    return std::nullopt;
}

std::vector<std::optional<unsigned>> SyReCCustomBaseVisitor::determineAccessedValuePerDimensionFromSignalAccess(const syrec::VariableAccess& signalAccess) {
    std::vector<std::optional<unsigned int>> resultContainer;
    resultContainer.reserve(signalAccess.indexes.size());
    std::transform(
    signalAccess.indexes.cbegin(),
    signalAccess.indexes.cend(),
    std::back_inserter(resultContainer),
    [&](const syrec::Expression::ptr& exprDefiningAccessedValueOfDimension) -> std::optional<unsigned int> {
        if (const auto& exprAsNumericExpr = std::dynamic_pointer_cast<const syrec::NumericExpression>(exprDefiningAccessedValueOfDimension); exprAsNumericExpr != nullptr) {
            return tryEvaluateNumber(*exprAsNumericExpr->value, nullptr);
        }
        return std::nullopt;
    });   
    return resultContainer;
}

std::pair<std::vector<unsigned>, std::size_t> SyReCCustomBaseVisitor::determineNumAccessedValuesPerDimensionAndFirstNotExplicitlyAccessedDimensionIndex(const syrec::VariableAccess& signalAccess) {
    const auto&               numDeclaredDimensionsOfAccessedSignal = signalAccess.var->dimensions.size();
    std::vector<unsigned int> resultContainer(numDeclaredDimensionsOfAccessedSignal, 1);
    if (signalAccess.indexes.size() < numDeclaredDimensionsOfAccessedSignal) {
        const auto& indexOfFirstNotExplicitlyAccessedDimension = signalAccess.indexes.size();
        for (std::size_t i = indexOfFirstNotExplicitlyAccessedDimension; i < numDeclaredDimensionsOfAccessedSignal; ++i) {
            resultContainer[i] = signalAccess.var->dimensions[i];
        }
        return std::make_pair(resultContainer, indexOfFirstNotExplicitlyAccessedDimension);
    }
    return std::make_pair(resultContainer, numDeclaredDimensionsOfAccessedSignal);
}

bool SyReCCustomBaseVisitor::doOperandsRequireBroadcastingBasedOnDimensionAccessAndLogError(std::size_t numDeclaredDimensionsOfLhsOperand, std::size_t firstNoExplicitlyAccessedDimensionOfLhsOperand, const std::vector<unsigned int>& numAccessedValuesPerDimensionOfLhsOperand,
                                                                                            std::size_t numDeclaredDimensionsOfRhsOperand, std::size_t firstNoExplicitlyAccessedDimensionOfRhsOperand, const std::vector<unsigned int>& numAccessedValuesPerDimensionOfRhsOperand, const messageUtils::Message::Position* broadcastingErrorPosition, valueBroadcastCheck::DimensionAccessMissmatchResult* missmatchResult) {
    const auto& broadcastCheckResult = valueBroadcastCheck::determineMissmatchesInAccessedValuesPerDimension(
            numDeclaredDimensionsOfLhsOperand, firstNoExplicitlyAccessedDimensionOfLhsOperand, numAccessedValuesPerDimensionOfLhsOperand,
            numDeclaredDimensionsOfRhsOperand, firstNoExplicitlyAccessedDimensionOfRhsOperand, numAccessedValuesPerDimensionOfRhsOperand);

    if (!broadcastCheckResult.has_value()) {
        return false;
    }

    if (missmatchResult) {
        *missmatchResult = *broadcastCheckResult;
    }

    if (!broadcastingErrorPosition) {
        return true;
    }

    if (sharedData->parserConfig->supportBroadcastingExpressionOperands) {
        createError(*broadcastingErrorPosition, "Broadcasting of operands is currently not supported");
        return true;
    }

    switch (broadcastCheckResult->missmatchReason) {
        case valueBroadcastCheck::DimensionAccessMissmatchResult::MissmatchReason::SizeOfResult: {
            createError(*broadcastingErrorPosition, MissmatchedNumberOfDimensionsBetweenOperands, broadcastCheckResult->numNotExplicitlyAccessedDimensionPerOperand.first, broadcastCheckResult->numNotExplicitlyAccessedDimensionPerOperand.second);
            break;
        }
        case valueBroadcastCheck::DimensionAccessMissmatchResult::MissmatchReason::AccessedValueOfAnyDimension: {
            createError(*broadcastingErrorPosition, MissmatchedNumberOfValuesForDimensionsBetweenOperands, combineValuePerDimensionMissmatch(broadcastCheckResult->valuePerNotExplicitlyAccessedDimensionMissmatch));
            break;
        }
        default: {
            createError(*broadcastingErrorPosition, FallbackBroadcastCheckFailedErrorBasedOnDimensionAccess);   
            break;
        }
    }
    return true;
}

std::optional<bool> SyReCCustomBaseVisitor::isSizeOfSignalAcessResult1DSignalOrLogError(std::size_t numDeclaredDimensionsOfLhsOperand, std::size_t firstNoExplicitlyAccessedDimensionOfLhsOperand, const std::vector<unsigned int>& numAccessedValuesPerDimension, const messageUtils::Message::Position* errorPosition) {
    if (numAccessedValuesPerDimension.size() > numDeclaredDimensionsOfLhsOperand || firstNoExplicitlyAccessedDimensionOfLhsOperand > numDeclaredDimensionsOfLhsOperand) {
        return std::nullopt;
    }

    if (const auto& sizeOfOperandAfterSignalAccess = std::max((!firstNoExplicitlyAccessedDimensionOfLhsOperand ? numDeclaredDimensionsOfLhsOperand : numDeclaredDimensionsOfLhsOperand - firstNoExplicitlyAccessedDimensionOfLhsOperand), static_cast<std::size_t>(1))
        ; sizeOfOperandAfterSignalAccess != 1) {
        if (errorPosition) {
            createError(*errorPosition, OperandIsNot1DSignal, sizeOfOperandAfterSignalAccess);
        }
        return false;
    }

    if (const auto& numAccessedValuesOfLastAccessedDimension = !firstNoExplicitlyAccessedDimensionOfLhsOperand ? numAccessedValuesPerDimension.front() : numAccessedValuesPerDimension.at(firstNoExplicitlyAccessedDimensionOfLhsOperand - 1); numAccessedValuesOfLastAccessedDimension != 1) {
        if (errorPosition) {
            createError(*errorPosition, OneDimensionalOperandDoesAccessMoreThanOneValueOfDimension, numAccessedValuesOfLastAccessedDimension);
        }
    }
    
    return true;
}


bool SyReCCustomBaseVisitor::doOperandsRequiredBroadcastingBasedOnBitwidthAndLogError(std::size_t accessedBitRangeWidthOfLhsOperand, std::size_t accessedBitRangeWidthOfRhsOperand, const messageUtils::Message::Position* broadcastingErrorPosition) {
    if (accessedBitRangeWidthOfLhsOperand != accessedBitRangeWidthOfRhsOperand) {
        if (broadcastingErrorPosition) {
            createError(*broadcastingErrorPosition, InvalidSwapSignalWidthMissmatch, accessedBitRangeWidthOfLhsOperand, accessedBitRangeWidthOfRhsOperand);         
        }
        return true;
    }
    return false;
}

void SyReCCustomBaseVisitor::fixExpectedBitwidthToValueIfLatterIsLargerThanCurrentOne(unsigned potentialNewExpectedExpressionBitwidth) const {
    if (!sharedData->optionalExpectedExpressionSignalWidth.has_value() || *sharedData->optionalExpectedExpressionSignalWidth < potentialNewExpectedExpressionBitwidth) {
        sharedData->optionalExpectedExpressionSignalWidth = potentialNewExpectedExpressionBitwidth;
    }
}


std::string SyReCCustomBaseVisitor::combineValuePerDimensionMissmatch(const std::vector<valueBroadcastCheck::AccessedValuesOfDimensionMissmatch>& valuePerNotExplicitlyAccessedDimensionMissmatch) {
    std::vector<std::string> errorContainer;
    errorContainer.reserve(valuePerNotExplicitlyAccessedDimensionMissmatch.size());
    
    std::transform(
    valuePerNotExplicitlyAccessedDimensionMissmatch.cbegin(),
    valuePerNotExplicitlyAccessedDimensionMissmatch.cend(),
    std::back_inserter(errorContainer),
    [](const valueBroadcastCheck::AccessedValuesOfDimensionMissmatch& affectedValuesPerDimensionMissmatch) {
        return fmt::format(MissmatchedNumberOfValuesForDimension, affectedValuesPerDimensionMissmatch.dimensionIdx, affectedValuesPerDimensionMissmatch.expectedNumberOfValues, affectedValuesPerDimensionMissmatch.actualNumberOfValues);
    });

    std::ostringstream errorsConcatinatedBuffer;
    std::copy(errorContainer.cbegin(), errorContainer.cend(), InfixIterator<std::string>(errorsConcatinatedBuffer, ","));
    return errorsConcatinatedBuffer.str();
}