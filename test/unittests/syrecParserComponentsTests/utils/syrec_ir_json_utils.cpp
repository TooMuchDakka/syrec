#include "syrec_ir_json_utils.hpp"

using namespace syrecTestsJsonUtils;

std::optional<syrec::Module::ptr> syrecTestsJsonUtils::parseModuleFromJson(const nlohmann::json& json) {
    return std::nullopt;
}

std::optional<syrec::Statement::ptr> parseStatementFromJson(const nlohmann::json& json) {
    return std::nullopt;
}

std::optional<syrec::IfStatement::ptr>     syrecTestsJsonUtils::parseIfStatementFromJson(const nlohmann::json& json) {
    return std::nullopt;
}

std::optional<syrec::ForStatement::ptr>    syrecTestsJsonUtils::parseForStatementFromJson(const nlohmann::json& json) {
    return std::nullopt;
}

std::optional<syrec::AssignStatement::ptr> syrecTestsJsonUtils::parseAssignStatementFromJson(const nlohmann::json& json) {
    return std::nullopt;
}

std::optional<syrec::CallStatement::ptr>   syrecTestsJsonUtils::parseCallStatementFromJson(const nlohmann::json& json) {
    return std::nullopt;
}

std::optional<syrec::UncallStatement::ptr> syrecTestsJsonUtils::parseUncallStatementFromJson(const nlohmann::json& json) {
    return std::nullopt;
}

std::optional<syrec::SkipStatement::ptr>   syrecTestsJsonUtils::parseSkipStatementFromJson(const nlohmann::json& json) {
    return std::nullopt;
}

std::optional<syrec::SwapStatement::ptr>   syrecTestsJsonUtils::parseSwapStatementFromJson(const nlohmann::json& json) {
    return std::nullopt;
}

std::optional<syrec::UnaryStatement::ptr>  syrecTestsJsonUtils::parseUnaryStatementFromJson(const nlohmann::json& json) {
    return std::nullopt;
}

std::optional<syrec::Number::ptr>         syrecTestsJsonUtils::parseNumberFromJson(const nlohmann::json& json) {
    return std::nullopt;
}

std::optional<syrec::Variable::ptr>       syrecTestsJsonUtils::parseVariableFromJson(const nlohmann::json& json) {
    return std::nullopt;
}

std::optional<syrec::VariableAccess::ptr> syrecTestsJsonUtils::parseVariableAccessFromJson(const nlohmann::json& json) {
    return std::nullopt;
}

std::optional<syrec::expression::ptr> parseExpressionFromJson(const nlohmann::json& json) {
    return std::nullopt;
}

std::optional<syrec::BinaryExpression::ptr>   syrecTestsJsonUtils::parseBinaryExpressionFromJson(const nlohmann::json& json) {
    return std::nullopt;
}

std::optional<syrec::NumericExpression::ptr>  syrecTestsJsonUtils::parseNumericExpressionFromJson(const nlohmann::json& json) {
    return std::nullopt;
}

std::optional<syrec::VariableExpression::ptr> syrecTestsJsonUtils::parseVariableExpressionFromJson(const nlohmann::json& json) {
    return std::nullopt;
}

std::optional<syrec::ShiftExpression::ptr>    syrecTestsJsonUtils::parseShiftExpressionFromJson(const nlohmann::json& json) {
    return std::nullopt;
}