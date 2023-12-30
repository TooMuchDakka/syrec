#include "test_base_syrec_circuit_comparison_test.hpp"
#include "test_case_creation_utils.hpp"

#include "gtest/gtest.h"

class DeadCodeEliminationTest: public BaseSyrecCircuitComparisonTestFixture, public testing::WithParamInterface<std::string> {
public:
    inline static const std::string pathToTestCaseFile = "./unittests/syrecOptimizerComponentsTests/testdata/dead_code_elimination.json";

protected:
    std::string getTestDataFilePath() const override {
        return pathToTestCaseFile;
    }

    std::string getTestCaseJsonKey() const override {
        return GetParam();
    }

    syrec::ReadProgramSettings getDefaultParserConfig() const override {
        return syrec::ReadProgramSettings(defaultSignalBitwidth, true, true, true, false, false, false, optimizations::MultiplicationSimplificationMethod::None, std::nullopt, std::nullopt);
    }
};

INSTANTIATE_TEST_SUITE_P(SyReCOptimizations, DeadCodeEliminationTest,
                         testing::ValuesIn(syrecTestUtils::loadTestCasesNamesFromFile(DeadCodeEliminationTest::pathToTestCaseFile, {})),
                         [](const testing::TestParamInfo<DeadCodeEliminationTest::ParamType>& info) {
                             auto s = info.param;
                             std::replace( s.begin(), s.end(), syrecTestUtils::notAllowedTestNameCharacter, syrecTestUtils::testNameDelimiterSymbol);
                             return s; });

TEST_P(DeadCodeEliminationTest, GenericRemovalOfUnusedVariableAndModulesTest) {
    performParsingAndCompareExpectedAndActualCircuit();
}
