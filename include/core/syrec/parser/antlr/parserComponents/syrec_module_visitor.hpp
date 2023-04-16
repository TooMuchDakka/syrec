#ifndef SYREC_MODULE_VISITOR_HPP
#define SYREC_MODULE_VISITOR_HPP
#pragma once

#include "SyReCBaseVisitor.h"
#include "core/syrec/parser/antlr/parserComponents/syrec_custom_base_visitor.hpp"
#include "core/syrec/parser/antlr/parserComponents/syrec_statement_visitor.hpp"

#include "core/syrec/module.hpp"

namespace parser {
    class SyReCModuleVisitor: public SyReCCustomBaseVisitor {
    public:
        explicit SyReCModuleVisitor(const std::shared_ptr<SharedVisitorData>& sharedVisitorData) : SyReCCustomBaseVisitor(sharedVisitorData),
            foundModules(syrec::Module::vec()),
            statementVisitor(std::make_unique<SyReCStatementVisitor>(SyReCStatementVisitor(sharedVisitorData))),
            lastDeclaredModuleIdent("")
        {}

        std::any visitProgram(SyReCParser::ProgramContext* context) override;

        [[nodiscard]] std::vector<std::string> getErrors() const;
        [[nodiscard]] std::vector<std::string> getWarnings() const;
        [[nodiscard]] syrec::Module::vec       getFoundModules() const;

    private:
        syrec::Module::vec                     foundModules;
        std::unique_ptr<SyReCStatementVisitor> statementVisitor;
        std::string                            lastDeclaredModuleIdent;

        std::any visitModule(SyReCParser::ModuleContext* context) override;
        std::any visitParameterList(SyReCParser::ParameterListContext* context) override;
        std::any visitParameter(SyReCParser::ParameterContext* context) override;
        std::any visitSignalList(SyReCParser::SignalListContext* context) override;
        std::any visitSignalDeclaration(SyReCParser::SignalDeclarationContext* context) override;
        std::any visitStatementList(SyReCParser::StatementListContext* context) override;

        void removeUnusedVariablesAndParametersFromModule(const syrec::Module::ptr& module) const;
        void removeUnusedModules(const std::string_view& expectedIdentOfTopLevelModuleToNotRemove);
        void removeModulesWithoutParameters(const std::string_view& expectedIdentOfTopLevelModuleToNotRemove);

        [[nodiscard]] std::string determineExpectedNameOfTopLevelModule() const;

        static std::optional<syrec::Variable::Types> getParameterType(const antlr4::Token* token);
        static std::optional<syrec::Variable::Types> getSignalType(const antlr4::Token* token);
    };
}
#endif