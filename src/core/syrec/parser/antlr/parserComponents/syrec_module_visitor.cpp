#include "core/syrec/parser/antlr/parserComponents/syrec_module_visitor.hpp"
#include "core/syrec/parser/custom_semantic_errors.hpp"

using namespace parser;

void SyReCModuleVisitor::sortErrorAccordingToPosition(std::vector<messageUtils::Message>& errors) {
    std::sort(errors.begin(), errors.end(), [](const messageUtils::Message& thisMsg, const messageUtils::Message& thatMsg) {
        return thisMsg.position.compare(thatMsg.position) < 0;
    });
}

std::vector<messageUtils::Message> SyReCModuleVisitor::getErrors() const {
    sortErrorAccordingToPosition(sharedData->errors);
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
    bool               wereSemanticChecksForCurrentModuleOk = true;
    for (const auto& moduleContextInformation: context->module()) {
        SymbolTable::openScope(sharedData->currentSymbolTableScope);

        if (const auto moduleParseResult = tryVisitAndConvertProductionReturnValue<syrec::Module::ptr>(moduleContextInformation); moduleParseResult.has_value()) {
            if (sharedData->currentSymbolTableScope->contains(*moduleParseResult)) {
                createError(mapAntlrTokenPosition(moduleContextInformation->IDENT()->getSymbol()),  DuplicateModuleIdentDeclaration, (*moduleParseResult)->name);
                wereSemanticChecksForCurrentModuleOk = false;
            }
            else {
                SymbolTable::closeScope(sharedData->currentSymbolTableScope);
                sharedData->currentSymbolTableScope->addEntry(**moduleParseResult, nullptr);
                foundModules.emplace_back(*moduleParseResult);
            }
        }

        if (!wereSemanticChecksForCurrentModuleOk) {
            SymbolTable::closeScope(sharedData->currentSymbolTableScope);
        }
    }
    
    const auto& nameOfTopLevelModule = determineExpectedNameOfTopLevelModule();
    addSkipStatementToMainModuleIfEmpty(foundModules, nameOfTopLevelModule);
    return 0;
}

std::any SyReCModuleVisitor::visitModule(SyReCParser::ModuleContext* context) {
    sharedData->currentModuleCallNestingLevel++;

    syrec::Module::ptr module;
    bool               isDeclaredModuleValid = true;
    if (const auto& moduleIdentToken = context->IDENT(); moduleIdentToken != nullptr) {
        module = std::make_shared<syrec::Module>(moduleIdentToken->getText());
        isDeclaredModuleValid = true;
    }

    if (const auto numDeclaredParametersInAST = context->parameterList() ? context->parameterList()->parameter().size() : 0) {
        const auto declaredParameters = tryVisitAndConvertProductionReturnValue<syrec::Variable::vec>(context->parameterList());
        std::for_each(
                declaredParameters->cbegin(),
                declaredParameters->cend(),
                [&module](const syrec::Variable::ptr& parsedParameter) { module->addParameter(parsedParameter); }
        );
        isDeclaredModuleValid &= declaredParameters.has_value() && declaredParameters->size() >= numDeclaredParametersInAST;
    }

    for (const auto& signalListAntlrContext : context->signalList()) {
        const auto& parsedSignalList = tryVisitAndConvertProductionReturnValue<syrec::Variable::vec>(signalListAntlrContext);
        isDeclaredModuleValid &= parsedSignalList.has_value();
        if (isDeclaredModuleValid) {
            std::for_each(
            parsedSignalList->cbegin(),
            parsedSignalList->cend(),
            [&module](const syrec::Variable::ptr& signal) {
                module->addVariable(signal);
            });
        }
    }

    if (const auto& validUserDefinedModuleStatements = tryVisitAndConvertProductionReturnValue<syrec::Statement::vec>(context->statementList()); validUserDefinedModuleStatements.has_value()) {
        module->statements = *validUserDefinedModuleStatements;    
    }
    sharedData->currentModuleCallNestingLevel--;
    return isDeclaredModuleValid ? std::make_optional(module) : std::nullopt;
}

std::any SyReCModuleVisitor::visitParameterList(SyReCParser::ParameterListContext* context) {
    const auto&          numParameterAstNodes = context->parameter().size();
    syrec::Variable::vec parametersContainer(numParameterAstNodes, nullptr);

    const auto& parameterAstNodesContainer = context->parameter();
    transformAndFilter(
        parameterAstNodesContainer.cbegin(),
        parameterAstNodesContainer.cend(),
        parametersContainer.begin(),
        [&](SyReCParser::ParameterContext* parameterAstNode) {
            return tryVisitAndConvertProductionReturnValue<syrec::Variable::ptr>(parameterAstNode);
        },
        [](const std::optional<syrec::Variable::ptr>& parsedParameter) {
            return parsedParameter.has_value();
        });

    return parametersContainer.size() == numParameterAstNodes
        ? std::make_optional(parametersContainer)
        : std::nullopt;
}

std::any SyReCModuleVisitor::visitParameter(SyReCParser::ParameterContext* context) {
    const auto parameterType = getParameterType(context->start);
    if (!parameterType.has_value()) {
        createError(mapAntlrTokenPosition(context->start),  InvalidParameterType);
    }

    const auto& declaredParameter = tryVisitAndConvertProductionReturnValue<syrec::Variable::ptr>(context->signalDeclaration());
    if (!parameterType.has_value() || !declaredParameter.has_value()) {
        return std::nullopt;
    }

    (*declaredParameter)->type = *parameterType;
    sharedData->currentSymbolTableScope->addEntry(**declaredParameter);

    /*
     * Due to a "potential" bug in the synthesizer in which the pointer address of the reference variable is used instead of the variable name,
     * we need to reuse the smart pointer reference for the parameter from the symbol table instead of the generated smart pointer of the signal production.
     * If said bug is resolve, we can reuse the production value again.
     */
    if (const auto& refetchedSymbolTableEntry = sharedData->currentSymbolTableScope->getVariable(declaredParameter->get()->name); refetchedSymbolTableEntry.has_value() && std::holds_alternative<syrec::Variable::ptr>(*refetchedSymbolTableEntry)) {
        return std::make_optional(std::get<syrec::Variable::ptr>(*refetchedSymbolTableEntry));
    }
    return std::nullopt;
}

std::any SyReCModuleVisitor::visitSignalList(SyReCParser::SignalListContext* context) {
    const auto declaredSignalsType = getSignalType(context->start);
    auto       wasSignalListDeclarationValid = true;
    if (!declaredSignalsType.has_value()) {
        createError(mapAntlrTokenPosition(context->start),  InvalidLocalType);
        wasSignalListDeclarationValid = false;
    }

    syrec::Variable::vec containerForSignalsOfType;
    if (declaredSignalsType.has_value()) {
        containerForSignalsOfType.reserve(context->signalDeclaration().size());
    }
    
    for (const auto& signalAstNode : context->signalDeclaration()) {
        if (const auto& parsedSignalDeclaration = tryVisitAndConvertProductionReturnValue<syrec::Variable::ptr>(signalAstNode); parsedSignalDeclaration.has_value()) {
            if (declaredSignalsType.has_value()) {
                parsedSignalDeclaration->get()->type = *declaredSignalsType;
                containerForSignalsOfType.emplace_back(*parsedSignalDeclaration);
                sharedData->currentSymbolTableScope->addEntry(**parsedSignalDeclaration);
            }
        } else {
            wasSignalListDeclarationValid = false;
        }
    }

    return wasSignalListDeclarationValid ? std::make_optional(containerForSignalsOfType) : std::nullopt;
}

std::any SyReCModuleVisitor::visitSignalDeclaration(SyReCParser::SignalDeclarationContext* context) {
    const auto&               declaredSignalIdent                 = context->IDENT() ? std::make_optional(context->IDENT()->getText()) : std::nullopt;
    auto        declaredSignalWidth                 = sharedData->parserConfig->cDefaultSignalWidth;
    auto                      isUserDefinedSignalDeclarationValid = declaredSignalIdent.has_value();

    if (declaredSignalIdent.has_value() && sharedData->currentSymbolTableScope->contains(*declaredSignalIdent)) {
        isUserDefinedSignalDeclarationValid = false;
        createError(mapAntlrTokenPosition(context->IDENT()->getSymbol()),  DuplicateDeclarationOfIdent, *declaredSignalIdent);
    }

    const auto& numDefinedValuesPerDimensionAstNodes = context->dimensionTokens.size();
    std::vector<unsigned int> declaredNumValuesPerDimension;
    declaredNumValuesPerDimension.reserve(numDefinedValuesPerDimensionAstNodes);
    
    for (const auto& numValuesPerDimensionToken : context->dimensionTokens) {
        if (const auto& parsedNumValuesPerDimension = numValuesPerDimensionToken ? tryConvertTextToNumber(numValuesPerDimensionToken->getText()) : std::nullopt;
            parsedNumValuesPerDimension.has_value()) {
            declaredNumValuesPerDimension.emplace_back(*parsedNumValuesPerDimension);
        } else {
            isUserDefinedSignalDeclarationValid = false;
        }
    }

    if (isUserDefinedSignalDeclarationValid && !numDefinedValuesPerDimensionAstNodes && declaredNumValuesPerDimension.empty()) {
        declaredNumValuesPerDimension.emplace_back(1);
    }

    if (context->signalWidthToken) {
        if (const auto& userDefinedSignalWidth = tryConvertTextToNumber(context->signalWidthToken->getText()); userDefinedSignalWidth.has_value()) {
            declaredSignalWidth = *userDefinedSignalWidth;
        } else {
            isUserDefinedSignalDeclarationValid = false;
        }
    }

    if (isUserDefinedSignalDeclarationValid) {
        // Variable type is assumed to be IN and should be fixed up by the caller of this function
        const auto& declaredSignalContainer = std::make_shared<syrec::Variable>(
                syrec::Variable::Types::In,
                *declaredSignalIdent,
                declaredNumValuesPerDimension,
                declaredSignalWidth);
        return std::make_optional(declaredSignalContainer);
    }
    return std::nullopt;
}

std::any SyReCModuleVisitor::visitStatementList(SyReCParser::StatementListContext* context) {
    return statementVisitor->visitStatementList(context);
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

std::optional<syrec::Variable::Types> SyReCModuleVisitor::getParameterType(const antlr4::Token* token) {
    if (!token) {
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
    if (!token) {
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