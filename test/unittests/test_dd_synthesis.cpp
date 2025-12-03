/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "algorithms/synthesis/dd_synthesis.hpp"
#include "algorithms/synthesis/qmdd_to_identity_transformer.hpp"
#include "core/io/pla_parser.hpp"
#include "core/truthTable/truth_table.hpp"
#include "dd/Export.hpp"
#include "dd/Operations.hpp"
#include "dd/Package.hpp"

#include <algorithm>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>

using namespace syrec;

class TestDDSynth: public testing::TestWithParam<std::string> {
protected:
    TruthTable                   tt{};
    std::string                  testCircuitsDir = "./circuits/";
    std::unique_ptr<dd::Package> dd              = std::make_unique<dd::Package>(15U);
    std::string                  fileName;

    void SetUp() override {
        fileName = testCircuitsDir + GetParam() + ".pla";
    }
};

INSTANTIATE_TEST_SUITE_P(TestDDSynth, TestDDSynth,
                         testing::Values(
                                 "swap",
                                 "toffoli",
                                 "x2Bit",
                                 "test_dd_synthesis_1",
                                 "test_dd_synthesis_2",
                                 "3_17_6",
                                 "bitwiseXor2Bit",
                                 "adder2Bit",
                                 "adder3Bit",
                                 "4_49_7",
                                 "hwb4_12",
                                 "hwb5_13",
                                 "hwb6_14",
                                 "hwb7_15",
                                 "hwb8_64",
                                 "hwb9_65",
                                 "graycode",
                                 "hamming_7",
                                 "mod4096",
                                 "mod8192",
                                 "mod638192",
                                 "14_bit",
                                 "urf1",
                                 "urf2",
                                 "urf3",
                                 "urf4",
                                 "urf5"),
                         [](const testing::TestParamInfo<TestDDSynth::ParamType>& info) {
                             auto s = info.param;
                             std::ranges::replace(s, '-', '_');
                             return s; });

TEST_P(TestDDSynth, GenericDDSynthesisTest) {
    EXPECT_TRUE(readPla(tt, fileName));

    // https://agra.informatik.uni-bremen.de/doc/konf/12aspdac_qmdd_synth_rev.pdf
    // https://www.cda.cit.tum.de/files/eda/2017_rc_improving_qmdd_synthesis_of_reversible_circuits.pdf
    // https://mqt.readthedocs.io/projects/core/en/latest/dd_package.html

    const std::optional<DDSynthesizer::QmddSynthesisResult> qmddSynthesisResult = DDSynthesizer::synthesizeQmdd(tt);
    ASSERT_TRUE(qmddSynthesisResult.has_value());

    //const dd::mEdge& reconstructedQuantumComputationFromQmdd = dd::buildFunctionality(*qmddSynthesisResult->quantumComputation, *qmddSynthesisResult->qmddPackage);
    const dd::mEdge& reconstructedQuantumComputationFromQmdd = QmddToIdentityTransformer::constructQmddFromQuantumComputationStartingFromIdentityQmdd(*qmddSynthesisResult->quantumComputation, *qmddSynthesisResult->qmddPackage);
    ASSERT_TRUE(qmddSynthesisResult->edgeToRootOfQmdd == reconstructedQuantumComputationFromQmdd);
}
