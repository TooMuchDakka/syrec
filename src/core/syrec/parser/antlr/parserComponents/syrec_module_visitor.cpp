#include "core/syrec/parser/antlr/parserComponents/syrec_module_visitor.hpp"

#include "core/syrec/variable.hpp"
#include "core/syrec/parser/custom_semantic_errors.hpp"
#include "core/syrec/parser/optimizations/constantPropagation/valueLookup/signal_value_lookup.hpp"

#include <fmt/format.h>

using namespace parser;

std::vector<messageUtils::Message> SyReCModuleVisitor::getErrors() const {
    return sharedData->errors;
}

std::vector<messageUtils::Message> SyReCModuleVisitor::getWarnings() const {
    return sharedData->warnings;
}

syrec::Module::vec       SyReCModuleVisitor::getFoundModules() const {
    return foundModules;
}

std::any SyReCModuleVisitor::visitProgram(SyReCParser::ProgramContext* context) {
    SymbolTable::openScope(sharedData->currentSymbolTableScope);

    syrec::Module::vec unoptimizedModules;
    for (std::size_t i = 0; i < context->module().size(); ++i) {
        const auto module = context->module().at(i);
        /*
         * We have moved the opening and closing of a new symbol table scope from the module visitor here, into the parent,
         * to not lose information about the declared symbols of the module that is required for some optimizations
         */
        SymbolTable::openScope(sharedData->currentSymbolTableScope);
        const auto moduleParseResult = tryVisitAndConvertProductionReturnValue<syrec::Module::ptr>(module);
        bool       alreadyClosedOpenedSymbolTableScope = false;

        if (moduleParseResult.has_value()) {
            if (sharedData->currentSymbolTableScope->contains(*moduleParseResult)) {
                createMessage(mapAntlrTokenPosition(module->IDENT()->getSymbol()), messageUtils::Message::Severity::Error, DuplicateModuleIdentDeclaration, (*moduleParseResult)->name);
            } else {
                const auto& unoptimizedModuleVersion       = *moduleParseResult;
                auto        optimizedModuleVersion         = unoptimizedModuleVersion;
                std::vector<std::size_t> indicesOfOptimizedAwayParameters;
                if (sharedData->parserConfig->deadCodeEliminationEnabled) {
                    optimizedModuleVersion = createCopyOfModule(unoptimizedModuleVersion);
                    // TODO: UNUSED_REFERENCE - Marked as used
                    removeUnusedVariablesAndParametersFromModule(optimizedModuleVersion);
                    indicesOfOptimizedAwayParameters = determineUnusedParametersBetweenModuleVersions(unoptimizedModuleVersion, optimizedModuleVersion);
                }
                alreadyClosedOpenedSymbolTableScope = true;
                SymbolTable::closeScope(sharedData->currentSymbolTableScope);
                sharedData->currentSymbolTableScope->addEntry(unoptimizedModuleVersion, indicesOfOptimizedAwayParameters);
                unoptimizedModules.emplace_back(unoptimizedModuleVersion);
                foundModules.emplace_back(optimizedModuleVersion);   
            }
        }

        if (!alreadyClosedOpenedSymbolTableScope) {
            SymbolTable::closeScope(sharedData->currentSymbolTableScope);
        }
    }
    
    const auto& nameOfTopLevelModule = determineExpectedNameOfTopLevelModule();
    // TODO: UNUSED_REFERENCE - Marked as used
    if (sharedData->parserConfig->deadCodeEliminationEnabled) {
        removeUnusedOptimizedModulesWithHelpOfInformationOfUnoptimized(unoptimizedModules, foundModules, nameOfTopLevelModule);
        /*
         * If a module consists of only dead stores, all parameters as well as locals should already be removed and thus the module should be removed by this call
         * and thus no extra check is required after the dead store optimization
         */
        removeModulesWithoutParameters(foundModules, nameOfTopLevelModule);
        // TODO: Remove now empty modules after dead code elimination (either here or at pos P1)?
    }
    addSkipStatementToMainModuleIfEmpty(foundModules, nameOfTopLevelModule);
    return 0;
}

std::any SyReCModuleVisitor::visitModule(SyReCParser::ModuleContext* context) {
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

    auto validUserDefinedModuleStatements = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(context->statementList());
    //isDeclaredModuleValid &= validUserDefinedModuleStatements.has_value() && !validUserDefinedModuleStatements->empty();
    
    sharedData->currentModuleCallNestingLevel--;

    if (isDeclaredModuleValid) {
        lastDeclaredModuleIdent = moduleIdent;
        if (validUserDefinedModuleStatements.has_value()) {
            if (sharedData->parserConfig->deadStoreEliminationEnabled && sharedData->optionalDeadStoreEliminator.has_value()) {
                auto        statementsToProcess = *std::move(validUserDefinedModuleStatements);
                validUserDefinedModuleStatements.reset();
                sharedData->optionalDeadStoreEliminator.value()->removeDeadStoresFrom(statementsToProcess);

                // TODO: What if there are no remaining statements in the module body after the dead store elimination, currently we opt to simply insert a skip statement
                if (statementsToProcess.empty()) {
                    statementsToProcess.emplace_back(std::make_shared<syrec::SkipStatement>());
                }
                module->statements = statementsToProcess;
            } else {
                module->statements = *validUserDefinedModuleStatements;      
            }
        }
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
        createMessage(mapAntlrTokenPosition(context->start), messageUtils::Message::Severity::Error, InvalidParameterType);
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
        createMessage(mapAntlrTokenPosition(context->start), messageUtils::Message::Severity::Error, InvalidLocalType);
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
        createMessage(mapAntlrTokenPosition(context->IDENT()->getSymbol()), messageUtils::Message::Severity::Error, DuplicateDeclarationOfIdent, signalIdent);
    }
    
    for (const auto& dimensionToken : context->dimensionTokens) {
        const auto dimensionTokenValueAsNumber = dimensionToken != nullptr
            ? convertToNumber(dimensionToken->getText())
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
        const std::optional<unsigned int> customSignalWidth = convertToNumber(context->signalWidthToken->getText());
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
void SyReCModuleVisitor::removeUnusedOptimizedModulesWithHelpOfInformationOfUnoptimized(const syrec::Module::vec& unoptimizedModules, syrec::Module::vec& optimizedModules, const std::string_view& expectedIdentOfTopLevelModuleToNotRemove) const {
    const auto& wasUsedStatusPerFoundModule = sharedData->currentSymbolTableScope->determineIfModuleWasUsed(unoptimizedModules);
    std::size_t moduleIdx                   = 0;

    for (auto foundModule = optimizedModules.begin(); foundModule != optimizedModules.end(); moduleIdx++) {
        if (foundModule->get()->name == expectedIdentOfTopLevelModuleToNotRemove 
            || wasUsedStatusPerFoundModule.at(moduleIdx)) {
            ++foundModule;
        }
        else {
            foundModule = optimizedModules.erase(foundModule);   
        }
    }
}

// TODO: UNUSED_REFERENCE - Marked as used
void SyReCModuleVisitor::removeModulesWithoutParameters(syrec::Module::vec& modules, const std::string_view& expectedIdentOfTopLevelModuleToNotRemove) const {
    modules.erase(
    std::remove_if(
            modules.begin(),
            modules.end(),
            [&expectedIdentOfTopLevelModuleToNotRemove](const syrec::Module::ptr& module) {
                return module->name != expectedIdentOfTopLevelModuleToNotRemove && module->parameters.empty();
            }),
    modules.end());
}

std::string SyReCModuleVisitor::determineExpectedNameOfTopLevelModule() const {
    const auto& defaultExpectedTopLevelModuleName              = sharedData->parserConfig->expectedMainModuleName;
    const auto doesModuleWithExpectedDefaultTopLevelNameExist = std::any_of(
             foundModules.cbegin(),
             foundModules.cend(),
             [&defaultExpectedTopLevelModuleName](const syrec::Module::ptr& module) {
                return module->name == defaultExpectedTopLevelModuleName;
            });
    return doesModuleWithExpectedDefaultTopLevelNameExist && !lastDeclaredModuleIdent.empty() ? defaultExpectedTopLevelModuleName : lastDeclaredModuleIdent;
}

syrec::Module::ptr SyReCModuleVisitor::createCopyOfModule(const syrec::Module::ptr& moduleToCreateCopyFrom) const {
    const syrec::Module::ptr createCopyOfModule = std::make_shared<syrec::Module>(moduleToCreateCopyFrom->name);
    for (const auto& parameterToCopy : moduleToCreateCopyFrom->parameters) {
        createCopyOfModule->addParameter(parameterToCopy);
    }

    for (const auto& localToCopy : moduleToCreateCopyFrom->variables) {
        createCopyOfModule->addVariable(localToCopy);
    }

    for (const auto& statement : moduleToCreateCopyFrom->statements) {
        createCopyOfModule->addStatement(statement);
    }
    return createCopyOfModule;
}

std::vector<std::size_t> SyReCModuleVisitor::determineUnusedParametersBetweenModuleVersions(const syrec::Module::ptr& unoptimizedModule, const syrec::Module::ptr& optimizedModule) const {
    std::set<std::string> setOfParameterNamesOfOptimizedModule;
    std::transform(
    optimizedModule->parameters.cbegin(),
    optimizedModule->parameters.cend(),
    std::inserter(setOfParameterNamesOfOptimizedModule, setOfParameterNamesOfOptimizedModule.begin()),
    [](const syrec::Variable::ptr& variable) {
        return variable->name;
    });

    std::vector<std::size_t> droppedParametersInOptimizedModule;
    for (std::size_t i = 0; i < unoptimizedModule->parameters.size(); ++i) {
        const auto& parameterName                        = unoptimizedModule->parameters.at(i)->name;
        if(setOfParameterNamesOfOptimizedModule.count(parameterName) == 0) {
            droppedParametersInOptimizedModule.emplace_back(i);
        }
    }
    return droppedParametersInOptimizedModule;
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

void SyReCModuleVisitor::addSkipStatementToMainModuleIfEmpty(const syrec::Module::vec& modules, const std::string_view& expectedIdentOfTopLevelModule) const {
    const auto& foundTopLevelModule = std::find_if(
        modules.begin(),
        modules.end(),
        [&expectedIdentOfTopLevelModule](const syrec::Module::ptr& module) {
            return module->name == expectedIdentOfTopLevelModule;
        }
    );
    if (foundTopLevelModule == modules.end() || !(*foundTopLevelModule)->statements.empty()) {
        return;
    }

    (*foundTopLevelModule)->statements.emplace_back(std::make_shared<syrec::SkipStatement>());
}