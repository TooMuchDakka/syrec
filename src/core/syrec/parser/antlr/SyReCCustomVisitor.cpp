#include <climits>
#include <cstdint>
#include <optional>
#include <string>
#include <fmt/format.h>
#include "core/syrec/parser/custom_semantic_errors.hpp"

#include "SyReCCustomVisitor.h"

using namespace parser;

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
    const auto declaredParameters = tryConvertProductionReturnValue<syrec::Variable::vec>(visit(context->parameterList()));
    bool              isDeclaredModuleValid = true;

    syrec::Module::ptr module = std::make_shared<syrec::Module>(moduleIdent);
    isDeclaredModuleValid &= declaredParameters.has_value();
    if (isDeclaredModuleValid) {
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
    const auto parameterType = tryConvertProductionReturnValue<syrec::Variable::Types>(visit(context->parameterType())); //std::any_cast<std::optional<syrec::Variable::Types>>(visit(context->parameterType()));
    if (!parameterType.has_value()) {
        createError(InvalidParameterType);
    } 

    auto declaredParameter = tryConvertProductionReturnValue<syrec::Variable::ptr>(visit(context->signalDeclaration()));
    if (declaredParameter.has_value()) {
        //declaredParameter.value()->type = parameterType;
        declaredParameter.value()->type = 0;
        currentSymbolTableScope->addEntry(declaredParameter.value());
    }
    return declaredParameter;
}

std::any SyReCCustomVisitor::visitInSignalType(SyReCParser::InSignalTypeContext* context) {
    return std::make_optional(syrec::Variable::Types::In);
}

std::any SyReCCustomVisitor::visitOutSignalType(SyReCParser::OutSignalTypeContext* context) {
    return std::make_optional(syrec::Variable::Types::Out);
}

std::any SyReCCustomVisitor::visitInoutSignalType(SyReCParser::InoutSignalTypeContext* context) {
    return std::make_optional(syrec::Variable::Types::Inout);
}

std::any SyReCCustomVisitor::visitSignalList(SyReCParser::SignalListContext* context) {
    bool                                isValidSignalListDeclaration = true;

    const auto declaredSignalsType = tryConvertProductionReturnValue<syrec::Variable::Types>(visit(context->signalType()));
    if (!declaredSignalsType.has_value()) {
        createError(InvalidLocalType);
        isValidSignalListDeclaration = false;
    }

    syrec::Variable::vec declaredSignalsOfType{};
    for (const auto& signal : context->signalDeclaration()) {
        const auto declaredSignal = tryConvertProductionReturnValue<syrec::Variable::ptr>(visit(signal));
        isValidSignalListDeclaration &= declaredSignal.has_value();

        if (isValidSignalListDeclaration) {
            //declaredSignal.value()       = declaredSignalsOfType.value();
            declaredSignal.value()->type = 0;
            declaredSignalsOfType.emplace_back(declaredSignal.value());
            currentSymbolTableScope->addEntry(declaredSignal.value());
        }
    }
    
    return declaredSignalsOfType;
}

std::any SyReCCustomVisitor::visitWireSignalType(SyReCParser::WireSignalTypeContext* context) {
    return syrec::Variable::Types::Wire;
}

std::any SyReCCustomVisitor::visitStateSignalType(SyReCParser::StateSignalTypeContext* context) {
    return syrec::Variable::Types::State;
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



