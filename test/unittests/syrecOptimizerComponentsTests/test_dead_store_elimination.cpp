#include "../syrecComponentsTestsUtils/test_case_creation_utils.hpp"
#include "../syrecComponentsTestsUtils/test_base_syrec_circuit_comparison_test.hpp"

#include "gtest/gtest.h"

class DeadStoreEliminationTest: public BaseSyrecCircuitComparisonTestFixture, public testing::WithParamInterface<std::string> {
public:
    inline static const std::string pathToTestCaseFile = "./unittests/syrecOptimizerComponentsTests/testdata/dead_store_elimination.json";

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
        defaultConfig.deadCodeEliminationEnabled = true;
        defaultConfig.deadStoreEliminationEnabled = true;
        return defaultConfig;
    }
};

INSTANTIATE_TEST_SUITE_P(SyReCOptimizations, DeadStoreEliminationTest,
                         testing::ValuesIn(syrecTestUtils::loadTestCasesNamesFromFile(DeadStoreEliminationTest::pathToTestCaseFile, {})),
                         [](const testing::TestParamInfo<DeadStoreEliminationTest::ParamType>& info) {
                             auto s = info.param;
                             std::replace( s.begin(), s.end(), syrecTestUtils::notAllowedTestNameCharacter, syrecTestUtils::testNameDelimiterSymbol);
                             return s; });

TEST_P(DeadStoreEliminationTest, GenericRemovalOfUnusedVariableAndModulesTest) {
    performParsingAndCompareExpectedAndActualCircuit();
}