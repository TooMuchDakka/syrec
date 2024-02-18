#include "../syrecComponentsTestsUtils/test_base_syrec_circuit_comparison_test.hpp"
#include "../syrecComponentsTestsUtils/test_case_creation_utils.hpp"

#include "gtest/gtest.h"

class OperationStrengthReductionTest: public BaseSyrecCircuitComparisonTestFixture, public testing::WithParamInterface<std::string> {
public:
    inline static const std::string pathToTestCaseFile = "./unittests/syrecOptimizerComponentsTests/testdata/operation_strength_reduction.json";

protected:
    std::string getTestDataFilePath() const override {
        return pathToTestCaseFile;
    }

    std::string getTestCaseJsonKey() const override {
        return GetParam();
    }

    syrec::ReadProgramSettings getDefaultParserConfig() const override {
        auto configToUse                                    = syrec::ReadProgramSettings();
        configToUse.constantPropagationEnabled              = true;
        configToUse.operationStrengthReductionEnabled       = true;
        return configToUse;
    }
};

INSTANTIATE_TEST_SUITE_P(SyReCOptimizations, OperationStrengthReductionTest,
                         testing::ValuesIn(syrecTestUtils::loadTestCasesNamesFromFile(OperationStrengthReductionTest::pathToTestCaseFile, {})),
                         [](const testing::TestParamInfo<OperationStrengthReductionTest::ParamType>& info) {
                             auto s = info.param;
                             std::replace( s.begin(), s.end(), syrecTestUtils::notAllowedTestNameCharacter, syrecTestUtils::testNameDelimiterSymbol);
                             return s; });

TEST_P(OperationStrengthReductionTest, GenericOperationStengthReduction) {
    performParsingAndCompareExpectedAndActualCircuit();
}