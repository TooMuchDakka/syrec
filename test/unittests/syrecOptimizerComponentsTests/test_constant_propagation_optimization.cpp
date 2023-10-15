#include "test_base_syrec_circuit_comparison_test.hpp"
#include "test_case_creation_utils.hpp"

#include "gtest/gtest.h"

class ConstantPropagationTests: public BaseSyrecCircuitComparisonTestFixture, public testing::WithParamInterface<std::string> {
public:
    inline static const std::string pathToTestCaseFile = "./unittests/syrecOptimizerComponentsTests/testdata/test_constant_propagation_optimization.json";

protected:
    std::string getTestDataFilePath() const override {
        return pathToTestCaseFile;
    }

    std::string getTestCaseJsonKey() const override {
        return GetParam();
    }

    syrec::ReadProgramSettings getDefaultParserConfig() const override {
        return syrec::ReadProgramSettings(defaultSignalBitwidth, false, false, true, false);
    }
};

INSTANTIATE_TEST_SUITE_P(SyReCOptimizations, ConstantPropagationTests,
                         testing::ValuesIn(syrecTestUtils::loadTestCasesNamesFromFile(ConstantPropagationTests::pathToTestCaseFile, {})),
                         [](const testing::TestParamInfo<ConstantPropagationTests::ParamType>& info) {
                             auto s = info.param;
                             std::replace( s.begin(), s.end(), syrecTestUtils::notAllowedTestNameCharacter, syrecTestUtils::testNameDelimiterSymbol);
                             return s; });

TEST_P(ConstantPropagationTests, GenericConstantPropagationTest) {
    performParsingAndCompareExpectedAndActualCircuit();
}