#include "core/syrec/parser/antlr/parserComponents/syrec_module_visitor.hpp"

#include "core/syrec/variable.hpp"
#include "core/syrec/parser/custom_semantic_errors.hpp"
#include "core/syrec/parser/parser_utilities.hpp"
#include "core/syrec/parser/optimizations/constantPropagation/valueLookup/signal_value_lookup.hpp"

#include <fmt/format.h>

using namespace parser;

std::vector<std::string> SyReCModuleVisitor::getErrors() const {
    return sharedData->errors;
}

std::vector<std::string> SyReCModuleVisitor::getWarnings() const {
    return sharedData->warnings;
}

syrec::Module::vec       SyReCModuleVisitor::getFoundModules() const {
    return foundModules;
}

std::any SyReCModuleVisitor::visitProgram(SyReCParser::ProgramContext* context) {
    const auto                                dimensions = std::vector<unsigned int>({2, 4, 3});
    const valueLookup::SignalValueLookup::ptr x = std::make_shared<valueLookup::SignalValueLookup>(valueLookup::SignalValueLookup(10, dimensions, 10)); 
    x->invalidateStoredValueFor({1, std::nullopt});
    const auto y = x->tryFetchValueFor({0, 3, 1}, std::nullopt);

    SymbolTable::openScope(sharedData->currentSymbolTableScope);
    for (const auto& module: context->module()) {
        const auto moduleParseResult = tryVisitAndConvertProductionReturnValue<syrec::Module::ptr>(module);
        if (moduleParseResult.has_value()) {
            if (sharedData->currentSymbolTableScope->contains(*moduleParseResult)) {
                createError(mapAntlrTokenPosition(module->IDENT()->getSymbol()), fmt::format(DuplicateModuleIdentDeclaration, (*moduleParseResult)->name));
            } else {
                sharedData->currentSymbolTableScope->addEntry(*moduleParseResult);
                foundModules.emplace_back(moduleParseResult.value());
            }
        }
    }

    // TODO: UNUSED_REFERENCE - Marked as used
    if (sharedData->parserConfig->isRemovalOfUnusedVariablesAndModulesEnabled) {
        removeUnusedModules();
        removeModulesWithoutParameters();
    }

    return 0;
}

std::any SyReCModuleVisitor::visitModule(SyReCParser::ModuleContext* context) {
    SymbolTable::openScope(sharedData->currentSymbolTableScope);
    sharedData->currentModuleCallNestingLevel++;

    // TODO: Wrap into optional, since token could be null if no alternative rule is found and the moduleProduction is choosen instead (as the first alternative from the list of possible ones if nothing else matches)
    const std::string  moduleIdent = context->IDENT()->getText();
    syrec::Module::ptr module      = std::make_shared<syrec::Module>(moduleIdent);

    const auto declaredParameters    = tryVisitAndConvertProductionReturnValue<syrec::Variable::vec>(context->parameterList());
    bool       isDeclaredModuleValid = context->parameterList() != nullptr ? declaredParameters.has_value() : true;

    if (isDeclaredModuleValid && declaredParameters.has_value()) {
        for (const auto& declaredParameter: declaredParameters.value()) {
            module->addParameter(declaredParameter);
        }
    }

    for (const auto& declaredSignals: context->signalList()) {
        const auto parsedDeclaredSignals = tryVisitAndConvertProductionReturnValue<syrec::Variable::vec>(declaredSignals);
        isDeclaredModuleValid &= parsedDeclaredSignals.has_value();
        if (isDeclaredModuleValid) {
            for (const auto& declaredSignal: *parsedDeclaredSignals) {
                module->addVariable(declaredSignal);
            }
        }
    }

    const auto validUserDefinedModuleStatements = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(context->statementList());
    isDeclaredModuleValid &= validUserDefinedModuleStatements.has_value() && !validUserDefinedModuleStatements->empty();

    // TODO: UNUSED_REFERENCE - Marked as used
    if (isDeclaredModuleValid && sharedData->parserConfig->isRemovalOfUnusedVariablesAndModulesEnabled) {
        removeUnusedVariablesAndParametersFromModule(module);
    }

    SymbolTable::closeScope(sharedData->currentSymbolTableScope);
    sharedData->currentModuleCallNestingLevel--;

    if (isDeclaredModuleValid) {
        module->statements = *validUserDefinedModuleStatements;
        return std::make_optional(module);
    }
    return std::nullopt;
}

std::any SyReCModuleVisitor::visitParameterList(SyReCParser::ParameterListContext* context) {
    syrec::Variable::vec                parametersContainer;

    for (const auto& parameter: context->parameter()) {
        const auto parsedParameterDefinition = tryVisitAndConvertProductionReturnValue<syrec::Variable::ptr>(parameter);
        if (parsedParameterDefinition.has_value()) {
            parametersContainer.emplace_back(*parsedParameterDefinition);
        }
    }
    const bool areAllParametersValid = parametersContainer.size() == context->parameter().size();
    return areAllParametersValid ? std::make_optional(parametersContainer) : std::nullopt;
}

std::any SyReCModuleVisitor::visitParameter(SyReCParser::ParameterContext* context) {
    const auto parameterType = getParameterType(context->start);
    if (!parameterType.has_value()) {
        createError(mapAntlrTokenPosition(context->start), InvalidParameterType);
    }

    auto declaredParameter = tryVisitAndConvertProductionReturnValue<syrec::Variable::ptr>(context->signalDeclaration());
    if (!parameterType.has_value() || !declaredParameter.has_value()) {
        return std::nullopt;
    }

    (*declaredParameter)->type = *parameterType;
    sharedData->currentSymbolTableScope->addEntry(*declaredParameter);
    return declaredParameter;
}

std::any SyReCModuleVisitor::visitSignalList(SyReCParser::SignalListContext* context) {
    const auto declaredSignalsType          = getSignalType(context->start);
    bool       isValidSignalListDeclaration = true;
    if (!declaredSignalsType.has_value()) {
        createError(mapAntlrTokenPosition(context->start), InvalidLocalType);
        isValidSignalListDeclaration = false;
    }

    syrec::Variable::vec declaredSignalsOfType{};
    for (const auto& signal: context->signalDeclaration()) {
        const auto declaredSignal = tryVisitAndConvertProductionReturnValue<syrec::Variable::ptr>(signal);
        isValidSignalListDeclaration &= declaredSignal.has_value();

        if (isValidSignalListDeclaration) {
            (*declaredSignal)->type = *declaredSignalsType;
            declaredSignalsOfType.emplace_back(*declaredSignal);
            sharedData->currentSymbolTableScope->addEntry(*declaredSignal);
        }
    }

    return std::make_optional(declaredSignalsOfType);
}

std::any SyReCModuleVisitor::visitSignalDeclaration(SyReCParser::SignalDeclarationContext* context) {
    std::optional<syrec::Variable::ptr> signal;
    std::vector<unsigned int>           dimensions{};
    unsigned int                        signalWidth              = sharedData->parserConfig->cDefaultSignalWidth;
    bool                                isValidSignalDeclaration = true;

    const std::string signalIdent = context->IDENT()->getText();
    if (sharedData->currentSymbolTableScope->contains(signalIdent)) {
        isValidSignalDeclaration = false;
        createError(mapAntlrTokenPosition(context->IDENT()->getSymbol()), fmt::format(DuplicateDeclarationOfIdent, signalIdent));
    }
    
    for (const auto& dimensionToken : context->dimensionTokens) {
        const auto dimensionTokenValueAsNumber = dimensionToken != nullptr
            ? ParserUtilities::convertToNumber(dimensionToken->getText())
            : std::nullopt;

        isValidSignalDeclaration = dimensionTokenValueAsNumber.has_value();
        if (isValidSignalDeclaration) {
            dimensions.emplace_back(*dimensionTokenValueAsNumber);
        }
    }

    if (isValidSignalDeclaration && dimensions.empty()) {
        dimensions.emplace_back(1);
    }

    if (context->signalWidthToken != nullptr) {
        const std::optional<unsigned int> customSignalWidth = ParserUtilities::convertToNumber(context->signalWidthToken->getText());
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

std::any SyReCModuleVisitor::visitStatementList(SyReCParser::StatementListContext* context) {
    return statementVisitor->visitStatementList(context);
}

void SyReCModuleVisitor::removeUnusedVariablesAndParametersFromModule(const syrec::Module::ptr& module) const {
    // TODO: UNUSED_REFERENCE - Marked as used
    const auto&                        unusedModuleLocalsOrVariables = sharedData->currentSymbolTableScope->getUnusedLiterals();

    // Use the erase/remove idiom
    module->parameters.erase(
        std::remove_if(
                module->parameters.begin(),
                module->parameters.end(),
                [&unusedModuleLocalsOrVariables](const syrec::Variable::ptr& moduleParameter) {
                        return unusedModuleLocalsOrVariables.find(moduleParameter->name) != unusedModuleLocalsOrVariables.end();
                }),
        module->parameters.end());

    module->variables.erase(
    std::remove_if(
            module->variables.begin(),
            module->variables.end(),
            [&unusedModuleLocalsOrVariables](const syrec::Variable::ptr& moduleParameter) {
                return unusedModuleLocalsOrVariables.find(moduleParameter->name) != unusedModuleLocalsOrVariables.end();
            }),
    module->variables.end());
}

// TODO: UNUSED_REFERENCE - Marked as used
void SyReCModuleVisitor::removeUnusedModules() {
    const auto& wasUsedStatusPerFoundModule = sharedData->currentSymbolTableScope->determineIfModuleWasUsed(foundModules);
    std::size_t moduleIdx                   = 0;

    for (auto foundModule = foundModules.begin(); foundModule != foundModules.end();) {
        if (!wasUsedStatusPerFoundModule.at(moduleIdx)) {
            foundModules.erase(foundModule);
        } else {
            ++foundModule;
        }
        moduleIdx++;
    }
}

// TODO: UNUSED_REFERENCE - Marked as used
void SyReCModuleVisitor::removeModulesWithoutParameters() {
    foundModules.erase(
    std::remove_if(
            foundModules.begin(),
            foundModules.end(),
            [](const syrec::Module::ptr& module) {
                return module->parameters.empty();
            }),
    foundModules.end());
}

std::optional<syrec::Variable::Types> SyReCModuleVisitor::getParameterType(const antlr4::Token* token) {
    if (token == nullptr) {
        return std::nullopt;
    }

    std::optional<syrec::Variable::Types> parameterType;
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
            break;
    }
    return parameterType;
}

std::optional<syrec::Variable::Types> SyReCModuleVisitor::getSignalType(const antlr4::Token* token) {
    if (token == nullptr) {
        return std::nullopt;
    }

    std::optional<syrec::Variable::Types> signalType;
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