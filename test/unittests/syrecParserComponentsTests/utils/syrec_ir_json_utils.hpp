#ifndef SYREC_PARSER_SUCCESS_TEST_JSON_KEYS
#define SYREC_PARSER_SUCCESS_TEST_JSON_KEYS
#pragma once

#include <string>
#include <regex>

#include <nlohmann/json.hpp>
#include "core/syrec/module.hpp"
#include "core/syrec/parser/operation.hpp"

namespace syrecTestsJsonUtils {
    const std::string cJsonKeyModuleName             = "moduleName";
    const std::string cJsonKeyModuleParameters       = "parameters";
    const std::string cJsonKeyParameterAssignability = "assignability";
    const std::string cJsonKeyModuleLocals           = "locals";
    const std::string cJsonKeyModuleBodyStatements   = "statements";

    const std::string cJsonKeySignalName      = "signalName";
    const std::string cJsonKeySignalDimension = "dimension";
    const std::string cJsonKeySignalWidth     = "signalWidth";

    const std::string cJsonKeyStatementType               = "statementType";
    const std::string cJsonKeyIfStmtCheckCondition        = "checkCondition";
    const std::string cJsonKeyIfStmtTrueBranchStatements  = "trueBranchStatements";
    const std::string cJsonKeyIfStmtFalseBranchStatements = "falseBranchStatements";
    const std::string cJsonKeyIFStmtClosingCondition      = "closingCondition";

    const std::string cJsonKeyForStmtVariableName   = "loopVariableName";
    const std::string cJsonKeyForStmtIterationRange = "iterationRange";
    const std::string cJsonKeyForStmtStepSize       = "stepsize";
    const std::string cJsonKeyForStmtBodyStatements = "loopBodyStatements";

    const std::string cJsonKeyExprLhsOperand = "lhs";
    const std::string cJsonKeyExprRhsOperand = "rhs";
    const std::string cJsonKeyExprOperation  = "operation";
    const std::string cJsonKeyExprOperand    = "operand";

    const std::string cJsonKeyCallStmtActualParameters = "actualParameters";
    const std::string cJsonKeySignalDimensionsAccess   = "dimensions";
    const std::string cJsonKeySignalBitAccess          = "accessedBits";

    const std::string cJsonKeyExpressionType             = "expressionType";
    const std::string cJsonKeyNumberAsExpression         = "numerAsExpression";
    const std::string cJsonKeyNumberAsConstantInteger    = "numerAsConstantInteger";
    const std::string cJsonKeyNumberAsConstantFromSignal = "numberAsConstantFromSignal";

    const std::string cJsonKeyTestCaseName = "testCase";
    const std::string cJsonKeyTestCaseRawCircuitData = "circuit";
    const std::string cJsonKeyTestCaseExpectedIR = "expectedIR";

    class SyrecIRJsonParser {
    public:
        SyrecIRJsonParser() {
            validIdentValueRegex = std::regex("^(\_|\W)(\_|\w)*", std::regex_constants::ECMAScript | std::regex_constants::icase | std::regex_constants::optimize);
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
        
        enum StatementType {
            InvalidStatement,
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
            InvalidExpression,
            Number,
            VariableAccess,
            Unary,
            Binary,
            Shift
        };

        bool isValidIdentValue(const std::string& identValue) const;

        NLOHMANN_JSON_SERIALIZE_ENUM(StatementType, {
                                                            {IfStatement, "if"},
                                                            {ForStatement, "for"},
                                                            {AssignStatement, "assign"},
                                                            {CallStatement, "call"},
                                                            {UncallStatement, "uncall"},
                                                            {SkipStatement, "skip"},
                                                            {SwapStatement, "swap"},
                                                            {UnaryStatement, "unary"},
                                                    })

        NLOHMANN_JSON_SERIALIZE_ENUM(ExpressionType, {{Number, "number"},
                                                      {VariableAccess, "variableAccess"},
                                                      {Unary, "unary"},
                                                      {Binary, "binary"},
                                                      {Shift, "shift"}})

        NLOHMANN_JSON_SERIALIZE_ENUM(syrec_operation::operation, {{syrec_operation::operation::addition, "+"},
                                                                  {syrec_operation::operation::subtraction, "-"},
                                                                  {syrec_operation::operation::multiplication, "*"},
                                                                  {syrec_operation::operation::division, "/"},
                                                                  {syrec_operation::operation::modulo, "%"},
                                                                  {syrec_operation::operation::upper_bits_multiplication, "*>"},
                                                                  {syrec_operation::operation::bitwise_xor, "|"},
                                                                  {syrec_operation::operation::logical_and, "&&"},
                                                                  {syrec_operation::operation::logical_or, "||"},
                                                                  {syrec_operation::operation::bitwise_and, "&"},
                                                                  {syrec_operation::operation::bitwise_or, "|"},
                                                                  {syrec_operation::operation::less_than, "<"},
                                                                  {syrec_operation::operation::greater_than, ">"},
                                                                  {syrec_operation::operation::equals, "="},
                                                                  {syrec_operation::operation::not_equals, "!="},
                                                                  {syrec_operation::operation::less_equals, "<="},
                                                                  {syrec_operation::operation::greater_equals, ">="},
                                                                  {syrec_operation::operation::increment_assign, "++="},
                                                                  {syrec_operation::operation::decrement_assign, "--="},
                                                                  {syrec_operation::operation::negate_assign, "~="},
                                                                  {syrec_operation::operation::add_assign, "+="},
                                                                  {syrec_operation::operation::minus_assign, "-="},
                                                                  {syrec_operation::operation::xor_assign, "^="},
                                                                  {syrec_operation::operation::bitwise_negation, "~"},
                                                                  {syrec_operation::operation::logical_negation, "!"},
                                                                  {syrec_operation::operation::shift_left, "<<"},
                                                                  {syrec_operation::operation::shift_right, ">>"}})
    };
}



#endif