#ifndef SYREC_PARSER_SUCCESS_TEST_JSON_KEYS
#define SYREC_PARSER_SUCCESS_TEST_JSON_KEYS
#pragma once

#include <string>
#include <regex>

#include <nlohmann/json.hpp>
#include "core/syrec/module.hpp"
#include "core/syrec/parser/operation.hpp"

namespace syrecTestsJsonUtils {
    const std::string_view cJsonKeyModuleName             = "moduleName";
    const std::string_view cJsonKeyModuleParameters       = "parameters";
    const std::string_view cJsonKeyParameterAssignability = "assignability";
    const std::string_view cJsonKeyModuleLocals           = "locals";
    const std::string_view cJsonKeyModuleBodyStatements   = "statements";

    const std::string_view cJsonKeySignalName      = "signalName";
    const std::string_view cJsonKeySignalDimensions = "dimensions";
    const std::string_view cJsonKeySignalWidth     = "signalWidth";

    const std::string_view cJsonKeyStatementType               = "statementType";
    const std::string_view cJsonKeyIfStmtCheckCondition        = "checkCondition";
    const std::string_view cJsonKeyIfStmtTrueBranchStatements  = "trueBranchStatements";
    const std::string_view cJsonKeyIfStmtFalseBranchStatements = "falseBranchStatements";
    const std::string_view cJsonKeyIFStmtClosingCondition      = "closingCondition";

    const std::string_view cJsonKeyForStmtVariableName   = "loopVariableName";
    const std::string_view cJsonKeyForStmtIterationRange = "iterationRange";
    const std::string_view cJsonKeyForStmtStepSize       = "stepsize";
    const std::string_view cJsonKeyForStmtBodyStatements = "loopBodyStatements";

    const std::string_view cJsonKeyExprLhsOperand = "lhs";
    const std::string_view cJsonKeyExprRhsOperand = "rhs";
    const std::string_view cJsonKeyExprOperation  = "operation";
    const std::string_view cJsonKeyExprOperand    = "operand";

    const std::string_view cJsonKeyCallStmtActualParameters = "actualParameters";
    const std::string_view cJsonKeySignalDimensionsAccess   = "accessedDimensions";
    const std::string_view cJsonKeySignalBitAccess          = "accessedBits";

    const std::string_view cJsonKeyExpressionType             = "expressionType";
    const std::string_view cJsonKeyNumberType                 = "number";
    const std::string_view cJsonKeyNumberValue                = "value";
    const std::string_view cJsonKeyNumberAsConstantInteger    = "asConstantInteger";
    const std::string_view cJsonKeyNumberAsConstantFromSignal = "fromLoopVariable";

    const std::string_view cJsonKeyTestCaseName = "testCase";
    const std::string_view cJsonKeyTestCaseRawCircuitData = "circuit";
    const std::string_view cJsonKeyTestCaseExpectedIR = "expectedIR";

    class SyrecIRJsonParser {
    public:
        SyrecIRJsonParser() {
            validIdentValueRegex = std::regex("(\_|[a-z]|[A-Z])(\_|[a-z]|[A-Z]|\\d)*", std::regex_constants::ECMAScript | std::regex_constants::icase | std::regex_constants::optimize);
        }

        void parseModuleFromJson(const nlohmann::json& json, std::optional<syrec::Module::ptr>& module) const;
        void parseStatementFromJson(const nlohmann::json& json, std::optional<syrec::Statement::ptr>& stmt) const;

        void parseIfStatementFromJson(const nlohmann::json& json, std::optional<syrec::IfStatement::ptr>& ifStmt) const;
        void parseForStatementFromJson(const nlohmann::json& json, std::optional<syrec::ForStatement::ptr>& forStmt) const;
        void parseAssignStatementFromJson(const nlohmann::json& json, std::optional<syrec::AssignStatement::ptr>& assignStmt) const;
        void parseCallStatementFromJson(const nlohmann::json& json, std::optional<syrec::CallStatement::ptr>& callStmt) const;
        void parseUncallStatementFromJson(const nlohmann::json& json, std::optional<syrec::UncallStatement::ptr>& uncallStmt) const;
        void parseSkipStatementFromJson(const nlohmann::json& json, std::optional<syrec::SkipStatement::ptr>& skipStmt) const;
        void parseSwapStatementFromJson(const nlohmann::json& json, std::optional<syrec::SwapStatement::ptr>& swapStmt) const;
        void parseUnaryStatementFromJson(const nlohmann::json& json, std::optional<syrec::UnaryStatement::ptr>& unaryStmt) const;

        void parseNumberFromJson(const nlohmann::json& json, std::optional<syrec::Number::ptr>& number) const;
        void parseVariableFromJson(const nlohmann::json& json, std::optional<syrec::Variable::ptr>& variable) const;
        void parseVariableAccessFromJson(const nlohmann::json& json, std::optional<syrec::VariableAccess::ptr>& variableAccess) const;

        void parseExpressionFromJson(const nlohmann::json& json, std::optional<syrec::expression::ptr>& expr) const;
        void parseBinaryExpressionFromJson(const nlohmann::json& json, std::optional<syrec::BinaryExpression::ptr>& binaryExpr) const;
        void parseNumericExpressionFromJson(const nlohmann::json& json, std::optional<syrec::NumericExpression::ptr>& numericExpr) const;
        void parseVariableExpressionFromJson(const nlohmann::json& json, std::optional<syrec::VariableExpression::ptr>& variableExpr) const;
        void parseShiftExpressionFromJson(const nlohmann::json& json, std::optional<syrec::ShiftExpression::ptr>& shiftExpr) const;

    protected:
        std::regex validIdentValueRegex;
        [[nodiscard]] bool isValidIdentValue(const std::string& identValue) const;

        const std::string_view cJsonValueSignalInAsString = "in";
        const std::string_view cJsonValueSignalOutAsString = "out";
        const std::string_view cJsonValueSignalInOutAsString = "inout";
        const std::string_view cJsonValueSignalWireAsString  = "wire";
        const std::string_view cJsonValueSignalStateAsString = "state";

        enum NumberType {
            InvalidNumberType = -1,
            Constant,
            FromLoopVariable
        };

        enum SignalType {
            InvalidSignalType = -1,
            In,
            InOut,
            Out,
            State,
            Wire
        };

        enum StatementType {
            InvalidStatement = -1,
            IfStatement,
            ForStatement,
            AssignStatement,
            CallStatement,
            UncallStatement,
            SkipStatement,
            SwapStatement,
            UnaryStatement,
        };

        enum ExpressionType {
            InvalidExpression = -1,
            Number,
            VariableAccess,
            Unary,
            Binary,
            Shift
        };

        [[nodiscard]] SignalType getSignalTypeFromJson(const std::string_view& signalTypeAsJsonString) const;
        [[nodiscard]] StatementType getStatementTypeFromJson(const std::string_view& statementTypeAsJsonString) const;
        [[nodiscard]] ExpressionType getExpressionTypeFromJson(const std::string_view& expressionTypeAsJsonString) const;
        [[nodiscard]] NumberType     getNumberTypeFromjson(const std::string_view& numberTypeAsJsonString) const;
        [[nodiscard]] syrec_operation::operation getOperationFromJson(const std::string_view& operationAsJsonString) const;
    };
}
#endif