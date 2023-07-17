#include "test_base_syrec_circuit_comparison_test.hpp"
#include "test_case_creation_utils.hpp"

#include "gtest/gtest.h"

using namespace syrec;

class CombineRedundantInstructionsTest: public BaseSyrecCircuitComparisonTestFixture, public testing::WithParamInterface<std::string> {
public:
    inline static const std::string pathToTestCaseFile = "./unittests/syrecOptimizerComponentsTests/testdata/combine_redundant_instructions.json";

protected:
    std::string getTestDataFilePath() override {
        return pathToTestCaseFile;
    }

    std::string getTestCaseJsonKey() override {
        return GetParam();
    }

    syrec::ReadProgramSettings getDefaultParserConfig() override {
        return syrec::ReadProgramSettings(defaultSignalBitwidth, false, false, false, false, false, false, true, optimizations::MultiplicationSimplificationMethod::None, std::nullopt);
    }
};

INSTANTIATE_TEST_SUITE_P(SyReCOptimizations, CombineRedundantInstructionsTest,
                         testing::ValuesIn(syrecTestUtils::loadTestCasesNamesFromFile(CombineRedundantInstructionsTest::pathToTestCaseFile, {})),
                         [](const testing::TestParamInfo<CombineRedundantInstructionsTest::ParamType>& info) {
                             auto s = info.param;
                             std::replace( s.begin(), s.end(), '-', '_');
                             return s; });

TEST_P(CombineRedundantInstructionsTest, GenericCombinationOfRedundantInstructionsTest) {
    performParsingAndCompareExpectedAndActualCircuit();
}