#include "test_base_syrec_circuit_comparison_test.hpp"
#include "test_case_creation_utils.hpp"

#include "gtest/gtest.h"

class NoAdditionalLineSynthesisTest: public BaseSyrecCircuitComparisonTestFixture, public testing::WithParamInterface<std::string> {
public:
    inline static const std::string pathToTestCaseFile = "./unittests/syrecOptimizerComponentsTests/testdata/no_additional_line_synthesis.json";

protected:
    std::string getTestDataFilePath() override {
        return pathToTestCaseFile;
    }

    std::string getTestCaseJsonKey() override {
        return GetParam();
    }

    syrec::ReadProgramSettings getDefaultParserConfig() override {
        return syrec::ReadProgramSettings(16, false, false, true, true);
    }
};

INSTANTIATE_TEST_SUITE_P(SyReCOptimizations, NoAdditionalLineSynthesisTest,
                         testing::ValuesIn(syrecTestUtils::loadTestCasesNamesFromFile(NoAdditionalLineSynthesisTest::pathToTestCaseFile, {})),
                         [](const testing::TestParamInfo<NoAdditionalLineSynthesisTest::ParamType>& info) {
                             auto s = info.param;
                             std::replace( s.begin(), s.end(), '-', '_');
                             return s; });

TEST_P(NoAdditionalLineSynthesisTest, GenericNoAdditionalLineSynthesisOptimization) {
    performParsingAndCompareExpectedAndActualCircuit();
}