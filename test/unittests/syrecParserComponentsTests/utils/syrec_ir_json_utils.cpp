#include "syrec_ir_json_utils.hpp"

#include "googletest/googletest/include/gtest/gtest.h"

using namespace syrecTestsJsonUtils;

void syrecTestsJsonUtils::SyrecIRJsonParser::parseModuleFromJson(const nlohmann::json& json, std::optional<syrec::Module::ptr>& module) const {
    ASSERT_TRUE(json.is_object());
    ASSERT_TRUE(json.contains(syrecTestsJsonUtils::cJsonKeyModuleName));
    ASSERT_TRUE(json.at(syrecTestsJsonUtils::cJsonKeyModuleName).is_string());
    
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


void syrecTestsJsonUtils::SyrecIRJsonParser::parseStatementFromJson(const nlohmann::json& json, std::optional<syrec::Statement::ptr>& stmt) const {
    ASSERT_TRUE(json.is_object());
    ASSERT_TRUE(json.contains(syrecTestsJsonUtils::cJsonKeyStatementType));
    StatementType stmtType = StatementType::InvalidStatement;
    ASSERT_NO_THROW(json.at(syrecTestsJsonUtils::cJsonKeyStatementType).get<StatementType>());
    switch (stmtType) {
        case StatementType::AssignStatement:
            ASSERT_NO_THROW(parseAssignStatementFromJson(json, stmt));
            break;
        case StatementType::CallStatement:
            ASSERT_NO_THROW(parseCallStatementFromJson(json, stmt));
            break;
        case StatementType::ForStatement:
            ASSERT_NO_THROW(parseForStatementFromJson(json, stmt));
            break;
        case StatementType::IfStatement:
            ASSERT_NO_THROW(parseIfStatementFromJson(json, stmt));
            break;
        case StatementType::SkipStatement:
            ASSERT_NO_THROW(parseSkipStatementFromJson(json, stmt));
            break;
        case StatementType::SwapStatement:
            ASSERT_NO_THROW(parseSwapStatementFromJson(json, stmt));
            break;
        case StatementType::UncallStatement:
            ASSERT_NO_THROW(parseUncallStatementFromJson(json, stmt));
            break;
        case StatementType::UnaryStatement:
            ASSERT_NO_THROW(parseUnaryStatementFromJson(json, stmt));
            break;
        default:
            FAIL();
    }
}

void syrecTestsJsonUtils::SyrecIRJsonParser::parseIfStatementFromJson(const nlohmann::json& json, std::optional<syrec::IfStatement::ptr>& ifStmt) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseForStatementFromJson(const nlohmann::json& json, std::optional<syrec::ForStatement::ptr>& forStmt) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseAssignStatementFromJson(const nlohmann::json& json, std::optional<syrec::AssignStatement::ptr>& assignStmt) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseCallStatementFromJson(const nlohmann::json& json, std::optional<syrec::CallStatement::ptr>& callStmt) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseUncallStatementFromJson(const nlohmann::json& json, std::optional<syrec::UncallStatement::ptr>& uncallStmt) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseSkipStatementFromJson(const nlohmann::json& json, std::optional<syrec::SkipStatement::ptr>& skipStmt) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseSwapStatementFromJson(const nlohmann::json& json, std::optional<syrec::SwapStatement::ptr>& swapStmt) const {}
void syrecTestsJsonUtils::SyrecIRJsonParser::parseUnaryStatementFromJson(const nlohmann::json& json, std::optional<syrec::UnaryStatement::ptr>& unaryStmt) const {}

void syrecTestsJsonUtils::SyrecIRJsonParser::parseNumberFromJson(const nlohmann::json& json, std::optional<syrec::Number::ptr>& number) const {
    ASSERT_TRUE(json.is_object());
    ASSERT_TRUE(json.contains(syrecTestsJsonUtils::cJsonKeyNumberType));
    const NumberType parsedNumberType = getNumberTypeFromjson(json.at(syrecTestsJsonUtils::cJsonKeyNumberType).get<std::string>());
    ASSERT_TRUE(json.contains(syrecTestsJsonUtils::cJsonKeyNumberValue));

    switch (parsedNumberType) {
        case NumberType::Constant:
            ASSERT_TRUE(json.at(syrecTestsJsonUtils::cJsonKeyNumberValue).is_number_unsigned());
            number.emplace(std::make_shared<syrec::Number>(syrec::Number(json.at(syrecTestsJsonUtils::cJsonKeyNumberValue).get<unsigned int>())));
            break;
        case NumberType::FromLoopVariable: {
            ASSERT_TRUE(json.at(syrecTestsJsonUtils::cJsonKeyNumberValue).is_string());
            const std::string accessedSignalWithOperationPrefix = json.at(syrecTestsJsonUtils::cJsonKeyNumberValue).get<std::string>();
            ASSERT_TRUE(accessedSignalWithOperationPrefix.size() > 1 && accessedSignalWithOperationPrefix.at(0) == '$');

            const std::string loopVariableName = accessedSignalWithOperationPrefix.substr(1, accessedSignalWithOperationPrefix.size() - 1);
            ASSERT_TRUE(isValidIdentValue(loopVariableName));

            number.emplace(std::make_shared<syrec::Number>(syrec::Number(loopVariableName)));
            break;   
        }
        default:
            FAIL();
    }
}

void syrecTestsJsonUtils::SyrecIRJsonParser::parseVariableFromJson(const nlohmann::json& json, std::optional<syrec::Variable::ptr>& variable) const {
    ASSERT_TRUE(json.is_object());
    ASSERT_TRUE(json.contains(syrecTestsJsonUtils::cJsonKeyParameterAssignability));
    const SignalType parsedSignalType = getSignalTypeFromJson(json.at(syrecTestsJsonUtils::cJsonKeyParameterAssignability).get<std::string>());
    ASSERT_NE(SignalType::InvalidSignalType, parsedSignalType);

    ASSERT_TRUE(json.contains(syrecTestsJsonUtils::cJsonKeySignalName));
    ASSERT_TRUE(json.at(syrecTestsJsonUtils::cJsonKeySignalName).is_string());
    const std::string signalName = json.at(syrecTestsJsonUtils::cJsonKeySignalName).get<std::string>();
    ASSERT_TRUE(isValidIdentValue(signalName));

    ASSERT_TRUE(json.contains(syrecTestsJsonUtils::cJsonKeySignalDimensions));
    ASSERT_TRUE(json.at(syrecTestsJsonUtils::cJsonKeySignalDimensions).is_array());
    std::vector<unsigned int> dimensions{};

    for (auto& dimension: json.at(syrecTestsJsonUtils::cJsonKeySignalDimensions)) {
        ASSERT_TRUE(dimension.is_number_unsigned());
        dimensions.emplace_back(dimension.get<unsigned int>());
    }
    if (dimensions.empty()) {
        dimensions.emplace_back(1);
        // TODO: Remove
        int x = 0;
    }

    ASSERT_TRUE(json.contains(syrecTestsJsonUtils::cJsonKeySignalWidth));
    ASSERT_TRUE(json.at(syrecTestsJsonUtils::cJsonKeySignalWidth).is_number_unsigned());
    const unsigned int signalWidth = json.at(syrecTestsJsonUtils::cJsonKeySignalWidth).get<unsigned int>();

    // TODO: Add mapping to correct signal type
    variable.emplace(std::make_shared<syrec::Variable>(syrec::Variable(0, signalName, dimensions, signalWidth)));
}

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

inline syrecTestsJsonUtils::SyrecIRJsonParser::SignalType syrecTestsJsonUtils::SyrecIRJsonParser::getSignalTypeFromJson(const std::string_view& signalTypeAsJsonString) const {
    if (signalTypeAsJsonString == cJsonValueSignalInAsString)
        return SignalType::In;   
    if (signalTypeAsJsonString == cJsonValueSignalOutAsString)
        return SignalType::Out;   
    if (signalTypeAsJsonString == cJsonValueSignalInOutAsString)
        return SignalType::InOut;   
    if (signalTypeAsJsonString == cJsonValueSignalWireAsString)
        return SignalType::Wire;  
    if (signalTypeAsJsonString == cJsonValueSignalStateAsString)
        return SignalType::State;

    return SignalType::InvalidSignalType;
}

syrecTestsJsonUtils::SyrecIRJsonParser::StatementType syrecTestsJsonUtils::SyrecIRJsonParser::getStatementTypeFromJson(const std::string_view& statementTypeAsJsonString) const {
    
}

syrecTestsJsonUtils::SyrecIRJsonParser::ExpressionType syrecTestsJsonUtils::SyrecIRJsonParser::getExpressionTypeFromJson(const std::string_view& expressionTypeAsJsonString) const {
    
}

syrecTestsJsonUtils::SyrecIRJsonParser::NumberType syrecTestsJsonUtils::SyrecIRJsonParser::getNumberTypeFromjson(const std::string_view& numberTypeAsJsonString) const {
    
}

syrec_operation::operation syrecTestsJsonUtils::SyrecIRJsonParser::getOperationFromJson(const std::string_view& operationAsJsonString) const {
    
}
