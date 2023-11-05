#ifndef DANGEROUS_COMPONENT_CHECK_UTILS_HPP
#define DANGEROUS_COMPONENT_CHECK_UTILS_HPP
#pragma once

#include "core/syrec/statement.hpp"
#include "core/syrec/variable.hpp"
#include "core/syrec/parser/symbol_table.hpp"

#include <unordered_set>

namespace dangerousComponentCheckUtils {
    [[nodiscard]] bool doesNumberContainPotentiallyDangerousComponent(const syrec::Number& number);
    [[nodiscard]] bool doesSignalAccessContainPotentiallyDangerousComponent(const syrec::VariableAccess& signalAccess);
    [[nodiscard]] bool doesExprContainPotentiallyDangerousComponent(const syrec::expression& expr);
    [[nodiscard]] bool doesStatementContainPotentiallyDangerousComponent(const syrec::Statement& statement, const parser::SymbolTable& symbolTable);
    [[nodiscard]] bool doesModuleContainPotentiallyDangerousComponent(const syrec::Module& module, const parser::SymbolTable& symbolTable);
    [[nodiscard]] bool doesOptimizedModuleSignatureContainNoParametersOrOnlyReadonlyOnes(const parser::SymbolTable::ModuleCallSignature& moduleCallSignature);
    [[nodiscard]] bool doesOptimizedModuleBodyContainAssignmentToModifiableParameter(const syrec::Module& module, const parser::SymbolTable& symbolTable);
    [[nodiscard]] bool doesStatementDefineAssignmentToModifiableParameter(const syrec::Statement& statement, const std::unordered_set<std::string>& identsOfModifiableParameters, const parser::SymbolTable& symbolTable);
    [[nodiscard]] bool isModifiableParameterAccessed(const syrec::VariableAccess& signalAccess, const std::unordered_set<std::string>& identsOfModifiableParameters);
}
#endif