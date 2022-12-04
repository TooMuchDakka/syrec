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
    
    return declaredSignalsOfType;
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
                    accessedSignal.value()->indexes.emplace_back(accessedDimensionExpression->getOrConvertToExpression(0).value());
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

std::any SyReCCustomVisitor::visitNumber(SyReCParser::NumberContext* context) {
    return std::nullopt;
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

    try {
        return std::stoul(token->getText()) & UINT_MAX;
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

[[nodiscard]] bool SyReCCustomVisitor::validateSemanticChecksIfDimensionExpressionIsConstant(const size_t accessedDimensionIdx, const syrec::Variable::ptr& accessedSignal, const ExpressionEvaluationResult::ptr& expressionEvaluationResult) {
    if (!expressionEvaluationResult->hasValue()) {
        return false;
    }

    if (!expressionEvaluationResult->evaluatedToConstant()) {
        return true;
    }

    const auto expressionResultAsConstant = expressionEvaluationResult->getAsConstant().value();
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