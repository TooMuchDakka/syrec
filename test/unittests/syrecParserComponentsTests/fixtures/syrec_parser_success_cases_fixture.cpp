#include "test_base_syrec_circuit_comparison_test.hpp"
#include "test_case_creation_utils.hpp"

#include "gtest/gtest.h"

class SyrecParserSuccessTestFixture: public BaseSyrecCircuitComparisonTestFixture, public testing::WithParamInterface<std::string> {
public:
    constexpr static auto numberProductionTestCaseFile              = "./unittests/syrecParserComponentsTests/testdata/parsing/success/production_number.json";
    constexpr static auto assignmentStatementProductionTestCaseFile = "./unittests/syrecParserComponentsTests/testdata/parsing/success/production_assignStatement.json";
    constexpr static auto binaryExpressionProductionTestCaseFile    = "./unittests/syrecParserComponentsTests/testdata/parsing/success/production_binaryExpression.json";
    constexpr static auto callStatementProductionTestCaseFile       = "./unittests/syrecParserComponentsTests/testdata/parsing/success/production_callStatement.json";
    constexpr static auto expressionProductionTestCaseFile          = "./unittests/syrecParserComponentsTests/testdata/parsing/success/production_expression.json";
    constexpr static auto forStatementProductionTestCaseFile        = "./unittests/syrecParserComponentsTests/testdata/parsing/success/production_forStatement.json";
    constexpr static auto ifStatementProductionTestCaseFile         = "./unittests/syrecParserComponentsTests/testdata/parsing/success/production_ifStatement.json";
    constexpr static auto moduleProductionTestCaseFile              = "./unittests/syrecParserComponentsTests/testdata/parsing/success/production_module.json";
    constexpr static auto shiftExpressionProductionTestCaseFile     = "./unittests/syrecParserComponentsTests/testdata/parsing/success/production_shiftExpression.json";
    constexpr static auto signalProductionTestCaseFile              = "./unittests/syrecParserComponentsTests/testdata/parsing/success/production_signal.json";
    constexpr static auto skipStatementProductionTestCaseFile       = "./unittests/syrecParserComponentsTests/testdata/parsing/success/production_skipStatement.json";
    constexpr static auto statementListProductionTestCaseFile       = "./unittests/syrecParserComponentsTests/testdata/parsing/success/production_statementList.json";
    constexpr static auto swapStatementProductionTestCaseFile       = "./unittests/syrecParserComponentsTests/testdata/parsing/success/production_swapStatement.json";
    constexpr static auto unaryExpressionProductionTestCaseFile     = "./unittests/syrecParserComponentsTests/testdata/parsing/success/production_unaryExpression.json";
    constexpr static auto unaryStatementProductionTestCaseFile      = "./unittests/syrecParserComponentsTests/testdata/parsing/success/production_unaryStatement.json";

protected:
    std::string getTestDataFilePath() const override {
        return "";
    }

    std::string getTestCaseJsonKey() const override {
        return GetParam();
    }

    std::vector<std::string> getTestDataFilePaths() const override {
        return {
            numberProductionTestCaseFile,
            assignmentStatementProductionTestCaseFile,
            binaryExpressionProductionTestCaseFile,
            callStatementProductionTestCaseFile,
            expressionProductionTestCaseFile,
            forStatementProductionTestCaseFile,
            ifStatementProductionTestCaseFile,
            moduleProductionTestCaseFile,
            shiftExpressionProductionTestCaseFile,
            signalProductionTestCaseFile,
            skipStatementProductionTestCaseFile,
            statementListProductionTestCaseFile,
            swapStatementProductionTestCaseFile,
            unaryExpressionProductionTestCaseFile,
            unaryStatementProductionTestCaseFile
        };
    }

    std::string lookupTestCaseFilePathFromTestCaseName(const std::string_view& testCaseName) const override {
        const auto& firstUnderScorePosition = testCaseName.find_first_of(syrecTestUtils::testNameDelimiterSymbol);
        if (firstUnderScorePosition == std::string_view::npos) {
            return "";
        }
        const auto& secondUnderScorePosition = testCaseName.find_first_of(syrecTestUtils::testNameDelimiterSymbol, firstUnderScorePosition + 1);
        if (secondUnderScorePosition == std::string_view::npos) {
            return "";
        }

        const auto& productionIdentLength              = (secondUnderScorePosition - firstUnderScorePosition) - 1;
        const auto& productionIdentifierInTestCaseName = testCaseName.substr(firstUnderScorePosition + 1, productionIdentLength);
        if (productionIdentifierInTestCaseName == "number") {
            return numberProductionTestCaseFile;
        }
        if (productionIdentifierInTestCaseName == "assignStatement") {
            return assignmentStatementProductionTestCaseFile;
        }
        if (productionIdentifierInTestCaseName == "binaryExpression") {
            return binaryExpressionProductionTestCaseFile;
        }
        if (productionIdentifierInTestCaseName == "callStatement") {
            return callStatementProductionTestCaseFile;
        }
        if (productionIdentifierInTestCaseName == "expression") {
            return expressionProductionTestCaseFile;
        }
        if (productionIdentifierInTestCaseName == "forStatement") {
            return forStatementProductionTestCaseFile;
        }
        if (productionIdentifierInTestCaseName == "ifStatement") {
            return ifStatementProductionTestCaseFile;
        }
        if (productionIdentifierInTestCaseName == "module") {
            return moduleProductionTestCaseFile;
        }
        if (productionIdentifierInTestCaseName == "shiftExpression") {
            return shiftExpressionProductionTestCaseFile;
        }
        if (productionIdentifierInTestCaseName == "signal") {
            return signalProductionTestCaseFile;
        }
        if (productionIdentifierInTestCaseName == "skipStatement") {
            return skipStatementProductionTestCaseFile;
        }
        if (productionIdentifierInTestCaseName == "statementList") {
            return statementListProductionTestCaseFile;
        }
        if (productionIdentifierInTestCaseName == "swapStatement") {
            return swapStatementProductionTestCaseFile;
        }
        if (productionIdentifierInTestCaseName == "unaryExpression") {
            return unaryExpressionProductionTestCaseFile;
        }
        if (productionIdentifierInTestCaseName == "unaryStatement") {
            return unaryStatementProductionTestCaseFile;
        }
        return "";
    }

    std::string extractTestCaseNameFromParameter(const std::string& testCaseName) const override {
        const auto& productionPrefixAndNameDelimiterPosition = testCaseName.find_first_of(syrecTestUtils::testNameDelimiterSymbol);
        if (productionPrefixAndNameDelimiterPosition == std::string::npos) {
            return testCaseName;
        }

        const auto& parserProductionAndTestCaseDelimiterPosition = testCaseName.find_first_of(syrecTestUtils::testNameDelimiterSymbol, productionPrefixAndNameDelimiterPosition + 1);
        if (parserProductionAndTestCaseDelimiterPosition == std::string::npos) {
            return testCaseName;
        }

        if (parserProductionAndTestCaseDelimiterPosition + 1 == testCaseName.size()) {
            return "";
        }
        return testCaseName.substr(parserProductionAndTestCaseDelimiterPosition + 1);
    }
};

INSTANTIATE_TEST_SUITE_P(SyrecParserTest, SyrecParserSuccessTestFixture,
                         testing::ValuesIn(syrecTestUtils::loadTestCaseNamesFromFiles({SyrecParserSuccessTestFixture::numberProductionTestCaseFile,
                                                                                      SyrecParserSuccessTestFixture::assignmentStatementProductionTestCaseFile,
                                                                                      SyrecParserSuccessTestFixture::binaryExpressionProductionTestCaseFile,
                                                                                      SyrecParserSuccessTestFixture::callStatementProductionTestCaseFile,
                                                                                      SyrecParserSuccessTestFixture::expressionProductionTestCaseFile,
                                                                                      SyrecParserSuccessTestFixture::forStatementProductionTestCaseFile,
                                                                                      SyrecParserSuccessTestFixture::ifStatementProductionTestCaseFile,
                                                                                      SyrecParserSuccessTestFixture::moduleProductionTestCaseFile,
                                                                                      SyrecParserSuccessTestFixture::shiftExpressionProductionTestCaseFile,
                                                                                      SyrecParserSuccessTestFixture::signalProductionTestCaseFile,
                                                                                      SyrecParserSuccessTestFixture::skipStatementProductionTestCaseFile,
                                                                                      SyrecParserSuccessTestFixture::statementListProductionTestCaseFile,
                                                                                      SyrecParserSuccessTestFixture::swapStatementProductionTestCaseFile,
                                                                                      SyrecParserSuccessTestFixture::unaryExpressionProductionTestCaseFile,
                                                                                      SyrecParserSuccessTestFixture::unaryStatementProductionTestCaseFile},
                                                                                      {
                                                                                              {syrecTestUtils::extractFileNameWithoutExtensionAndPath(SyrecParserSuccessTestFixture::unaryExpressionProductionTestCaseFile), {"bitwiseNegationOfExpression", "bitwiseNegationOfNumber", "bitwiseNegationOfSignal", "bitwiseNegationSimplified", "logicalNegationOfExpression", "logicalNegationOfNumber", "logicalNegationOfSignal", "logicalNegationSimplified"}},
                                                                                              {syrecTestUtils::extractFileNameWithoutExtensionAndPath(SyrecParserSuccessTestFixture::assignmentStatementProductionTestCaseFile), {"addAssign_lhsNoExplicitDimension", "addAssign_lhsNoExplicitDimensionBitRange", "addAssign_lhsNoExplicitDimensionOneBit", "addAssign_lhsOneExplicitDimension", "addAssign_lhsOneExplicitDimensionBitRange", "addAssign_lhsOneExplicitDimensionOneBit", "subAssign_lhsNoExplicitDimension", "subAssign_lhsNoExplicitDimensionBitRange", "subAssign_lhsNoExplicitDimensionOneBit", "subAssign_lhsOneExplicitDimension", "subAssign_lhsOneExplicitDimensionBitRange", "subAssign_lhsOneExplicitDimensionOneBit", "xorAssign_lhsNoExplicitDimension", "xorAssign_lhsNoExplicitDimensionBitRange", "xorAssign_lhsNoExplicitDimensionOneBit", "xorAssign_lhsOneExplicitDimension", "xorAssign_lhsOneExplicitDimensionBitRange", "xorAssign_lhsOneExplicitDimensionOneBit"}},
                                                                                      })),
                         [](const testing::TestParamInfo<SyrecParserSuccessTestFixture::ParamType>& info) {
                             auto s = info.param;
                             std::replace( s.begin(), s.end(), syrecTestUtils::notAllowedTestNameCharacter, syrecTestUtils::testNameDelimiterSymbol);
                             return s; });

TEST_P(SyrecParserSuccessTestFixture, GenericSyrecParserTest) {
    performParsingAndCompareExpectedAndActualCircuit();
}