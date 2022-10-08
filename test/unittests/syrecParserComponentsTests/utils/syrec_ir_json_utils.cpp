#include "syrec_ir_json_utils.hpp"

#include "googletest/googletest/include/gtest/gtest.h"

using namespace syrecTestsJsonUtils;

void syrecTestsJsonUtils::SyrecIRJsonParser::parseModuleFromJson(const nlohmann::json& json, std::optional<syrec::Module::ptr>& module) const {
    ASSERT_TRUE(json.is_object());
    ASSERT_TRUE(json.contains(syrecTestsJsonUtils::cJsonKeyModuleName));
    ASSERT_TRUE(json.at(syrecTestsJsonUtils::cJsonKeyModuleName).is_string());
    
    // TODO: Validation of module name
    const std::string moduleName = json.at(syrecTestsJsonUtils::cJsonKeyModuleName);
    ASSERT_TRUE(isValidIdentValue(moduleName));

    ASSERT_TRUE(json.contains(syrecTestsJsonUtils::cJsonKeyModuleParameters));
    ASSERT_TRUE(json.at(syrecTestsJsonUtils::cJsonKeyModuleParameters).is_array());

    syrec::Variable::vec validParameters{};
    for (auto& parameter: json.at(syrecTestsJsonUtils::cJsonKeyModuleParameters)) {
        std::optional<syrec::Variable::ptr> parameterParsingResult;
        ASSERT_NO_THROW(parseVariableFromJson(parameter, parameterParsingResult));
        ASSERT_TRUE(parameterParsingResult.has_value());

        const syrec::Variable::ptr parsedParameter = parameterParsingResult.value();
        ASSERT_TRUE(parsedParameter->type != syrec::Variable::State && parsedParameter->type != syrec::Variable::Wire);
        validParameters.emplace_back(parsedParameter);
    }
    
    ASSERT_TRUE(json.contains(syrecTestsJsonUtils::cJsonKeyModuleLocals));
    ASSERT_TRUE(json.at(syrecTestsJsonUtils::cJsonKeyModuleLocals).is_array());

    syrec::Variable::vec validLocals{};
    for (auto& localVariable: json.at(syrecTestsJsonUtils::cJsonKeyModuleLocals)) {
        std::optional<syrec::Variable::ptr> localVariableParsingResult;
        ASSERT_NO_THROW(parseVariableFromJson(localVariable, localVariableParsingResult));
        ASSERT_TRUE(localVariableParsingResult.has_value());

        const syrec::Variable::ptr parsedLocalVariable = localVariableParsingResult.value();
        ASSERT_TRUE(parsedLocalVariable->type == syrec::Variable::State || parsedLocalVariable->type == syrec::Variable::Wire);
        validLocals.emplace_back(parsedLocalVariable);
    }

    ASSERT_TRUE(json.contains(syrecTestsJsonUtils::cJsonKeyModuleBodyStatements));
    ASSERT_TRUE(json.at(syrecTestsJsonUtils::cJsonKeyModuleBodyStatements).is_array());

    syrec::Statement::vec moduleStmts;
    for (auto& statement: json.at(syrecTestsJsonUtils::cJsonKeyModuleBodyStatements)) {
        std::optional<syrec::Statement::ptr> statementParsingResult;
        ASSERT_NO_THROW(parseStatementFromJson(statement, statementParsingResult));
        ASSERT_TRUE(statementParsingResult.has_value());
        moduleStmts.emplace_back(statementParsingResult.value());
    }
    ASSERT_FALSE(moduleStmts.empty());

    syrec::Module::ptr parsedModule = std::make_shared<syrec::Module>(syrec::Module(moduleName));
    parsedModule->parameters        = validParameters;
    parsedModule->variables         = validLocals;
    parsedModule->statements        = moduleStmts;
    module.emplace(parsedModule);
}


void syrecTestsJsonUtils::SyrecIRJsonParser::parseStatementFromJson(const nlohmann::json& json, std::optional<syrec::Statement::ptr>& stmt) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseIfStatementFromJson(const nlohmann::json& json, std::optional<syrec::IfStatement::ptr>& ifStmt) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseForStatementFromJson(const nlohmann::json& json, std::optional<syrec::ForStatement::ptr>& forStmt) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseAssignStatementFromJson(const nlohmann::json& json, std::optional<syrec::AssignStatement::ptr>& assignStmt) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseCallStatementFromJson(const nlohmann::json& json, std::optional<syrec::CallStatement::ptr>& callStmt) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseUncallStatementFromJson(const nlohmann::json& json, std::optional<syrec::UncallStatement::ptr>& uncallStmt) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseSkipStatementFromJson(const nlohmann::json& json, std::optional<syrec::SkipStatement::ptr>& skipStmt) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseSwapStatementFromJson(const nlohmann::json& json, std::optional<syrec::SwapStatement::ptr>& swapStmt) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseUnaryStatementFromJson(const nlohmann::json& json, std::optional<syrec::UnaryStatement::ptr>& unaryStmt) const {}

void syrecTestsJsonUtils::SyrecIRJsonParser::parseNumberFromJson(const nlohmann::json& json, std::optional<syrec::Number::ptr>& number) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseVariableFromJson(const nlohmann::json& json, std::optional<syrec::Variable::ptr>& variable) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseVariableAccessFromJson(const nlohmann::json& json, std::optional<syrec::VariableAccess::ptr>& variableAccess) const {}

void syrecTestsJsonUtils::SyrecIRJsonParser::parseExpressionFromJson(const nlohmann::json& json, std::optional<syrec::expression::ptr>& expr) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseBinaryExpressionFromJson(const nlohmann::json& json, std::optional<syrec::BinaryExpression::ptr>& binaryExpr) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseNumericExpressionFromJson(const nlohmann::json& json, std::optional<syrec::NumericExpression::ptr>& numericExpr) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseVariableExpressionFromJson(const nlohmann::json& json, std::optional<syrec::VariableExpression::ptr>& variableExpr) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseShiftExpressionFromJson(const nlohmann::json& json, std::optional<syrec::ShiftExpression::ptr>& shiftExpr) const {}

// TODO: Add EXCEPT_TRUE call for regex match to enable better error traceability in test output
inline bool syrecTestsJsonUtils::SyrecIRJsonParser::isValidIdentValue(const std::string& identValue) const {
    return !identValue.empty() && std::regex_match(identValue, validIdentValueRegex);
}
