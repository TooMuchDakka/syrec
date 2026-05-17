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
#include "algorithms/synthesis/qmdd_transformer.hpp"
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
                                 // TODO: OK
                                 "swap",
                                 // TODO: OK
                                 "toffoli",
                                 // TODO: OK
                                 "x2Bit",
                                 // TODO: OK
                                 "test_dd_synthesis_1",
                                 // TODO: OK
                                 "test_dd_synthesis_2",
                                 // TODO: OK
                                 "3_17_6",
                                 // TODO: OK
                                 "bitwiseXor2Bit",
                                 // TODO: OK
                                 "adder2Bit",
                                 // TODO: OK
                                 "adder3Bit",
                                 // TODO: FAILS (NOT SOLVABLE)
                                 "4_49_7",
                                 "hwb4_12",
                                 "hwb5_13",
                                 "hwb6_14",
                                 "hwb7_15",
                                 "hwb8_64",
                                 "hwb9_65",
                                 // TODO: OK
                                 "graycode",
                                 // TODO: FAILS (NOT SOLVABLE)
                                 "hamming_7",
                                 // TODO: OK
                                 "mod4096",
                                 "mod8192",
                                 "mod638192",
                                 // TODO: INFINITE LOOP
                                 //"14_bit",
                                 // TODO: FAILS (NOT SOLVABLE)
                                 "urf1",
                                 "urf2",
                                 "urf3",
                                 "urf4",
                                 "urf5",
                                 // TODO: OK
                                 "dd_synth_paper_example"),
                         [](const testing::TestParamInfo<TestDDSynth::ParamType>& info) {
                             auto s = info.param;
                             std::ranges::replace(s, '-', '_');
                             return s; });

TEST_P(TestDDSynth, GenericDDSynthesisTest) {
    EXPECT_TRUE(readPla(tt, fileName));

    // https://agra.informatik.uni-bremen.de/doc/konf/12aspdac_qmdd_synth_rev.pdf
    // https://www.cda.cit.tum.de/files/eda/2017_rc_improving_qmdd_synthesis_of_reversible_circuits.pdf
    // https://mqt.readthedocs.io/projects/core/en/latest/dd_package.html

    std::ostringstream stringifiedQmddSynthesisInputDumpStream;
    auto               qmddSynthesisDumper = InMemoryQmddDumper(stringifiedQmddSynthesisInputDumpStream, true);
    //auto qmddSynthesisDumper = QmddToFileDumper("C:\\School\\MThesis\\qmdd_transform.txt", false);
    const std::optional<DDSynthesizer::QmddSynthesisResult> qmddSynthesisResult = DDSynthesizer::synthesizeQmdd(tt, &qmddSynthesisDumper);
    ASSERT_TRUE(qmddSynthesisResult.has_value());

    const dd::mEdge& reconstructedQuantumComputationFromQmdd = QmddTransformer::constructQmddFromGatesOfQuantumComputation(*qmddSynthesisResult->quantumComputation, *qmddSynthesisResult->qmddPackage, nullptr);

    std::ostringstream stringifiedReconstructedQmddSynthesisInputDumpStream;
    auto               qmddReconstructionDumper = InMemoryQmddDumper(stringifiedReconstructedQmddSynthesisInputDumpStream, true);
    qmddReconstructionDumper.dumpQmdd(&reconstructedQuantumComputationFromQmdd);

    ASSERT_EQ(stringifiedQmddSynthesisInputDumpStream.str(), stringifiedReconstructedQmddSynthesisInputDumpStream.str()) << "Expected dumps of qmdds (generated for initial qmdd state during synthesis and after reconstruction of the original qmdd) to match!";
}
