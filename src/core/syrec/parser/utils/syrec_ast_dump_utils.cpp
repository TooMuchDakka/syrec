#include "core/syrec/parser/utils/syrec_ast_dump_utils.hpp"

using namespace syrecAstDumpUtils;

std::string SyrecASTDumper::stringifyModules(const syrec::Module::vec& modules) {
    std::vector<std::string> stringifiedModules(modules.size());

    size_t moduleIndex = 0;
    for (const auto& module: modules) {
        stringifiedModules.at(moduleIndex++) = stringifyModule(module);
    }

    std::ostringstream resultBuffer{};
    std::copy(stringifiedModules.cbegin(), stringifiedModules.cend(), infix_ostream_iterator<std::string>(resultBuffer, this->dumpConfig.newlineSequence.c_str()));
    return resultBuffer.str();
}

std::string SyrecASTDumper::stringifyModule(const syrec::Module::ptr& moduleToStringify) {
    const std::string stringifiedParameters = stringifyAndJoinMany(moduleToStringify->parameters, this->dumpConfig.parameterDelimiter.c_str(), stringifyVariable);
    const std::string stringifiedLocals = stringifyModuleLocals(moduleToStringify->variables);
    const std::string stringifiedStmts      = stringifyStatements(moduleToStringify->statements);

    return "module " + moduleToStringify->name + "(" + stringifiedParameters + ")" + this->dumpConfig.newlineSequence
        + (!moduleToStringify->variables.empty() ? stringifiedLocals + this->dumpConfig.newlineSequence : "")
        + stringifiedStmts;
}

std::string SyrecASTDumper::stringifyModuleLocals(const syrec::Variable::vec& moduleLocals) {
    if (moduleLocals.empty())
        return "";

    std::string stringifiedLocals = "";
    // TODO: This now only works because the Variable type IN is compiled to 0, a more robust translation would be nice (i.e. when refactoring the type to an enum instead of the unsigned int value it currently has)
    unsigned int currentLocalsType = 0;

    for (const syrec::Variable::ptr& local : moduleLocals) {
        if (local->type != currentLocalsType) {
            stringifiedLocals += this->dumpConfig.newlineSequence + stringifyLocal(local, true);
            currentLocalsType = local->type;
        }
        else {
            stringifiedLocals += this->dumpConfig.parameterDelimiter + stringifyLocal(local, false);
        }
    }

    return stringifiedLocals.substr(1, stringifiedLocals.size() - 1);
}

inline std::string SyrecASTDumper::stringifyStatements(const syrec::Statement::vec& statements) {
    std::vector<std::string> stringifiedStmts(statements.size());

    size_t stmtIndex = 0;
    for (const auto& stmt: statements) {
        stringifiedStmts.at(stmtIndex++) = repeatNTimes(this->dumpConfig.identationSequence, this->identationLevel) + stringifyStatement(stmt);
    }

    std::ostringstream resultBuffer{};
    std::copy(stringifiedStmts.cbegin(), stringifiedStmts.cend(), infix_ostream_iterator<std::string>(resultBuffer,  this->dumpConfig.stmtDelimiter.c_str()));
    return resultBuffer.str();
}

std::string SyrecASTDumper::stringifyStatement(const syrec::Statement::ptr& statement) {
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
    return stringifySkipStatement();
}

inline std::string SyrecASTDumper::stringifySwapStatement(const syrec::SwapStatement& swapStmt) {
    return stringifyVariableAccess(swapStmt.lhs) + " " + "<=>" + " " + stringifyVariableAccess(swapStmt.rhs);
}

std::string SyrecASTDumper::stringifyAssignStatement(const syrec::AssignStatement& assignStmt) {
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

inline std::string SyrecASTDumper::stringifyCallStatement(const syrec::CallStatement& callStmt) const {
    return "call " + callStmt.target->name + "(" + stringifyAndJoinMany<std::string>(callStmt.parameters, this->dumpConfig.parameterDelimiter.c_str(), [](const std::string& parameter) { return parameter; }) + ")";
}

inline std::string SyrecASTDumper::stringifyUncallStatement(const syrec::UncallStatement& uncallStmt) const {
    return "uncall " + uncallStmt.target->name + "(" + stringifyAndJoinMany<std::string>(uncallStmt.parameters, this->dumpConfig.parameterDelimiter.c_str(), [](const std::string& parameter) { return parameter; }) + ")";
}

std::string SyrecASTDumper::stringifyUnaryStatement(const syrec::UnaryStatement& unaryStmt) {
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

inline std::string SyrecASTDumper::stringifySkipStatement() {
    return "skip";
}

std::string SyrecASTDumper::stringifyForStatement(const syrec::ForStatement& forStmt) {

    //const std::string loopVarStringified = forStmt.loopVariable.empty() ? "" : ("$" + forStmt.loopVariable + " = ");
    const std::string loopVarStringified = forStmt.loopVariable.empty() ? "" : (forStmt.loopVariable + " = ");
    std::string       loopHeader = loopVarStringified;
    if (forStmt.range.first != forStmt.range.second) {
        loopHeader += stringifyNumber(forStmt.range.first) + " to ";
    }
    loopHeader += stringifyNumber(forStmt.range.second) + std::string(" step ") + stringifyNumber(forStmt.step) + " do";
    
    std::string stringifiedStmt = "for " + loopHeader + this->dumpConfig.newlineSequence;
    this->identationLevel++;
    stringifiedStmt += stringifyStatements(forStmt.statements) + this->dumpConfig.newlineSequence;
    this->identationLevel--;
    stringifiedStmt += repeatNTimes(this->dumpConfig.identationSequence, this->identationLevel) + "rof";
    return stringifiedStmt;
}

std::string SyrecASTDumper::stringifyIfStatement(const syrec::IfStatement& ifStmt) {
    std::string stringifiedCondition = "if " + stringifyExpression(ifStmt.condition) + " then" + this->dumpConfig.newlineSequence;
    this->identationLevel++;
    stringifiedCondition += stringifyStatements(ifStmt.thenStatements);
    this->identationLevel--;
    stringifiedCondition += this->dumpConfig.newlineSequence + repeatNTimes(this->dumpConfig.identationSequence, this->identationLevel) + "else" + this->dumpConfig.newlineSequence;

    this->identationLevel++;
    stringifiedCondition += stringifyStatements(ifStmt.elseStatements);
    this->identationLevel--;

    stringifiedCondition += this->dumpConfig.newlineSequence + repeatNTimes(this->dumpConfig.identationSequence, this->identationLevel) + "fi " + stringifyExpression(ifStmt.fiCondition);
    return stringifiedCondition;
}

std::string SyrecASTDumper::stringifyExpression(const syrec::expression::ptr& expression) {
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

std::string SyrecASTDumper::stringifyBinaryExpression(const syrec::BinaryExpression& binaryExpr) {
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

std::string SyrecASTDumper::stringifyNumericExpression(const syrec::NumericExpression& numericExpr) {
    return stringifyNumber(numericExpr.value);
}

std::string SyrecASTDumper::stringifyShiftExpression(const syrec::ShiftExpression& shiftExpr) {
    return "(" + stringifyExpression(shiftExpr.lhs) + " " + (shiftExpr.op == syrec::ShiftExpression::Left ? "<<" : ">>") + " " + stringifyNumber(shiftExpr.rhs) + ")";
}

std::string SyrecASTDumper::stringifyVariableExpression(const syrec::VariableExpression& variableExpression) {
    return stringifyVariableAccess(variableExpression.var);
}

// TODO: Throw exception on invalid signal type ?
std::string SyrecASTDumper::stringifyVariable(const syrec::Variable::ptr& variable) {
    const std::string dimensionsStringified = stringifyDimensions(variable->dimensions);
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
        default:
            variableAssignabilityStringified = "invalid";
            break;
    }
    return variableAssignabilityStringified + " " + variable->name + dimensionsStringified + bitwidthStringified;
}

std::string SyrecASTDumper::stringifyLocal(const syrec::Variable::ptr& local, bool withTypePrefix) {
    const std::string dimensionsStringified = stringifyDimensions(local->dimensions);
    const std::string bitwidthStringified   = "(" + std::to_string(local->bitwidth) + ")";

    std::string variableAssignabilityStringified;
    switch (local->type) {
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
    return (withTypePrefix ? variableAssignabilityStringified + " " : "") + local->name + dimensionsStringified + bitwidthStringified;
}


std::string SyrecASTDumper::stringifyVariableAccess(const syrec::VariableAccess::ptr& variableAccess) {
    std::string accessedDimension;
    std::string bitAccess;

    if (!variableAccess->indexes.empty()) {
        accessedDimension = stringifyAndJoinManyComplex<syrec::expression>(variableAccess->indexes, "", stringifyDimensionExpression);
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
inline std::string SyrecASTDumper::stringifyNumber(const syrec::Number::ptr& number) {
    if (number->isLoopVariable()) {
        // TODO: Since for now a loop variable ident contains the '$' symbol we can omit printing it a second time
        return number->variableName();
    }

    if (number->isCompileTimeConstantExpression()) {
        const auto        definedExpression     = number->getExpression();

        std::string numberStringified = "(" + stringifyNumber(definedExpression.lhsOperand) + " ";
        switch (definedExpression.operation) {
            case syrec::Number::Operation::Addition: {
                numberStringified += "+";
                break;
            }
            case syrec::Number::Operation::Subtraction: {
                numberStringified += "-";
                break;
            }
            case syrec::Number::Operation::Multiplication: {
                numberStringified += "*";
                break;
            }
            case syrec::Number::Operation::Division: {
                numberStringified += "/";
                break;
            }
        }
        numberStringified += " " + stringifyNumber(definedExpression.rhsOperand) + ")";
        return numberStringified;
    }
    return std::to_string(number->evaluate({}));
}

inline std::string SyrecASTDumper::stringifyDimensionExpression(const syrec::expression::ptr& expr) {
    return "[" + stringifyExpression(expr) + "]";
}

inline std::string SyrecASTDumper::stringifyDimensions(const std::vector<unsigned int>& dimensions) {
    if (dimensions.empty() || (dimensions.size() == 1 && dimensions.at(0) == 1)) {
        return "";
    }
    return stringifyAndJoinMany(dimensions, "", &SyrecASTDumper::stringifyDimension);
}

inline std::string SyrecASTDumper::stringifyDimension(const unsigned int& dimension) {
    return '[' + std::to_string(dimension) + "]";
}

inline std::string SyrecASTDumper::repeatNTimes(const std::string& str, const size_t numRepetitions) {
    if (numRepetitions == 0) {
        return "";
    }

    const std::vector repeatContainer(numRepetitions, str);
    std::ostringstream       resultBuffer{};
    std::copy(repeatContainer.cbegin(), repeatContainer.cend(), infix_ostream_iterator<std::string>(resultBuffer, ""));
    return resultBuffer.str();
}