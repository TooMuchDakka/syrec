#include "test_case_creation_utils.hpp"
#include "test_base_syrec_circuit_comparison_test.hpp"
#include "gtest/gtest.h"

using namespace syrec;

class MultiplicationSimplificationTest: public BaseSyrecCircuitComparisonTestFixture, public testing::WithParamInterface<std::string> {
public:
    inline static const std::string pathToTestCaseFile = "./unittests/syrecOptimizerComponentsTests/testdata/multiplication_simplification.json";

protected:
    std::string getTestDataFilePath() override {
        return pathToTestCaseFile;
    }

    std::string getTestCaseJsonKey() override {
        return GetParam();
    }
};

INSTANTIATE_TEST_SUITE_P(SyReCOptimizations, MultiplicationSimplificationTest,
                         testing::ValuesIn(syrecTestUtils::loadTestCasesNamesFromFile(MultiplicationSimplificationTest::pathToTestCaseFile, {})),
                         [](const testing::TestParamInfo<MultiplicationSimplificationTest::ParamType>& info) {
                             auto s = info.param;
                             std::replace( s.begin(), s.end(), '-', '_');
                             return s; });

TEST_P(MultiplicationSimplificationTest, GenericMultiplicationSimplification) {
    performParsingAndCompareExpectedAndActualCircuit();
}