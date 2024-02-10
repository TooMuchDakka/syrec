#include "../syrecComponentsTestsUtils/test_case_creation_utils.hpp"
#include "../syrecComponentsTestsUtils/test_base_syrec_circuit_comparison_test.hpp"

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
        return syrec::ReadProgramSettings(defaultSignalBitwidth, false, false, false, false, false, false, optimizations::MultiplicationSimplificationMethod::None, std::nullopt, std::nullopt);
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