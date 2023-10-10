#include "test_base_syrec_circuit_comparison_test.hpp"
#include "test_case_creation_utils.hpp"

#include "gtest/gtest.h"

class ReassociateExpressionTest: public BaseSyrecCircuitComparisonTestFixture, public testing::WithParamInterface<std::string> {
public:
    inline static const std::string pathToTestCaseFile = "./unittests/syrecOptimizerComponentsTests/testdata/reassociate_expression_optimization.json";

protected:
    std::string getTestDataFilePath() const override {
        return pathToTestCaseFile;
    }

    std::string getTestCaseJsonKey() const override {
        return GetParam();
    }

    syrec::ReadProgramSettings getDefaultParserConfig() const override {
        return syrec::ReadProgramSettings(16, true, false, true, true);
    }
};

INSTANTIATE_TEST_SUITE_P(SyReCOptimizations, ReassociateExpressionTest,
                         testing::ValuesIn(syrecTestUtils::loadTestCasesNamesFromFile(ReassociateExpressionTest::pathToTestCaseFile, {})),
                         [](const testing::TestParamInfo<ReassociateExpressionTest::ParamType>& info) {
                             auto s = info.param;
                             std::replace( s.begin(), s.end(), syrecTestUtils::notAllowedTestNameCharacter, syrecTestUtils::testNameDelimiterSymbol);
                             return s; });

TEST_P(ReassociateExpressionTest, GenericReassociateExpressionTest) {
    performParsingAndCompareExpectedAndActualCircuit();
}