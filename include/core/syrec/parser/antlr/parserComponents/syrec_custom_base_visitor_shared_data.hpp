#ifndef SYREC_CUSTOM_BASE_VISITOR_SHARED_DATA_HPP
#define SYREC_CUSTOM_BASE_VISITOR_SHARED_DATA_HPP
#pragma once

#include "core/syrec/parser/assignment_signal_access_restriction.hpp"
#include "core/syrec/parser/parser_config.hpp"
#include "core/syrec/parser/signal_access_restriction.hpp"
#include "core/syrec/parser/symbol_table.hpp"
#include "core/syrec/parser/symbol_table_backup_helper.hpp"

#include <optional>

namespace parser {
    class SharedVisitorData {
    public:
        std::vector<std::string>                                 errors;
        std::vector<std::string>                                 warnings;
        const std::unique_ptr<ParserConfig>                      parserConfig;
        std::shared_ptr<SymbolTable>                             currentSymbolTableScope;
        std::size_t                                              currentModuleCallNestingLevel;
        syrec::Number::loop_variable_mapping                     loopVariableMappingLookup;
        bool                                                     shouldSkipSignalAccessRestrictionCheck;
        bool                                                     currentlyParsingAssignmentStmtRhs;
        std::optional<unsigned int>                              optionalExpectedExpressionSignalWidth;
        std::set<std::string>                                    evaluableLoopVariables;
        std::optional<std::string>                               lastDeclaredLoopVariable;
        bool                                                     currentlyInOmittedRegion;
        std::stack<SymbolTableBackupHelper::ptr>                 localSignalValuesBackup;

        explicit SharedVisitorData(const ParserConfig& parserConfig) :
            parserConfig(std::make_unique<ParserConfig>(parserConfig)),
            currentSymbolTableScope(std::make_shared<SymbolTable>()),
            currentModuleCallNestingLevel(0),
            shouldSkipSignalAccessRestrictionCheck(true),
            currentlyParsingAssignmentStmtRhs(false),
            currentlyInOmittedRegion(false)
        {}

        void resetSignalAccessRestriction() {
            if (signalAccessRestriction != nullptr) {
                signalAccessRestriction.reset();
                signalAccessRestriction = nullptr;
            }
        }

        void createSignalAccessRestriction(const syrec::VariableAccess::ptr& assignedToSignalPart) {
            if (signalAccessRestriction != nullptr) {
                signalAccessRestriction.reset();
            }

            signalAccessRestriction = std::make_unique<AssignmentSignalAccessRestriction>(assignedToSignalPart);
        }

        [[nodiscard]] std::optional<const AssignmentSignalAccessRestriction*> getSignalAccessRestriction() const {
            if (signalAccessRestriction != nullptr) {
                return std::make_optional(signalAccessRestriction.get());
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<SymbolTableBackupHelper::ptr> getCurrentLocalSignalValueBackupScope() const {
            if (localSignalValuesBackup.empty()) {
                return std::nullopt;
            }
            return std::make_optional(localSignalValuesBackup.top());    
        }

        void createNewLocalSignalValueBackupScope() {
            localSignalValuesBackup.push(std::make_shared<SymbolTableBackupHelper>());
        }

        void popCurrentLocalSignalValueBackupScope() {
            localSignalValuesBackup.pop();
        }
    protected:
        std::unique_ptr<AssignmentSignalAccessRestriction> signalAccessRestriction;
    };
}; // namespace parser

#endif