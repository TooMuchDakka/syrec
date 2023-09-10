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
            statementVisitor(std::make_unique<SyReCStatementVisitor>(SyReCStatementVisitor(sharedVisitorData)))
        {}

        std::any visitProgram(SyReCParser::ProgramContext* context) override;

        [[nodiscard]] std::vector<messageUtils::Message> getErrors() const;
        [[nodiscard]] std::vector<messageUtils::Message> getWarnings() const;
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
        void removeUnusedOptimizedModulesWithHelpOfInformationOfUnoptimized(const syrec::Module::vec& unoptimizedModules, syrec::Module::vec& optimizedModules, const std::string_view& expectedIdentOfTopLevelModuleToNotRemove) const;
        void removeModulesWithoutParameters(syrec::Module::vec& modules, const std::string_view& expectedIdentOfTopLevelModuleToNotRemove) const;
        void addSkipStatementToMainModuleIfEmpty(const syrec::Module::vec& modules, const std::string_view& expectedIdentOfTopLevelModule) const;

        [[nodiscard]] std::string determineExpectedNameOfTopLevelModule() const;
        [[nodiscard]] syrec::Module::ptr createCopyOfModule(const syrec::Module::ptr& moduleToCreateCopyFrom) const;
        [[nodiscard]] std::vector<std::size_t> determineUnusedParametersBetweenModuleVersions(const syrec::Module::ptr& unoptimizedModule, const syrec::Module::ptr& optimizedModule) const;

        static std::optional<syrec::Variable::Types> getParameterType(const antlr4::Token* token);
        static std::optional<syrec::Variable::Types> getSignalType(const antlr4::Token* token);
    };
}
#endif