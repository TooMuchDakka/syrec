#include "syrec_ast_dump_utils.hpp"

using namespace syrecAstDumpUtils;

std::string syrecAstDumpUtils::stringifyModules(const syrec::Module::vec& modules) {
    return stringifyAndJoinMany(modules, "\n", stringifyModule);
}

std::string syrecAstDumpUtils::stringifyModule(const syrec::Module::ptr& moduleToStringify) {
    const std::string stringifiedParameters = stringifyAndJoinMany(moduleToStringify->parameters, ",", stringifyVariable);
    const std::string stringifiedLocals     = stringifyAndJoinMany(moduleToStringify->variables, ",", stringifyVariable);
    const std::string stringifiedStmts      = stringifyStatements(moduleToStringify->statements);

    return "module " + moduleToStringify->name + "(" + stringifiedParameters + ")\n" + stringifiedLocals + "\n" + stringifiedStmts;
}

std::string syrecAstDumpUtils::stringifyStatements(const syrec::Statement::vec& statements) {
    return stringifyAndJoinMany(statements, ";", stringifyStatement);
}

std::string syrecAstDumpUtils::stringifyStatement(const syrec::Statement::ptr& statement) {
    if (auto const* swapStmt = dynamic_cast<syrec::SwapStatement*>(statement.get())) {
        return stringifySwapStatement(*swapStmt);
    }
    if (auto const* assignStmt = dynamic_cast<syrec::AssignStatement*>(statement.get())) {
        return stringifyAssignStatement(*assignStmt);
    }
    if (auto const* callStmt = dynamic_cast<syrec::CallStatement*>(statement.get())) {
        return stringifyCallStatement(*callStmt);
    }
    if (auto const* uncallStmt = dynamic_cast<syrec::UncallStatement*>(statement.get())) {
        return stringifyUncallStatement(*uncallStmt);
    }
    if (auto const* unaryStmt = dynamic_cast<syrec::UnaryStatement*>(statement.get())) {
        return stringifyUnaryStatement(*unaryStmt);
    }
    if (auto const* skipStmt = dynamic_cast<syrec::SkipStatement*>(statement.get())) {
        return stringifySkipStatement(*skipStmt);
    }
    if (auto const* forStmt = dynamic_cast<syrec::ForStatement*>(statement.get())) {
        return stringifyForStatement(*forStmt);
    }
    if (auto const* ifStmt = dynamic_cast<syrec::IfStatement*>(statement.get())) {
        return stringifyIfStatement(*ifStmt);
    }
    return "";
}

std::string syrecAstDumpUtils::stringifySwapStatement(const syrec::SwapStatement& swapStmt) {
    return "";
}

std::string syrecAstDumpUtils::stringifyAssignStatement(const syrec::AssignStatement& assignStmt) {
    return "";
}

std::string syrecAstDumpUtils::stringifyCallStatement(const syrec::CallStatement& callStmt) {
    return "";
}

std::string syrecAstDumpUtils::stringifyUncallStatement(const syrec::UncallStatement& uncallStmt) {
    return "";
}

std::string syrecAstDumpUtils::stringifyUnaryStatement(const syrec::UnaryStatement& unaryStmt) {
    return "";
}

std::string syrecAstDumpUtils::stringifySkipStatement(const syrec::SkipStatement& skipStmt) {
    return "";
}

std::string syrecAstDumpUtils::stringifyForStatement(const syrec::ForStatement& forStmt) {
    return "";
}

std::string syrecAstDumpUtils::stringifyIfStatement(const syrec::IfStatement& ifStmt) {
    return "";
}

std::string syrecAstDumpUtils::stringifyExpression(const syrec::expression::ptr& expression) {
    return "";
}

std::string syrecAstDumpUtils::stringifyBinaryExpression(const syrec::BinaryExpression& binaryExpr) {
    return "";
}

std::string syrecAstDumpUtils::stringifyNumericExpression(const syrec::NumericExpression& numericExpr) {
    return "";
}

std::string syrecAstDumpUtils::stringifyShiftExpression(const syrec::ShiftExpression& shiftExpr) {
    return "";
}

std::string syrecAstDumpUtils::stringifyVariableExpression(const syrec::VariableExpression& variableExpression) {
    return "";
}

// TODO: Throw exception on invalid signal type ?
std::string syrecAstDumpUtils::stringifyVariable(const syrec::Variable::ptr& variable) {
    const std::string dimensionsStringified = stringifyAndJoinMany<unsigned int>(variable->dimensions, "", [](const unsigned int& t) { return "[" + std::to_string(t) + "]"; });
    const std::string bitwidthStringified   = "(" + std::to_string(variable->bitwidth) + ")";

    std::string variableAssignabilityStringified;
    switch (variable->type) {
        case syrec::Variable::In:
            variableAssignabilityStringified = "in";
            break;
        case syrec::Variable::Out:
            variableAssignabilityStringified = "out";
            break;
        case syrec::Variable::Inout:
            variableAssignabilityStringified = "inout";
            break;
        case syrec::Variable::Wire:
            variableAssignabilityStringified = "wire";
            break;
        case syrec::Variable::State:
            variableAssignabilityStringified = "state";
            break;
        default:
            variableAssignabilityStringified = "invalid";
            break;
    }
    return variableAssignabilityStringified + " " + variable->name + dimensionsStringified + bitwidthStringified;
}

std::string syrecAstDumpUtils::stringifyVariableAccess(const syrec::VariableAccess::ptr& variableAccess) {
    std::string accessedDimension;
    std::string bitAccess;

    if (!variableAccess->indexes.empty()) {
       accessedDimension = stringifyAndJoinManyComplex<syrec::expression>(variableAccess->indexes, "", [](const syrec::expression::ptr& expr) { return "[" + stringifyExpression(expr) + "]"; });
    }

    if (variableAccess->range.has_value()) {
        const auto&       definedBitAccess = variableAccess->range.value();
        bitAccess                    = "." + stringifyNumber(definedBitAccess.first);

        if (definedBitAccess.first != definedBitAccess.second) {
            bitAccess += ":" + stringifyNumber(definedBitAccess.second);
        }
    }
    return variableAccess->var->name + accessedDimension + bitAccess;
}

// TODO: Handling of loop variable
std::string syrecAstDumpUtils::stringifyNumber(const syrec::Number::ptr& number) {
    if (number->isLoopVariable()) {
        return "$" + number->variableName();
    }
    return std::to_string(number->evaluate({}));
}