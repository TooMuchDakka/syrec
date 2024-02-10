#include "core/syrec/parser/utils/dangerous_component_check_utils.hpp"

#include "core/syrec/parser/range_check.hpp"

inline std::optional<unsigned> tryFetchValueOfLoopVariable(const std::string& loopVariableIdent, const parser::SymbolTable& activeSymbolTableScope, bool isConstantPropagationEnabled, const syrec::Number::loop_variable_mapping& evaluableLoopVariablesIfConstantPropagationIsDisabled) {
    return isConstantPropagationEnabled || evaluableLoopVariablesIfConstantPropagationIsDisabled.count(loopVariableIdent) ? activeSymbolTableScope.tryFetchValueOfLoopVariable(loopVariableIdent) : std::nullopt;
}

inline bool isVariableReadOnly(const syrec::Variable& variable) {
    return variable.type == syrec::Variable::Types::In;
}

std::optional<unsigned> tryEvaluateNumberAsConstant(const syrec::Number& number, const parser::SymbolTable* symbolTableUsedForEvaluation, bool shouldPerformConstantPropagation, const syrec::Number::loop_variable_mapping& evaluableLoopVariablesIfConstantPropagationIsDisabled, bool* wasOriginalNumberSimplified) {
    if (number.isConstant()) {
        return std::make_optional(number.evaluate({}));
    }

    if (number.isLoopVariable() && symbolTableUsedForEvaluation) {
        if (const auto& fetchedValueOfLoopVariable = tryFetchValueOfLoopVariable(number.variableName(), *symbolTableUsedForEvaluation, shouldPerformConstantPropagation, evaluableLoopVariablesIfConstantPropagationIsDisabled); fetchedValueOfLoopVariable.has_value()) {
            if (wasOriginalNumberSimplified) {
                *wasOriginalNumberSimplified = true;
            }
            return fetchedValueOfLoopVariable;
        }
        return std::nullopt;
    }
    // TODO: handling and simplification of compile time constant expressions
    if (number.isCompileTimeConstantExpression()) {
        return std::nullopt;
    }
    return std::nullopt;
}

std::optional<unsigned> tryEvaluateExpressionToConstant(const syrec::Expression& expr, const parser::SymbolTable* symbolTableUsedForEvaluation, bool shouldPerformConstantPropagation, const syrec::Number::loop_variable_mapping& evaluableLoopVariablesIfConstantPropagationIsDisabled, bool* wasOriginalExprSimplified) {
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr) {
        return tryEvaluateNumberAsConstant(*exprAsNumericExpr->value, symbolTableUsedForEvaluation, shouldPerformConstantPropagation, evaluableLoopVariablesIfConstantPropagationIsDisabled, wasOriginalExprSimplified);
    }
    return std::nullopt;
}

bool dangerousComponentCheckUtils::doesNumberContainPotentiallyDangerousComponent(const syrec::Number& number) {
    if (number.isLoopVariable()) {
        return true;
    }

    if (!number.isCompileTimeConstantExpression()) {
        return false;
    }

    const auto& numberAsCompileTimeConstantExpr = number.getExpression();
    const auto& rhsOperandEvaluated             = tryEvaluateNumberAsConstant(number, nullptr, false, {}, nullptr);

    return (numberAsCompileTimeConstantExpr.operation == syrec::Number::CompileTimeConstantExpression::Division && rhsOperandEvaluated.has_value() && !*rhsOperandEvaluated) || doesNumberContainPotentiallyDangerousComponent(*numberAsCompileTimeConstantExpr.lhsOperand) || doesNumberContainPotentiallyDangerousComponent(*numberAsCompileTimeConstantExpr.rhsOperand);
}

bool dangerousComponentCheckUtils::doesSignalAccessContainPotentiallyDangerousComponent(const syrec::VariableAccess& signalAccess) {
    bool                                       containsPotentiallyDangerousOperation = false;
    const syrec::Number::loop_variable_mapping evaluableLoopVariablesIfConstantPropagationIsDisabled;

    if (signalAccess.range.has_value()) {
        containsPotentiallyDangerousOperation = doesNumberContainPotentiallyDangerousComponent(*signalAccess.range->first) || doesNumberContainPotentiallyDangerousComponent(*signalAccess.range->second);
        /*
         * We do not check that the bit range start should be smaller than the bit-range end. Additionally, we do not need to perform any optimizations of the signal access components since
         * either we are already checking the optimized signal access or we consider components which are not compile-time constants as potentially dangerous component.
         */
        const auto& bitRangeStartEvaluated = tryEvaluateNumberAsConstant(*signalAccess.range->first, nullptr, false, evaluableLoopVariablesIfConstantPropagationIsDisabled, nullptr);
        const auto& bitRangeEndEvaluated   = tryEvaluateNumberAsConstant(*signalAccess.range->second, nullptr, false, evaluableLoopVariablesIfConstantPropagationIsDisabled, nullptr);
        containsPotentiallyDangerousOperation &= !bitRangeStartEvaluated.has_value() || !bitRangeEndEvaluated.has_value() || !parser::isValidBitRangeAccess(signalAccess.var, std::make_pair(*bitRangeStartEvaluated, *bitRangeEndEvaluated));
    }

    for (std::size_t i = 0; i < signalAccess.indexes.size() && !containsPotentiallyDangerousOperation; ++i) {
        containsPotentiallyDangerousOperation |= doesExprContainPotentiallyDangerousComponent(*signalAccess.indexes.at(i));
        if (const auto& accessedValueOfDimension = tryEvaluateExpressionToConstant(*signalAccess.indexes.at(i), nullptr, false, evaluableLoopVariablesIfConstantPropagationIsDisabled, nullptr); accessedValueOfDimension.has_value()) {
            containsPotentiallyDangerousOperation |= !parser::isValidDimensionAccess(signalAccess.var, i, *accessedValueOfDimension, true);
        }
        else {
            containsPotentiallyDangerousOperation = true;
        }
    }
    return containsPotentiallyDangerousOperation;
}

bool dangerousComponentCheckUtils::doesExprContainPotentiallyDangerousComponent(const syrec::Expression& expr) {
    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&expr); exprAsBinaryExpr) {
        return doesExprContainPotentiallyDangerousComponent(*exprAsBinaryExpr->lhs) || doesExprContainPotentiallyDangerousComponent(*exprAsBinaryExpr->rhs);
    }
    if (const auto& exprAsShiftExpr = dynamic_cast<const syrec::ShiftExpression*>(&expr); exprAsShiftExpr) {
        return doesExprContainPotentiallyDangerousComponent(*exprAsShiftExpr->lhs) || doesNumberContainPotentiallyDangerousComponent(*exprAsShiftExpr->rhs);
    }
    if (const auto& exprAsVariableExpr = dynamic_cast<const syrec::VariableExpression*>(&expr); exprAsVariableExpr) {
        return doesSignalAccessContainPotentiallyDangerousComponent(*exprAsVariableExpr->var);
    }
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr) {
        return doesNumberContainPotentiallyDangerousComponent(*exprAsNumericExpr->value);
    }
    return false;
}

bool dangerousComponentCheckUtils::doesStatementContainPotentiallyDangerousComponent(const syrec::Statement& statement, const parser::SymbolTable& symbolTable) {
    if (const auto& statementAsIfStatement = dynamic_cast<const syrec::IfStatement*>(&statement); statementAsIfStatement) {
        return doesExprContainPotentiallyDangerousComponent(*statementAsIfStatement->condition) || std::any_of(statementAsIfStatement->thenStatements.cbegin(), statementAsIfStatement->thenStatements.cend(), [&symbolTable](const syrec::Statement::ptr& statement) {
                   return doesStatementContainPotentiallyDangerousComponent(*statement, symbolTable);
               }) ||
               std::any_of(statementAsIfStatement->elseStatements.cbegin(), statementAsIfStatement->elseStatements.cend(), [&symbolTable](const syrec::Statement::ptr& statement) {
                   return doesStatementContainPotentiallyDangerousComponent(*statement, symbolTable);
               });
    }
    if (const auto& statementAsLoopStatement = dynamic_cast<const syrec::ForStatement*>(&statement); statementAsLoopStatement) {
        return doesNumberContainPotentiallyDangerousComponent(*statementAsLoopStatement->range.first) || doesNumberContainPotentiallyDangerousComponent(*statementAsLoopStatement->range.second) || doesNumberContainPotentiallyDangerousComponent(*statementAsLoopStatement->step) || std::any_of(statementAsLoopStatement->statements.cbegin(), statementAsLoopStatement->statements.cend(), [&symbolTable](const syrec::Statement::ptr& loopBodyStmt) {
                   return doesStatementContainPotentiallyDangerousComponent(*loopBodyStmt, symbolTable);
               });
    }
    if (const auto& statementAsCallStatement = dynamic_cast<const syrec::CallStatement*>(&statement); statementAsCallStatement) {
        return std::any_of(
                statementAsCallStatement->target->statements.cbegin(),
                statementAsCallStatement->target->statements.cend(),
                [&symbolTable](const syrec::Statement::ptr& moduleBodyStmt) {
                    return doesStatementContainPotentiallyDangerousComponent(*moduleBodyStmt, symbolTable);
                });
    }
    if (const auto& statementAsAssignmentStmt = dynamic_cast<const syrec::AssignStatement*>(&statement); statementAsAssignmentStmt) {
        return doesSignalAccessContainPotentiallyDangerousComponent(*statementAsAssignmentStmt->lhs) || doesExprContainPotentiallyDangerousComponent(*statementAsAssignmentStmt->rhs);
    }
    if (const auto& statementAsUnaryAssignmentStmt = dynamic_cast<const syrec::UnaryStatement*>(&statement); statementAsUnaryAssignmentStmt) {
        return doesSignalAccessContainPotentiallyDangerousComponent(*statementAsUnaryAssignmentStmt->var);
    }
    if (const auto& statementAsSwapStmt = dynamic_cast<const syrec::SwapStatement*>(&statement); statementAsSwapStmt) {
        return doesSignalAccessContainPotentiallyDangerousComponent(*statementAsSwapStmt->lhs) || doesSignalAccessContainPotentiallyDangerousComponent(*statementAsSwapStmt->rhs);
    }
    return false;
}

bool dangerousComponentCheckUtils::doesModuleContainPotentiallyDangerousComponent(const syrec::Module& module, const parser::SymbolTable& symbolTable) {
    return std::any_of(
            module.statements.cbegin(),
            module.statements.cend(),
            [&symbolTable](const syrec::Statement::ptr& statement) {
                return doesStatementContainPotentiallyDangerousComponent(*statement, symbolTable);
            });
}

bool dangerousComponentCheckUtils::doesOptimizedModuleSignatureContainNoParametersOrOnlyReadonlyOnes(const parser::SymbolTable::ModuleCallSignature& moduleCallSignature) {
    return moduleCallSignature.parameters.empty() ||
           std::all_of(
                   moduleCallSignature.parameters.cbegin(),
                   moduleCallSignature.parameters.cend(),
                   [](const syrec::Variable::ptr& moduleParameter) {
                       return isVariableReadOnly(*moduleParameter);
                   });
}

bool dangerousComponentCheckUtils::doesOptimizedModuleBodyContainAssignmentToModifiableParameter(const syrec::Module& module, const parser::SymbolTable& symbolTable) {
    std::unordered_set<std::string> identsOfModifiableParameters;
    for (std::size_t i = 0; i < module.parameters.size(); ++i) {
        if (!isVariableReadOnly(*module.parameters.at(i))) {
            identsOfModifiableParameters.emplace(module.parameters.at(i)->name);
        }
    }

    return !identsOfModifiableParameters.empty() && std::any_of(module.statements.cbegin(), module.statements.cend(), [&symbolTable, &identsOfModifiableParameters](const syrec::Statement::ptr& statement) {
        return doesStatementDefineAssignmentToModifiableParameter(*statement, identsOfModifiableParameters, symbolTable);
    });
}

bool dangerousComponentCheckUtils::doesStatementDefineAssignmentToModifiableParameter(const syrec::Statement& statement, const std::unordered_set<std::string>& identsOfModifiableParameters, const parser::SymbolTable& symbolTable) {
    if (const auto& statementAsIfStatement = dynamic_cast<const syrec::IfStatement*>(&statement); statementAsIfStatement) {
        return std::any_of(statementAsIfStatement->thenStatements.cbegin(),
                           statementAsIfStatement->thenStatements.cend(),
                           [&identsOfModifiableParameters, &symbolTable](const syrec::Statement::ptr& trueBranchStmt) {
                               return doesStatementDefineAssignmentToModifiableParameter(*trueBranchStmt, identsOfModifiableParameters, symbolTable);
                           }) ||
               std::any_of(statementAsIfStatement->elseStatements.cbegin(),
                           statementAsIfStatement->elseStatements.cend(),
                           [&identsOfModifiableParameters, &symbolTable](const syrec::Statement::ptr& falseBranchStmt) {
                               return doesStatementDefineAssignmentToModifiableParameter(*falseBranchStmt, identsOfModifiableParameters, symbolTable);
                           });
    }
    if (const auto& statementAsLoopStatement = dynamic_cast<const syrec::ForStatement*>(&statement); statementAsLoopStatement) {
        return std::any_of(statementAsLoopStatement->statements.cbegin(), statementAsLoopStatement->statements.cend(),
                           [&identsOfModifiableParameters, &symbolTable](const syrec::Statement::ptr& loopBodyStmt) {
                               return doesStatementDefineAssignmentToModifiableParameter(*loopBodyStmt, identsOfModifiableParameters, symbolTable);
                           });
    }
    if (const auto& statementAsCallStatement = dynamic_cast<const syrec::CallStatement*>(&statement); statementAsCallStatement) {
        if (const auto& optimizedCallSignatureInformation = symbolTable.tryGetOptimizedSignatureForModuleCall(*statementAsCallStatement->target); optimizedCallSignatureInformation.has_value()) {
            const auto& parametersOfOptimizedModuleSignature = optimizedCallSignatureInformation->determineOptimizedCallSignature(nullptr);
            return std::any_of(parametersOfOptimizedModuleSignature.cbegin(), parametersOfOptimizedModuleSignature.cend(), [](const syrec::Variable::ptr& parameter) {
                return !isVariableReadOnly(*parameter);
            });
        }
        return true;
    }
    if (const auto& statementAsAssignmentStmt = dynamic_cast<const syrec::AssignStatement*>(&statement); statementAsAssignmentStmt) {
        return identsOfModifiableParameters.count(statementAsAssignmentStmt->lhs->var->name);
    }
    if (const auto& statementAsUnaryAssignmentStmt = dynamic_cast<const syrec::UnaryStatement*>(&statement); statementAsUnaryAssignmentStmt) {
        return identsOfModifiableParameters.count(statementAsUnaryAssignmentStmt->var->var->name);
    }
    if (const auto& statementAsSwapStmt = dynamic_cast<const syrec::SwapStatement*>(&statement); statementAsSwapStmt) {
        return identsOfModifiableParameters.count(statementAsSwapStmt->lhs->var->name) || identsOfModifiableParameters.count(statementAsSwapStmt->rhs->var->name);
    }
    return false;
}
