#include "syrec_ast_dump_utils.hpp"

using namespace syrecAstDumpUtils;

std::string syrecAstDumpUtils::stringifyModules(const syrec::Module::vec& modules) {
    return stringifyAndJoinMany(modules, "\n", stringifyModule);
}

std::string syrecAstDumpUtils::stringifyModule(const syrec::Module::ptr& moduleToStringify) {
    const std::string stringifiedParameters = stringifyAndJoinMany(moduleToStringify->parameters, ", ", stringifyVariable);
    const std::string stringifiedLocals     = stringifyAndJoinMany(moduleToStringify->variables, " ", stringifyVariable);
    const std::string stringifiedStmts      = stringifyStatements(moduleToStringify->statements);

    return "module " + moduleToStringify->name + "(" + stringifiedParameters + ")\n" + stringifiedLocals + "\n" + stringifiedStmts;
}

std::string syrecAstDumpUtils::stringifyStatements(const syrec::Statement::vec& statements) {
    return stringifyAndJoinMany(statements, ";\n", stringifyStatement);
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
    if (auto const* forStmt = dynamic_cast<syrec::ForStatement*>(statement.get())) {
        return stringifyForStatement(*forStmt);
    }
    if (auto const* ifStmt = dynamic_cast<syrec::IfStatement*>(statement.get())) {
        return stringifyIfStatement(*ifStmt);
    }
    return stringifySkipStatement(*statement);
}

std::string syrecAstDumpUtils::stringifySwapStatement(const syrec::SwapStatement& swapStmt) {
    return stringifyVariableAccess(swapStmt.lhs) + " " + "<=>" + " " + stringifyVariableAccess(swapStmt.rhs);
}

std::string syrecAstDumpUtils::stringifyAssignStatement(const syrec::AssignStatement& assignStmt) {
    std::string assignOpStringified;
    switch (assignStmt.op) {
        case syrec::AssignStatement::Add:
            assignOpStringified = "+=";
            break;
        case syrec::AssignStatement::Subtract:
            assignOpStringified = "-=";
            break;
        case syrec::AssignStatement::Exor:
            assignOpStringified = "^=";
            break;
        default:
            assignOpStringified = "";
            break;
    }
    return stringifyVariableAccess(assignStmt.lhs) + " " + assignOpStringified + " " + stringifyExpression(assignStmt.rhs);
}

std::string syrecAstDumpUtils::stringifyCallStatement(const syrec::CallStatement& callStmt) {
    return "call " + callStmt.target->name + "(" + stringifyAndJoinMany<std::string>(callStmt.parameters, ", ", [](const std::string& elem) { return elem; }) + ")";
}

std::string syrecAstDumpUtils::stringifyUncallStatement(const syrec::UncallStatement& uncallStmt) {
    return "uncall " + uncallStmt.target->name + "(" + stringifyAndJoinMany<std::string>(uncallStmt.parameters, ", ", [](const std::string& elem) { return elem; }) + ")";
}

std::string syrecAstDumpUtils::stringifyUnaryStatement(const syrec::UnaryStatement& unaryStmt) {
    std::string unaryOpStringified;
    switch (unaryStmt.op) {
        case syrec::UnaryStatement::Increment:
            unaryOpStringified = "++=";
            break;
        case syrec::UnaryStatement::Decrement:
            unaryOpStringified = "--=";
            break;
        case syrec::UnaryStatement::Invert:
            unaryOpStringified = "~=";
            break;
        default:
            unaryOpStringified = "";
            break;
    }
    return unaryOpStringified + " " + stringifyVariableAccess(unaryStmt.var);
}

std::string syrecAstDumpUtils::stringifySkipStatement(const syrec::SkipStatement& skipStmt) {
    return "skip";
}

std::string syrecAstDumpUtils::stringifyForStatement(const syrec::ForStatement& forStmt) {

    const std::string loopVarStringified = forStmt.loopVariable.empty() ? "" : "$" + forStmt.loopVariable + " = ";
    std::string       loopHeader = loopVarStringified;
    if (forStmt.range.first != forStmt.range.second) {
        loopHeader += stringifyNumber(forStmt.range.first) + " to ";
    }
    loopHeader += stringifyNumber(forStmt.range.second);
    const unsigned int stepsize            = forStmt.step->evaluate({});
    loopHeader += std::string(" step ") + (stepsize < 0 ? "-" : "") + stringifyNumber(forStmt.step) + " do";

    return "for " + loopHeader + "\n" + stringifyStatements(forStmt.statements) + "\nrof";
}

std::string syrecAstDumpUtils::stringifyIfStatement(const syrec::IfStatement& ifStmt) {
    return "if " + stringifyExpression(ifStmt.condition) + "then\n "
            + stringifyStatements(ifStmt.thenStatements)
        + "\nelse\n "
        +    stringifyStatements(ifStmt.elseStatements)
        + "\n fi" + " " + stringifyExpression(ifStmt.fiCondition); 
}

std::string syrecAstDumpUtils::stringifyExpression(const syrec::expression::ptr& expression) {
    if (auto const* numberExpr = dynamic_cast<syrec::NumericExpression*>(expression.get())) {
        return stringifyNumericExpression(*numberExpr);
    }
    if (auto const* binaryExpr = dynamic_cast<syrec::BinaryExpression*>(expression.get())) {
        return stringifyBinaryExpression(*binaryExpr);    
    }
    if (auto const* shiftExpr = dynamic_cast<syrec::ShiftExpression*>(expression.get())) {
        return stringifyShiftExpression(*shiftExpr);
    }
    if (auto const* variableExpr = dynamic_cast<syrec::VariableExpression*>(expression.get())) {
        return stringifyVariableExpression(*variableExpr);
    }
    // TODO: Unary epxression is currently not supported
    return "";
}

std::string syrecAstDumpUtils::stringifyBinaryExpression(const syrec::BinaryExpression& binaryExpr) {
    std::string binaryExprStringified;
    switch (binaryExpr.op) {
        case syrec::BinaryExpression::Add:
            binaryExprStringified = "+";
            break;
        case syrec::BinaryExpression::Subtract:
            binaryExprStringified = "-";
            break;
        case syrec::BinaryExpression::Exor:
            binaryExprStringified = "^";
            break;
        case syrec::BinaryExpression::Multiply:
            binaryExprStringified = "*";
            break;
        case syrec::BinaryExpression::Divide:
            binaryExprStringified = "/";
            break;
        case syrec::BinaryExpression::Modulo:
            binaryExprStringified = "%";
            break;
        case syrec::BinaryExpression::FracDivide:
            binaryExprStringified = "*>";
            break;
        case syrec::BinaryExpression::LogicalAnd:
            binaryExprStringified = "&&";
            break;
        case syrec::BinaryExpression::LogicalOr:
            binaryExprStringified = "||";
            break;
        case syrec::BinaryExpression::BitwiseAnd:
            binaryExprStringified = "&";
            break;
        case syrec::BinaryExpression::BitwiseOr:
            binaryExprStringified = "|";
            break;
        case syrec::BinaryExpression::LessThan:
            binaryExprStringified = "<";
            break;
        case syrec::BinaryExpression::GreaterThan:
            binaryExprStringified = ">";
            break;
        case syrec::BinaryExpression::Equals:
            binaryExprStringified = "=";
            break;
        case syrec::BinaryExpression::NotEquals:
            binaryExprStringified = "!=";
            break;
        case syrec::BinaryExpression::LessEquals:
            binaryExprStringified = "<=";
            break;
        case syrec::BinaryExpression::GreaterEquals:
            binaryExprStringified = ">=";
            break;
        default:
            binaryExprStringified = "";
            break;
    }
    return "(" + stringifyExpression(binaryExpr.lhs) + " " + binaryExprStringified + " " + stringifyExpression(binaryExpr.rhs) + ")";
}

std::string syrecAstDumpUtils::stringifyNumericExpression(const syrec::NumericExpression& numericExpr) {
    return stringifyNumber(numericExpr.value);
}

std::string syrecAstDumpUtils::stringifyShiftExpression(const syrec::ShiftExpression& shiftExpr) {
    return "(" + stringifyExpression(shiftExpr.lhs) + " " + (shiftExpr.op == syrec::ShiftExpression::Left ? ">>" : "<<") + " " + stringifyNumber(shiftExpr.rhs) + ")";
}

std::string syrecAstDumpUtils::stringifyVariableExpression(const syrec::VariableExpression& variableExpression) {
    return stringifyVariableAccess(variableExpression.var);
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