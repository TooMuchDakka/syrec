#include "gtest/gtest.h"
#include <googletest/googletest/include/gtest/gtest.h>

#include <algorithm>
#include <fstream>

class SyReCParserErrorCasesFixture: public ::testing::TestWithParam<std::string> {
protected:
    void SetUp() override {
        const std::string test = "./unittests/syrecParserComponentsTests/testdata/parser_error_cases.json";
        std::ifstream     configFileStream(test, std::ios_base::in);
        ASSERT_TRUE(configFileStream.good()) << "Could not open test data json file @ "
                                             << "";
    }
};

INSTANTIATE_TEST_SUITE_P(Another,
                         SyReCParserErrorCasesFixture,
                         testing::Values("test-value-one"),
                         [](const testing::TestParamInfo<SyReCParserErrorCasesFixture::ParamType>& info) {
                            auto s = info.param;
                            std::replace(s.begin(), s.end(), '-', '_');
                            return s;
                        });

TEST_P(SyReCParserErrorCasesFixture, GenericSyrecParserErrorTest) {
    ASSERT_TRUE(true);
}
