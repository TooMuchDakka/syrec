#include "test_case_creation_utils.hpp"
#include "test_base_syrec_circuit_comparison_test.hpp"

#include "gtest/gtest.h"

class NoAdditionalLineSynthesisTest: public BaseSyrecCircuitComparisonTestFixture, public testing::WithParamInterface<std::string> {
public:
    inline static const std::string pathToTestCaseFile = "./unittests/syrecOptimizerComponentsTests/testdata/no_additional_line_synthesis.json";

protected:
    std::string getTestDataFilePath() const override {
        return pathToTestCaseFile;
    }

    std::string getTestCaseJsonKey() const override {
        return GetParam();
    }

    syrec::ReadProgramSettings getDefaultParserConfig() const override {
        std::optional<noAdditionalLineSynthesis::NoAdditionalLineSynthesisConfig> noAdditionalLineConfig = std::make_optional<noAdditionalLineSynthesis::NoAdditionalLineSynthesisConfig>();
        noAdditionalLineConfig->useGeneratedAssignmentsByDecisionAsTieBreaker         = true;
        noAdditionalLineConfig->preferAssignmentsGeneratedByChoiceRegardlessOfCost    = true;

        auto        configToUse                     = syrec::ReadProgramSettings();
        configToUse.constantPropagationEnabled      = true;
        configToUse.operationStrengthReductionEnabled = true;
        configToUse.optionalNoAdditionalLineSynthesisConfig = noAdditionalLineConfig;
        return configToUse;
    }
};

INSTANTIATE_TEST_SUITE_P(SyReCOptimizations, NoAdditionalLineSynthesisTest,
                         testing::ValuesIn(syrecTestUtils::loadTestCasesNamesFromFile(NoAdditionalLineSynthesisTest::pathToTestCaseFile, {})),
                         [](const testing::TestParamInfo<NoAdditionalLineSynthesisTest::ParamType>& info) {
                             auto s = info.param;
                             std::replace( s.begin(), s.end(), syrecTestUtils::notAllowedTestNameCharacter, syrecTestUtils::testNameDelimiterSymbol);
                             return s; });

TEST_P(NoAdditionalLineSynthesisTest, GenericNoAdditionalLineSynthesisOptimization) {
    performParsingAndCompareExpectedAndActualCircuit();
}