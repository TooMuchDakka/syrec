#include "test_case_creation_utils.hpp"
#include "test_base_syrec_circuit_comparison_test.hpp"

#include "gtest/gtest.h"

class CombineRedundantInstructionsTest: public BaseSyrecCircuitComparisonTestFixture, public testing::WithParamInterface<std::string> {
public:
    inline static const std::string pathToTestCaseFile = "./unittests/syrecOptimizerComponentsTests/testdata/combine_redundant_instructions.json";

protected:
    std::string getTestDataFilePath() const override {
        return pathToTestCaseFile;
    }

    std::string getTestCaseJsonKey() const override {
        return GetParam();
    }

    syrec::ReadProgramSettings getDefaultParserConfig() const override {
        auto defaultConfig = syrec::ReadProgramSettings();
        defaultConfig.defaultBitwidth = defaultSignalBitwidth;
        return defaultConfig;
    }
};

INSTANTIATE_TEST_SUITE_P(SyReCOptimizations, CombineRedundantInstructionsTest,
                         testing::ValuesIn(syrecTestUtils::loadTestCasesNamesFromFile(CombineRedundantInstructionsTest::pathToTestCaseFile, {})),
                         [](const testing::TestParamInfo<CombineRedundantInstructionsTest::ParamType>& info) {
                             auto s = info.param;
                             std::replace( s.begin(), s.end(), syrecTestUtils::notAllowedTestNameCharacter, syrecTestUtils::testNameDelimiterSymbol);
                             return s; });

TEST_P(CombineRedundantInstructionsTest, GenericCombinationOfRedundantInstructionsTest) {
    performParsingAndCompareExpectedAndActualCircuit();
}