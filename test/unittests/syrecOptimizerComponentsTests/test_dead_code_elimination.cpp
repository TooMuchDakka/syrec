#include "test_base_syrec_circuit_comparison_test.hpp"
#include "test_case_creation_utils.hpp"
#include "gtest/gtest.h"

using namespace syrec;

class DeadCodeEliminationTest: public BaseSyrecCircuitComparisonTestFixture, public testing::WithParamInterface<std::string> {
public:
    inline static const std::string pathToTestCaseFile = "./unittests/syrecOptimizerComponentsTests/testdata/dead_code_elimination.json";

protected:
    std::string getTestDataFilePath() override {
        return pathToTestCaseFile;
    }

    std::string getTestCaseJsonKey() override {
        return GetParam();
    }

    syrec::ReadProgramSettings getDefaultParserConfig() override {
        const auto loopUnrollConfig = std::make_optional(optimizations::LoopOptimizationConfig(2, 2, UINT_MAX, true, false));
        return syrec::ReadProgramSettings(defaultSignalBitwidth, true, true, true, false, false, false, false, optimizations::MultiplicationSimplificationMethod::None, loopUnrollConfig);
    }
};

INSTANTIATE_TEST_SUITE_P(SyReCOptimizations, DeadCodeEliminationTest,
                         testing::ValuesIn(syrecTestUtils::loadTestCasesNamesFromFile(DeadCodeEliminationTest::pathToTestCaseFile, {})),
                         [](const testing::TestParamInfo<DeadCodeEliminationTest::ParamType>& info) {
                             auto s = info.param;
                             std::replace( s.begin(), s.end(), '-', '_');
                             return s; });

TEST_P(DeadCodeEliminationTest, GenericRemovalOfUnusedVariableAndModulesTest) {
    performParsingAndCompareExpectedAndActualCircuit();
}



