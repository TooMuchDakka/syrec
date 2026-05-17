/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "algorithms/synthesis/qmddTransformation/qmdd_transformation_operations.hpp"
#include "dd/Operations.hpp"
#include "dd/Package.hpp"
#include "ir/QuantumComputation.hpp"
#include "qmdd_pkg_test_utils.hpp"

#include "gmock/gmock-matchers.h"
#include <gtest/gtest.h>

using namespace syrec;

// TODO: Test for operations should include tests for optimized away portions of qmdd tree.

TEST(QmddTransformationOperationsTest, ApplyMCXGateWithNoControlQubits) {
    constexpr std::size_t nQubits     = 1U;
    constexpr dd::Qubit   targetQubit = 0U;
    const qc::Controls    controlQubits{};
    const auto            expectedXOperation = qc::StandardOperation(controlQubits, targetQubit, qc::X);

    auto       quantumComputation = qc::QuantumComputation(nQubits);
    const auto qmddPkg            = std::make_unique<dd::Package>(nQubits);
    applyMCXGateToQmdd(quantumComputation, *qmddPkg, dd::mEdge::one(), targetQubit, controlQubits);

    ASSERT_EQ(1U, quantumComputation.getNops());
    const auto& actualXOperation = quantumComputation.back();
    ASSERT_EQ(expectedXOperation, *actualXOperation);
}

TEST(QmddTransformationOperationsTest, ApplyMCXGateWithPositiveControlQubit) {
    constexpr std::size_t nQubits      = 2U;
    constexpr dd::Qubit   targetQubit  = 0U;
    constexpr dd::Qubit   controlQubit = 1U;
    const qc::Controls    controlQubits({qc::Control(controlQubit, qc::Control::Type::Pos)});
    const auto            expectedXOperation = qc::StandardOperation(controlQubits, targetQubit, qc::X);

    auto       quantumComputation = qc::QuantumComputation(nQubits);
    const auto qmddPkg            = std::make_unique<dd::Package>(nQubits);
    applyMCXGateToQmdd(quantumComputation, *qmddPkg, dd::mEdge::one(), targetQubit, controlQubits);

    ASSERT_EQ(1U, quantumComputation.getNops());
    const auto& actualXOperation = quantumComputation.back();
    ASSERT_EQ(expectedXOperation, *actualXOperation);
}

TEST(QmddTransformationOperationsTest, ApplyMCXGateWithNegativeControlQubit) {
    constexpr std::size_t nQubits      = 2U;
    constexpr dd::Qubit   targetQubit  = 0U;
    constexpr dd::Qubit   controlQubit = 1U;
    const qc::Controls    controlQubits({qc::Control(controlQubit, qc::Control::Type::Neg)});
    const auto            expectedXOperation = qc::StandardOperation(controlQubits, targetQubit, qc::X);

    auto       quantumComputation = qc::QuantumComputation(nQubits);
    const auto qmddPkg            = std::make_unique<dd::Package>(nQubits);
    applyMCXGateToQmdd(quantumComputation, *qmddPkg, dd::mEdge::one(), targetQubit, controlQubits);

    ASSERT_EQ(1U, quantumComputation.getNops());
    const auto& actualXOperation = quantumComputation.back();
    ASSERT_EQ(expectedXOperation, *actualXOperation);
}

TEST(QmddTransformationOperationsTest, ApplyMCXGateWithMultipleControlQubits) {
    constexpr std::size_t nQubits            = 3U;
    constexpr dd::Qubit   targetQubit        = 0U;
    constexpr dd::Qubit   firstControlQubit  = 1U;
    constexpr dd::Qubit   secondControlQubit = 2U;
    const qc::Controls    controlQubits({qc::Control(firstControlQubit, qc::Control::Type::Neg),
                                         qc::Control(secondControlQubit, qc::Control::Type::Pos)});
    const auto            expectedXOperation = qc::StandardOperation(controlQubits, targetQubit, qc::X);

    auto       quantumComputation = qc::QuantumComputation(nQubits);
    const auto qmddPkg            = std::make_unique<dd::Package>(nQubits);
    applyMCXGateToQmdd(quantumComputation, *qmddPkg, dd::mEdge::one(), targetQubit, controlQubits);

    ASSERT_EQ(1U, quantumComputation.getNops());
    const auto& actualXOperation = quantumComputation.back();
    ASSERT_EQ(expectedXOperation, *actualXOperation);
}

TEST(QmddTransformationOperationsTest, ApplyMCXWithUnknownTargetQubit) {
    constexpr std::size_t nQubits           = 1U;
    constexpr dd::Qubit   targetQubit       = 1U;
    constexpr dd::Qubit   firstControlQubit = 0U;
    const qc::Controls    controlQubits({qc::Control(firstControlQubit, qc::Control::Type::Neg)});

    auto       quantumComputation = qc::QuantumComputation(nQubits);
    const auto qmddPkg            = std::make_unique<dd::Package>(nQubits);
    ASSERT_THROW(syrec::applyMCXGateToQmdd(quantumComputation, *qmddPkg, dd::mEdge::one(), targetQubit, controlQubits), std::out_of_range);
}

TEST(QmddTransformationOperationsTest, ApplyMCXWithUnknownControlQubit) {
    constexpr std::size_t nQubits           = 1U;
    constexpr dd::Qubit   targetQubit       = 0U;
    constexpr dd::Qubit   firstControlQubit = 1U;
    const qc::Controls    controlQubits({qc::Control(firstControlQubit, qc::Control::Type::Neg)});

    auto       quantumComputation = qc::QuantumComputation(nQubits);
    const auto qmddPkg            = std::make_unique<dd::Package>(nQubits);
    ASSERT_THROW(syrec::applyMCXGateToQmdd(quantumComputation, *qmddPkg, dd::mEdge::one(), targetQubit, controlQubits), std::out_of_range);
}

TEST(QmddTransformationOperationsTest, ApplyMCXGateWithTargetQubitMatchingControlQubit) {
    constexpr std::size_t nQubits     = 1U;
    constexpr dd::Qubit   targetQubit = 0U;
    const qc::Controls    controlQubits({qc::Control(targetQubit, qc::Control::Type::Neg)});

    auto       quantumComputation = qc::QuantumComputation(nQubits);
    const auto qmddPkg            = std::make_unique<dd::Package>(nQubits);
    ASSERT_THROW(syrec::applyMCXGateToQmdd(quantumComputation, *qmddPkg, dd::mEdge::one(), targetQubit, controlQubits), std::runtime_error);
}

TEST(QmddTransformationOperationsTest, TrySwapPathsOfEdgesOfQmddNodeWithPPrimeAndNPrimeEdgeContainingLessPathsThanPAndNEdge) {
    constexpr std::size_t nQubits            = 2U;
    const auto            qmddPkg            = std::make_unique<dd::Package>(nQubits);
    auto                  quantumComputation = qc::QuantumComputation(nQubits);

    const dd::mEdge& edgeN1QmddNode0      = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime);
    const dd::mEdge& edgePPrime1QmddNode0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::N);
    const dd::mEdge& edgeNPrime1QmddNode0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P | QmddNodeEdge::PPrime);
    const dd::mEdge& edgeP1QmddNode0      = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P);
    const dd::mEdge& edgeToQmddRootNode   = createNonLeafQmddNode(*qmddPkg, 1U, edgeN1QmddNode0, edgePPrime1QmddNode0, edgeNPrime1QmddNode0, edgeP1QmddNode0);
    qmddPkg->incRef(edgeToQmddRootNode);

    const NPathsToOneTerminalPerEdgeOfQmddNode& pathsFromRootToOneTerminals = getNPathsToOneTerminalPerEdgeOfQmddNode(*edgeToQmddRootNode.p);
    ASSERT_FALSE(syrec::trySwapPathsOfEdgesOfQmddNode(quantumComputation, *qmddPkg, pathsFromRootToOneTerminals));
    ASSERT_EQ(0U, quantumComputation.getNops());
}

TEST(QmddTransformationOperationsTest, TrySwapPathsOfEdgesOfQmddNodeWithPPrimeEdgeContainingMorePathsThanNEdgeFromQmddRoot) {
    constexpr std::size_t nQubits            = 3U;
    const auto            qmddPkg            = std::make_unique<dd::Package>(nQubits);
    auto                  quantumComputation = qc::QuantumComputation(nQubits);

    const dd::mEdge& edgePPrime2N1Node0      = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::PPrime | QmddNodeEdge::P);
    const dd::mEdge& edgePPrime2NPrime1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::NPrime | QmddNodeEdge::P);
    const dd::mEdge& edgePPrime2Node1        = createNonLeafQmddNode(*qmddPkg, 1U, dd::mEdge::zero(), dd::mEdge::zero(), edgePPrime2NPrime1Node0, edgePPrime2N1Node0);

    const dd::mEdge& edgeN2P1QmddNode0  = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::N | QmddNodeEdge::PPrime);
    const dd::mEdge& edgeN2QmddNode1    = createNonLeafQmddNode(*qmddPkg, 1U, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero(), edgeN2P1QmddNode0);
    const dd::mEdge& edgeToQmddRootNode = createNonLeafQmddNode(*qmddPkg, 2U, edgeN2QmddNode1, edgePPrime2Node1, dd::mEdge::zero(), dd::mEdge::zero());
    qmddPkg->incRef(edgeToQmddRootNode);

    const NPathsToOneTerminalPerEdgeOfQmddNode& pathsFromRootToOneTerminals = getNPathsToOneTerminalPerEdgeOfQmddNode(*edgeToQmddRootNode.p);
    ASSERT_TRUE(syrec::trySwapPathsOfEdgesOfQmddNode(quantumComputation, *qmddPkg, pathsFromRootToOneTerminals));
    ASSERT_EQ(1U, quantumComputation.getNops());

    const qc::Controls  controlQubits{};
    constexpr dd::Qubit targetQubit        = 2U;
    const auto          expectedXOperation = qc::StandardOperation(controlQubits, targetQubit, qc::X);
    ASSERT_EQ(expectedXOperation, *quantumComputation.back());
}

TEST(QmddTransformationOperationsTest, TrySwapPathsOfEdgesOfQmddNodeWithNPrimeEdgeContainingMorePathsThanPEdgeFromQmddRoot) {
    constexpr std::size_t nQubits            = 2U;
    const auto            qmddPkg            = std::make_unique<dd::Package>(nQubits);
    auto                  quantumComputation = qc::QuantumComputation(nQubits);

    const dd::mEdge& edgeNPrime1Node0   = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::N | QmddNodeEdge::NPrime);
    const dd::mEdge& edgeP1Node0        = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const dd::mEdge& edgeToQmddRootNode = createNonLeafQmddNode(*qmddPkg, 1U, dd::mEdge::zero(), dd::mEdge::zero(), edgeNPrime1Node0, edgeP1Node0);
    qmddPkg->incRef(edgeToQmddRootNode);

    const NPathsToOneTerminalPerEdgeOfQmddNode& pathsFromRootToOneTerminals = getNPathsToOneTerminalPerEdgeOfQmddNode(*edgeToQmddRootNode.p);
    ASSERT_TRUE(syrec::trySwapPathsOfEdgesOfQmddNode(quantumComputation, *qmddPkg, pathsFromRootToOneTerminals));
    ASSERT_EQ(1U, quantumComputation.getNops());

    const qc::Controls  controlQubits{};
    constexpr dd::Qubit targetQubit        = 1U;
    const auto          expectedXOperation = qc::StandardOperation(controlQubits, targetQubit, qc::X);
    ASSERT_EQ(expectedXOperation, *quantumComputation.back());
}

TEST(QmddTransformationOperationsTest, TrySwapPathsOfEdgesOfQmddNodeWithPPrimeEdgeContainingMorePathsThanNEdgeFromNonQmddRoot) {
    constexpr std::size_t nQubits            = 5U;
    const auto            qmddPkg            = std::make_unique<dd::Package>(nQubits);
    auto                  quantumComputation = qc::QuantumComputation(nQubits);

    const dd::mEdge& edgeP4N3PPrime2P1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::NPrime | QmddNodeEdge::P);
    const dd::mEdge& edgeP4N3PPrime2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::one(), edgeP4N3PPrime2P1Node0);

    const dd::mEdge& edgeP4N3N2N1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::NPrime);
    const dd::mEdge& edgeP4N3N2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, edgeP4N3N2N1Node0, dd::mEdge::zero(), dd::mEdge::one(), dd::mEdge::zero());

    const dd::mEdge& edgeP4N3Node2      = createNonLeafQmddNode(*qmddPkg, 2U, edgeP4N3N2Node1, edgeP4N3PPrime2Node1, dd::mEdge::zero(), dd::mEdge::zero());
    const dd::mEdge& edgeP4Node3        = createNonLeafQmddNode(*qmddPkg, 3U, edgeP4N3Node2, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    const dd::mEdge& edgeToQmddRootNode = createNonLeafQmddNode(*qmddPkg, 4U, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero(), edgeP4Node3);
    qmddPkg->incRef(edgeToQmddRootNode);

    const NPathsToOneTerminalPerEdgeOfQmddNode& pathsFromNonRootToOneTerminals = getNPathsToOneTerminalPerEdgeOfQmddNode(*edgeP4N3Node2.p);
    ASSERT_TRUE(syrec::trySwapPathsOfEdgesOfQmddNode(quantumComputation, *qmddPkg, pathsFromNonRootToOneTerminals));
    ASSERT_EQ(1U, quantumComputation.getNops());

    const qc::Controls  controlQubits{qc::Control(4U, qc::Control::Type::Pos), qc::Control(3U, qc::Control::Type::Neg)};
    constexpr dd::Qubit targetQubit        = 2U;
    const auto          expectedXOperation = qc::StandardOperation(controlQubits, targetQubit, qc::X);
    ASSERT_EQ(expectedXOperation, *quantumComputation.back());
}

TEST(QmddTransformationOperationsTest, TrySwapPathsOfEdgesOfQmddNodeWithNPrimeEdgeContainingMorePathsThanPEdgeFromNonQmddRoot) {
    constexpr std::size_t nQubits            = 5U;
    const auto            qmddPkg            = std::make_unique<dd::Package>(nQubits);
    auto                  quantumComputation = qc::QuantumComputation(nQubits);

    const dd::mEdge& edgeN4P3NPrime2P1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::NPrime | QmddNodeEdge::P);
    const dd::mEdge& edgeN4P3NPrime2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::one(), edgeN4P3NPrime2P1Node0);

    const dd::mEdge& edgeN4P3P2N1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::NPrime);
    const dd::mEdge& edgeN4P3P2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, dd::mEdge::one(), dd::mEdge::zero(), dd::mEdge::zero(), edgeN4P3P2N1Node0);

    const dd::mEdge& edgeN4P3Node2      = createNonLeafQmddNode(*qmddPkg, 2U, dd::mEdge::zero(), dd::mEdge::zero(), edgeN4P3NPrime2Node1, edgeN4P3P2Node1);
    const dd::mEdge& edgeN4Node3        = createNonLeafQmddNode(*qmddPkg, 3U, edgeN4P3Node2, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    const dd::mEdge& edgeToQmddRootNode = createNonLeafQmddNode(*qmddPkg, 4U, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero(), edgeN4Node3);
    qmddPkg->incRef(edgeToQmddRootNode);

    const NPathsToOneTerminalPerEdgeOfQmddNode& pathsFromNonRootToOneTerminals = getNPathsToOneTerminalPerEdgeOfQmddNode(*edgeN4P3Node2.p);
    ASSERT_TRUE(syrec::trySwapPathsOfEdgesOfQmddNode(quantumComputation, *qmddPkg, pathsFromNonRootToOneTerminals));
    ASSERT_EQ(1U, quantumComputation.getNops());

    const qc::Controls  controlQubits{qc::Control(4U, qc::Control::Type::Pos), qc::Control(3U, qc::Control::Type::Neg)};
    constexpr dd::Qubit targetQubit        = 2U;
    const auto          expectedXOperation = qc::StandardOperation(controlQubits, targetQubit, qc::X);
    ASSERT_EQ(expectedXOperation, *quantumComputation.back());
}

TEST(QmddTransformationOperationsTest, TryShiftUniquePathOfPPrimeEdgeToNEdgeOfQmddRoot) {
    const auto qmddPkg            = std::make_unique<dd::Package>(3U);
    auto       quantumComputation = qc::QuantumComputation(3U);

    const dd::mEdge& edgeN2N1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::N);
    const dd::mEdge& edgeN2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, edgeN2N1Node0, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());

    const dd::mEdge& edgePPrime2N1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const dd::mEdge& edgePPrime2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, edgePPrime2N1Node0, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    const dd::mEdge& edgeToQmddRoot     = createNonLeafQmddNode(*qmddPkg, 2U, edgeN2Node1, edgePPrime2Node1, dd::mEdge::zero(), dd::mEdge::zero());
    qmddPkg->incRef(edgeToQmddRoot);

    QmddNodeAndPathsPerEdge pathsFromRootToOneTerminals(*edgeToQmddRoot.p);
    getPathsToOneTerminalThroughEdgeOfQmddNode(*edgeToQmddRoot.p, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P, pathsFromRootToOneTerminals);
    ASSERT_TRUE(syrec::tryShiftUniquePathsOfQmddNode(quantumComputation, *qmddPkg, pathsFromRootToOneTerminals));
    ASSERT_EQ(1U, quantumComputation.getNops());

    const qc::Controls  controlQubits{qc::Control(1U, qc::Control::Type::Neg), qc::Control(0U, qc::Control::Type::Pos)};
    constexpr dd::Qubit targetQubit        = 2U;
    const auto          expectedXOperation = qc::StandardOperation(controlQubits, targetQubit, qc::X);
    ASSERT_EQ(expectedXOperation, *quantumComputation.back());
}

TEST(QmddTransformationOperationsTest, TryShiftUniquePathFromNPrimeEdgeToPEdgeOfQmddRoot) {
    const auto qmddPkg            = std::make_unique<dd::Package>(3U);
    auto       quantumComputation = qc::QuantumComputation(3U);

    const dd::mEdge& edgeNPrime2N1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::N);
    const dd::mEdge& edgeNPrime2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, dd::mEdge::zero(), dd::mEdge::zero(), edgeNPrime2N1Node0, dd::mEdge::zero());

    const dd::mEdge& edgeP2N1Node0  = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const dd::mEdge& edgeP2Node1    = createNonLeafQmddNode(*qmddPkg, 1U, edgeP2N1Node0, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    const dd::mEdge& edgeToQmddRoot = createNonLeafQmddNode(*qmddPkg, 2U, dd::mEdge::zero(), dd::mEdge::zero(), edgeNPrime2Node1, edgeP2Node1);
    qmddPkg->incRef(edgeToQmddRoot);

    QmddNodeAndPathsPerEdge pathsFromRootToOneTerminals(*edgeToQmddRoot.p);
    getPathsToOneTerminalThroughEdgeOfQmddNode(*edgeToQmddRoot.p, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P, pathsFromRootToOneTerminals);
    ASSERT_TRUE(syrec::tryShiftUniquePathsOfQmddNode(quantumComputation, *qmddPkg, pathsFromRootToOneTerminals));
    ASSERT_EQ(1U, quantumComputation.getNops());

    const qc::Controls  controlQubits{qc::Control(1U, qc::Control::Type::Neg), qc::Control(0U, qc::Control::Type::Neg)};
    constexpr dd::Qubit targetQubit        = 2U;
    const auto          expectedXOperation = qc::StandardOperation(controlQubits, targetQubit, qc::X);
    ASSERT_EQ(expectedXOperation, *quantumComputation.back());
}

TEST(QmddTransformationOperationsTest, TryShiftUniquePathFromPPrimeEdgeToNEdgeOfNonQmddRoot) {
    constexpr std::size_t nQubits            = 5;
    const auto            qmddPkg            = std::make_unique<dd::Package>(nQubits);
    auto                  quantumComputation = qc::QuantumComputation(nQubits);

    const dd::mEdge& edgeN4P3N2P1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::N);
    const dd::mEdge& edgeN4P3N2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero(), edgeN4P3N2P1Node0);

    const dd::mEdge& edgeN4P3PPrime2N1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const dd::mEdge& edgeN4P3PPrime2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, edgeN4P3PPrime2N1Node0, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());

    // Qmdd node on which shift operation is applied
    const dd::mEdge& edgeN4P3Node2  = createNonLeafQmddNode(*qmddPkg, 2U, edgeN4P3N2Node1, edgeN4P3PPrime2Node1, dd::mEdge::zero(), dd::mEdge::zero());
    const dd::mEdge& edgeN4Node3    = createNonLeafQmddNode(*qmddPkg, 3U, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero(), edgeN4P3Node2);
    const dd::mEdge& edgeToQmddRoot = createNonLeafQmddNode(*qmddPkg, 4U, edgeN4Node3, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    qmddPkg->incRef(edgeToQmddRoot);

    QmddNodeAndPathsPerEdge pathsFromNonRootToOneTerminals(*edgeN4P3Node2.p);
    getPathsToOneTerminalThroughEdgeOfQmddNode(*edgeN4P3Node2.p, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P, pathsFromNonRootToOneTerminals);
    ASSERT_TRUE(syrec::tryShiftUniquePathsOfQmddNode(quantumComputation, *qmddPkg, pathsFromNonRootToOneTerminals));
    ASSERT_EQ(1U, quantumComputation.getNops());

    const qc::Controls  controlQubits{qc::Control(4U, qc::Control::Type::Neg), qc::Control(3U, qc::Control::Type::Pos), qc::Control(1U, qc::Control::Type::Neg), qc::Control(0U, qc::Control::Type::Pos)};
    constexpr dd::Qubit targetQubit        = 2U;
    const auto          expectedXOperation = qc::StandardOperation(controlQubits, targetQubit, qc::X);
    ASSERT_EQ(expectedXOperation, *quantumComputation.back());
}

TEST(QmddTransformationOperationsTest, TryShiftUniquePathFromNPrimeEdgeToPEdgeOfNonQmddRoot) {
    constexpr std::size_t nQubits            = 5;
    const auto            qmddPkg            = std::make_unique<dd::Package>(nQubits);
    auto                  quantumComputation = qc::QuantumComputation(nQubits);

    const dd::mEdge& edgeN4P3P2N1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const dd::mEdge& edgeN4P3P2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, edgeN4P3P2N1Node0, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());

    const dd::mEdge& edgeN4P3NPrime2N1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::N);
    const dd::mEdge& edgeN4P3NPrime2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, edgeN4P3NPrime2N1Node0, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());

    // Qmdd node on which shift operation is applied
    const dd::mEdge& edgeN4P3Node2  = createNonLeafQmddNode(*qmddPkg, 2U, dd::mEdge::zero(), dd::mEdge::zero(), edgeN4P3NPrime2Node1, edgeN4P3P2Node1);
    const dd::mEdge& edgeN4Node3    = createNonLeafQmddNode(*qmddPkg, 3U, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero(), edgeN4P3Node2);
    const dd::mEdge& edgeToQmddRoot = createNonLeafQmddNode(*qmddPkg, 4U, edgeN4Node3, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    qmddPkg->incRef(edgeToQmddRoot);

    QmddNodeAndPathsPerEdge pathsFromNonRootToOneTerminals(*edgeN4P3Node2.p);
    getPathsToOneTerminalThroughEdgeOfQmddNode(*edgeN4P3Node2.p, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P, pathsFromNonRootToOneTerminals);
    ASSERT_TRUE(syrec::tryShiftUniquePathsOfQmddNode(quantumComputation, *qmddPkg, pathsFromNonRootToOneTerminals));
    ASSERT_EQ(1U, quantumComputation.getNops());

    const qc::Controls  controlQubits{qc::Control(4U, qc::Control::Type::Neg), qc::Control(3U, qc::Control::Type::Pos), qc::Control(1U, qc::Control::Type::Neg), qc::Control(0U, qc::Control::Type::Neg)};
    constexpr dd::Qubit targetQubit        = 2U;
    const auto          expectedXOperation = qc::StandardOperation(controlQubits, targetQubit, qc::X);
    ASSERT_EQ(expectedXOperation, *quantumComputation.back());
}

TEST(QmddTransformationOperationsTest, TryShiftUniquePathsFromPPrimeOrNPrimeEdgeContainingNoUniquePaths) {
    constexpr std::size_t nQubits            = 5;
    const auto            qmddPkg            = std::make_unique<dd::Package>(nQubits);
    auto                  quantumComputation = qc::QuantumComputation(nQubits);

    // Signatures of paths starting from NPrime and P Edge of qmdd node on which shift operation is applied must match so that operation does not succeed.
    const dd::mEdge& edgeN4P3P2N1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const dd::mEdge& edgeN4P3P2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, edgeN4P3P2N1Node0, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());

    const dd::mEdge& edgeN4P3NPrime2NPrime1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::PPrime);
    const dd::mEdge& edgeN4P3NPrime2Node1        = createNonLeafQmddNode(*qmddPkg, 1U, dd::mEdge::zero(), dd::mEdge::zero(), edgeN4P3NPrime2NPrime1Node0, dd::mEdge::zero());

    // Signatures of paths starting from PPrime and N Edge of qmdd node on which shift operation is applied must match so that operation does not succeed.
    const dd::mEdge& edgeN4P3N2Node1      = edgeN4P3P2Node1;
    const dd::mEdge& edgeN4P3PPrime2Node1 = edgeN4P3NPrime2Node1;

    // Qmdd node on which shift operation is applied to.
    const dd::mEdge& edgeN4P3Node2  = createNonLeafQmddNode(*qmddPkg, 2U, edgeN4P3N2Node1, edgeN4P3PPrime2Node1, edgeN4P3NPrime2Node1, edgeN4P3P2Node1);
    const dd::mEdge& edgeN4Node3    = createNonLeafQmddNode(*qmddPkg, 3U, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero(), edgeN4P3Node2);
    const dd::mEdge& edgeToQmddRoot = createNonLeafQmddNode(*qmddPkg, 4U, edgeN4Node3, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    qmddPkg->incRef(edgeToQmddRoot);

    QmddNodeAndPathsPerEdge pathsFromNonRootToOneTerminals(*edgeN4P3Node2.p);
    getPathsToOneTerminalThroughEdgeOfQmddNode(*edgeN4P3Node2.p, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P, pathsFromNonRootToOneTerminals);
    ASSERT_FALSE(syrec::tryShiftUniquePathsOfQmddNode(quantumComputation, *qmddPkg, pathsFromNonRootToOneTerminals));
    ASSERT_EQ(0U, quantumComputation.getNops());
}

TEST(QmddTransformationOperationsTest, TryShiftUniquePathsFromEmptyQmddNode) {
    const auto qmddPkg            = std::make_unique<dd::Package>(1U);
    auto       quantumComputation = qc::QuantumComputation(1U);

    const auto                    qmddNode = qmddPkg->makeDDNode<dd::mNode>(0U, std::array<dd::mEdge, 4U>({dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero()}));
    const QmddNodeAndPathsPerEdge pathsFromRootToOneTerminals(*qmddNode.p);
    ASSERT_FALSE(syrec::tryShiftUniquePathsOfQmddNode(quantumComputation, *qmddPkg, pathsFromRootToOneTerminals));
    ASSERT_EQ(0U, quantumComputation.getNops());
}

TEST(QmddTransformationOperationsTest, TryMakeSharedQmddPathUniqueInPPrimeEdgeOfRootNode) {
    constexpr std::size_t nQubits            = 3U;
    const auto            qmddPkg            = std::make_unique<dd::Package>(nQubits);
    auto                  quantumComputation = qc::QuantumComputation(nQubits);

    const dd::mEdge& edgeN2N1Node0      = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const dd::mEdge& edgeN2NPrime1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::N);
    const dd::mEdge& edgeN2Node1        = createNonLeafQmddNode(*qmddPkg, 1U, edgeN2N1Node0, dd::mEdge::zero(), edgeN2NPrime1Node0, dd::mEdge::zero());
    const dd::mEdge& edgePPrime2Node1   = edgeN2Node1;
    const dd::mEdge& edgeToQmddRoot     = createNonLeafQmddNode(*qmddPkg, 2U, edgeN2Node1, edgePPrime2Node1, dd::mEdge::zero(), dd::mEdge::zero());
    qmddPkg->incRef(edgeToQmddRoot);

    QmddNodeAndPathsPerEdge pathsFromRootToOneTerminals(*edgeToQmddRoot.p);
    getPathsToOneTerminalThroughEdgeOfQmddNode(*edgeToQmddRoot.p, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P, pathsFromRootToOneTerminals);
    ASSERT_TRUE(syrec::tryMakeSharedPathOfQmddNodeUnique(quantumComputation, *qmddPkg, pathsFromRootToOneTerminals));
    ASSERT_EQ(1U, quantumComputation.getNops());

    const qc::Controls  controlQubits{qc::Control(2U, qc::Control::Type::Pos)};
    constexpr dd::Qubit targetQubit        = 1U;
    const auto          expectedXOperation = qc::StandardOperation(controlQubits, targetQubit, qc::X);
    ASSERT_EQ(expectedXOperation, *quantumComputation.back());
}

TEST(QmddTransformationOperationsTest, TryMakeSharedQmddPathUniqueInNPrimeEdgeOfRootNode) {
    constexpr std::size_t nQubits            = 3U;
    const auto            qmddPkg            = std::make_unique<dd::Package>(nQubits);
    auto                  quantumComputation = qc::QuantumComputation(nQubits);

    const dd::mEdge& edgeP2P1Node0      = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const dd::mEdge& edgeP2NPrime1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const dd::mEdge& edgeP2Node1        = createNonLeafQmddNode(*qmddPkg, 1U, dd::mEdge::zero(), dd::mEdge::zero(), edgeP2NPrime1Node0, edgeP2P1Node0);

    const dd::mEdge& edgeNPrime2NPrime1Node0 = edgeP2NPrime1Node0;
    const dd::mEdge& edgeNPrime2Node1        = createNonLeafQmddNode(*qmddPkg, 1U, dd::mEdge::zero(), dd::mEdge::zero(), edgeNPrime2NPrime1Node0, dd::mEdge::zero());
    const dd::mEdge& edgeToQmddRoot          = createNonLeafQmddNode(*qmddPkg, 2U, dd::mEdge::zero(), dd::mEdge::zero(), edgeNPrime2Node1, edgeP2Node1);
    qmddPkg->incRef(edgeToQmddRoot);

    QmddNodeAndPathsPerEdge pathsFromRootToOneTerminals(*edgeToQmddRoot.p);
    getPathsToOneTerminalThroughEdgeOfQmddNode(*edgeToQmddRoot.p, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P, pathsFromRootToOneTerminals);
    ASSERT_TRUE(syrec::tryMakeSharedPathOfQmddNodeUnique(quantumComputation, *qmddPkg, pathsFromRootToOneTerminals));
    ASSERT_EQ(1U, quantumComputation.getNops());

    const qc::Controls  controlQubits{qc::Control(2U, qc::Control::Type::Neg), qc::Control(1U, qc::Control::Type::Neg)};
    constexpr dd::Qubit targetQubit        = 0U;
    const auto          expectedXOperation = qc::StandardOperation(controlQubits, targetQubit, qc::X);
    ASSERT_EQ(expectedXOperation, *quantumComputation.back());
}

TEST(QmddTransformationOperationsTest, TryMakeSharedQmddPathUniqueInPPrimeEdgeOfNonRootNode) {
    constexpr std::size_t nQubits            = 4U;
    const auto            qmddPkg            = std::make_unique<dd::Package>(nQubits);
    auto                  quantumComputation = qc::QuantumComputation(nQubits);

    const dd::mEdge& edgeP3N2X1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const dd::mEdge& edgeP3N2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, edgeP3N2X1Node0, edgeP3N2X1Node0, dd::mEdge::zero(), dd::mEdge::zero());

    const dd::mEdge& edgeP3PPrime2N1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const dd::mEdge& edgeP3PPrime2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, edgeP3PPrime2N1Node0, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    // Qmdd node whose shared path between the N and PPrime should be made unique.
    const dd::mEdge& edgeP3Node2    = createNonLeafQmddNode(*qmddPkg, 2U, edgeP3N2Node1, edgeP3PPrime2Node1, dd::mEdge::zero(), dd::mEdge::zero());
    const dd::mEdge& edgeToQmddRoot = createNonLeafQmddNode(*qmddPkg, 3U, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero(), edgeP3Node2);
    qmddPkg->incRef(edgeToQmddRoot);

    QmddNodeAndPathsPerEdge pathsFromNonRootToOneTerminals(*edgeP3Node2.p);
    getPathsToOneTerminalThroughEdgeOfQmddNode(*edgeP3Node2.p, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P, pathsFromNonRootToOneTerminals);
    ASSERT_TRUE(syrec::tryMakeSharedPathOfQmddNodeUnique(quantumComputation, *qmddPkg, pathsFromNonRootToOneTerminals));
    ASSERT_EQ(1U, quantumComputation.getNops());

    const qc::Controls  controlQubits{qc::Control(3U, qc::Control::Type::Pos), qc::Control(2U, qc::Control::Type::Pos), qc::Control(1U, qc::Control::Type::Neg)};
    constexpr dd::Qubit targetQubit        = 0U;
    const auto          expectedXOperation = qc::StandardOperation(controlQubits, targetQubit, qc::X);
    ASSERT_EQ(expectedXOperation, *quantumComputation.back());
}

TEST(QmddTransformationOperationsTest, TryMakeSharedQmddPathUniqueInNPrimeEdgeOfNonRootNode) {
    constexpr std::size_t nQubits            = 4U;
    const auto            qmddPkg            = std::make_unique<dd::Package>(nQubits);
    auto                  quantumComputation = qc::QuantumComputation(nQubits);

    const dd::mEdge& edgeN3P2X1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const dd::mEdge& edgeN3P2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, edgeN3P2X1Node0, edgeN3P2X1Node0, dd::mEdge::zero(), dd::mEdge::zero());

    const dd::mEdge& edgeN3NPrime2N1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const dd::mEdge& edgeN3NPrime2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, edgeN3NPrime2N1Node0, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    // Qmdd node whose shared path between the P and NPrime should be made unique.
    const dd::mEdge& edgeN3Node2    = createNonLeafQmddNode(*qmddPkg, 2U, dd::mEdge::zero(), dd::mEdge::zero(), edgeN3NPrime2Node1, edgeN3P2Node1);
    const dd::mEdge& edgeToQmddRoot = createNonLeafQmddNode(*qmddPkg, 3U, edgeN3Node2, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    qmddPkg->incRef(edgeToQmddRoot);

    QmddNodeAndPathsPerEdge pathsFromNonRootToOneTerminals(*edgeN3Node2.p);
    getPathsToOneTerminalThroughEdgeOfQmddNode(*edgeN3Node2.p, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P, pathsFromNonRootToOneTerminals);
    ASSERT_TRUE(syrec::tryMakeSharedPathOfQmddNodeUnique(quantumComputation, *qmddPkg, pathsFromNonRootToOneTerminals));
    ASSERT_EQ(1U, quantumComputation.getNops());

    const qc::Controls  controlQubits{qc::Control(3U, qc::Control::Type::Neg), qc::Control(2U, qc::Control::Type::Neg), qc::Control(1U, qc::Control::Type::Neg)};
    constexpr dd::Qubit targetQubit        = 0U;
    const auto          expectedXOperation = qc::StandardOperation(controlQubits, targetQubit, qc::X);
    ASSERT_EQ(expectedXOperation, *quantumComputation.back());
}

TEST(QmddTransformationOperationsTest, TryMakeSharedQmddPathUniqueInEmptyQmddNode) {
    const auto qmddPkg            = std::make_unique<dd::Package>(1U);
    auto       quantumComputation = qc::QuantumComputation(1U);

    const auto                    qmddNode = qmddPkg->makeDDNode<dd::mNode>(0U, std::array<dd::mEdge, 4U>({dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero()}));
    const QmddNodeAndPathsPerEdge pathsFromRootToOneTerminals(*qmddNode.p);
    ASSERT_FALSE(syrec::tryMakeSharedPathOfQmddNodeUnique(quantumComputation, *qmddPkg, pathsFromRootToOneTerminals));
    ASSERT_EQ(0U, quantumComputation.getNops());
}

TEST(QmddTransformationOperationsTest, TryMakeSharedQmddPathUniqueInPPrimeEdgeOfRootNodeButSharedPathCannotBeMadeUnique) {
    constexpr std::size_t nQubits            = 3U;
    const auto            qmddPkg            = std::make_unique<dd::Package>(nQubits);
    auto                  quantumComputation = qc::QuantumComputation(nQubits);

    const dd::mEdge& edgePPrime2N1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const dd::mEdge& edgePPrime2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, edgePPrime2N1Node0, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    // Qmdd node whose shared path between the N and PPrime should be made unique.
    const dd::mEdge& edgeToQmddRoot = createNonLeafQmddNode(*qmddPkg, 2U, dd::mEdge::one(), edgePPrime2Node1, dd::mEdge::zero(), dd::mEdge::zero());
    qmddPkg->incRef(edgeToQmddRoot);

    QmddNodeAndPathsPerEdge pathsFromRootToOneTerminals(*edgeToQmddRoot.p);
    getPathsToOneTerminalThroughEdgeOfQmddNode(*edgeToQmddRoot.p, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P, pathsFromRootToOneTerminals);
    ASSERT_FALSE(syrec::tryMakeSharedPathOfQmddNodeUnique(quantumComputation, *qmddPkg, pathsFromRootToOneTerminals));
    ASSERT_EQ(0U, quantumComputation.getNops());
}

TEST(QmddTransformationOperationsTest, TryMakeSharedQmddPathUniqueInNPrimeEdgeOfRootNodeButSharedPathCannotBeMadeUnique) {
    constexpr std::size_t nQubits            = 3U;
    const auto            qmddPkg            = std::make_unique<dd::Package>(nQubits);
    auto                  quantumComputation = qc::QuantumComputation(nQubits);

    const dd::mEdge& edgeNPrime2N1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const dd::mEdge& edgeNPrime2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, edgeNPrime2N1Node0, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    // Qmdd node whose shared path between the P and NPrime should be made unique.
    const dd::mEdge& edgeToQmddRoot = createNonLeafQmddNode(*qmddPkg, 2U, dd::mEdge::zero(), dd::mEdge::zero(), edgeNPrime2Node1, dd::mEdge::one());
    qmddPkg->incRef(edgeToQmddRoot);

    QmddNodeAndPathsPerEdge pathsFromRootToOneTerminals(*edgeToQmddRoot.p);
    getPathsToOneTerminalThroughEdgeOfQmddNode(*edgeToQmddRoot.p, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P, pathsFromRootToOneTerminals);
    ASSERT_FALSE(syrec::tryMakeSharedPathOfQmddNodeUnique(quantumComputation, *qmddPkg, pathsFromRootToOneTerminals));
    ASSERT_EQ(0U, quantumComputation.getNops());
}

TEST(QmddTransformationOperationsTest, TryMakeSharedQmddPathUniqueInPPrimeEdgeOfNonRootNodeButSharedPathCannotBeMadeUnique) {
    constexpr std::size_t nQubits            = 4U;
    const auto            qmddPkg            = std::make_unique<dd::Package>(nQubits);
    auto                  quantumComputation = qc::QuantumComputation(nQubits);

    const dd::mEdge& edgeP3PPrime2N1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const dd::mEdge& edgeP3PPrime2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, edgeP3PPrime2N1Node0, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    // Qmdd node whose shared path between the N and PPrime should be made unique.
    const dd::mEdge& edgeP3Node2    = createNonLeafQmddNode(*qmddPkg, 2U, dd::mEdge::one(), edgeP3PPrime2Node1, dd::mEdge::zero(), dd::mEdge::zero());
    const dd::mEdge& edgeToQmddRoot = createNonLeafQmddNode(*qmddPkg, 3U, edgeP3Node2, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    qmddPkg->incRef(edgeToQmddRoot);

    QmddNodeAndPathsPerEdge pathsFromNonRootToOneTerminals(*edgeP3Node2.p);
    getPathsToOneTerminalThroughEdgeOfQmddNode(*edgeP3Node2.p, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P, pathsFromNonRootToOneTerminals);
    ASSERT_FALSE(syrec::tryMakeSharedPathOfQmddNodeUnique(quantumComputation, *qmddPkg, pathsFromNonRootToOneTerminals));
    ASSERT_EQ(0U, quantumComputation.getNops());
}

TEST(QmddTransformationOperationsTest, TryMakeSharedQmddPathUniqueInNPrimeEdgeOfNonRootNodeButSharedPathCannotBeMadeUnique) {
    constexpr std::size_t nQubits            = 4U;
    const auto            qmddPkg            = std::make_unique<dd::Package>(nQubits);
    auto                  quantumComputation = qc::QuantumComputation(nQubits);

    const dd::mEdge& edgeP3NPrime2N1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const dd::mEdge& edgeP3NPrime2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, edgeP3NPrime2N1Node0, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    // Qmdd node whose shared path between the P and NPrime should be made unique.
    const dd::mEdge& edgeP3Node2    = createNonLeafQmddNode(*qmddPkg, 2U, dd::mEdge::zero(), dd::mEdge::zero(), edgeP3NPrime2Node1, dd::mEdge::one());
    const dd::mEdge& edgeToQmddRoot = createNonLeafQmddNode(*qmddPkg, 3U, edgeP3Node2, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    qmddPkg->incRef(edgeToQmddRoot);

    QmddNodeAndPathsPerEdge pathsFromNonRootToOneTerminals(*edgeP3Node2.p);
    getPathsToOneTerminalThroughEdgeOfQmddNode(*edgeP3Node2.p, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P, pathsFromNonRootToOneTerminals);
    ASSERT_FALSE(syrec::tryMakeSharedPathOfQmddNodeUnique(quantumComputation, *qmddPkg, pathsFromNonRootToOneTerminals));
    ASSERT_EQ(0U, quantumComputation.getNops());
}

TEST(QmddTransformationOperationsTest, GetEdgeToRootOfQmddContainingMultipleEntries) {
    const auto  qmddPkg               = std::make_unique<dd::Package>(3U);
    const auto& edgeToLowestQmddNode  = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::PPrime | QmddNodeEdge::NPrime);
    const auto& edgeToSubtreeQmddNode = createNonLeafQmddNode(*qmddPkg, 1U, dd::mEdge::one(), edgeToLowestQmddNode, edgeToLowestQmddNode, dd::mEdge::one());
    const auto& edgeToRootNode        = createNonLeafQmddNode(*qmddPkg, 2U, dd::mEdge::one(), edgeToSubtreeQmddNode, dd::mEdge::zero(), dd::mEdge::zero());
    qmddPkg->incRef(edgeToRootNode);

    const dd::mEdge* edgeToRootOfQmdd = tryGetEdgeToQmddRootNode(*qmddPkg);
    ASSERT_THAT(edgeToRootOfQmdd, testing::NotNull());
    ASSERT_EQ(2U, edgeToRootOfQmdd->p->v);
}

TEST(QmddTransformationOperationsTest, GetEdgeOfRootOfEmptyQmdd) {
    const auto       qmddPkg          = std::make_unique<dd::Package>(0U);
    const dd::mEdge* edgeToRootOfQmdd = tryGetEdgeToQmddRootNode(*qmddPkg);
    ASSERT_THAT(edgeToRootOfQmdd, testing::IsNull());
}

TEST(QmddTransformationOperationsTest, CheckTerminateConditionOfQmddNodeWithPPrimeSubtreeNotPointingToZeroTerminal) {
    const auto  qmddPkg               = std::make_unique<dd::Package>(3U);
    const auto& edgeToLowestQmddNode  = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::PPrime | QmddNodeEdge::NPrime);
    const auto& edgeToSubtreeQmddNode = createNonLeafQmddNode(*qmddPkg, 1U, dd::mEdge::one(), edgeToLowestQmddNode, edgeToLowestQmddNode, dd::mEdge::one());
    const auto& edgeToRootNode        = createNonLeafQmddNode(*qmddPkg, 2U, dd::mEdge::one(), edgeToSubtreeQmddNode, dd::mEdge::zero(), dd::mEdge::zero());
    ;
    ASSERT_FALSE(syrec::terminate(*edgeToRootNode.p));
}

TEST(QmddTransformationOperationsTest, CheckTerminateConditionOfQmddNodeWithPPrimeSubtreePointingToZeroTerminal) {
    const auto  qmddPkg              = std::make_unique<dd::Package>(1U);
    const auto& edgeToRootOfQmddTree = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::N);
    ASSERT_TRUE(syrec::terminate(*edgeToRootOfQmddTree.p));
}

TEST(QmddTransformationOperationsTest, CheckTerminateConditionOfQmddNodeWithPPrimeSubtreePointingToOneTerminal) {
    const auto  qmddPkg              = std::make_unique<dd::Package>(1U);
    const auto& edgeToRootOfQmddTree = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::N | QmddNodeEdge::PPrime);
    ASSERT_FALSE(syrec::terminate(*edgeToRootOfQmddTree.p));
}

TEST(QmddTransformationOperationsTest, Test) {
    constexpr std::size_t nQubits            = 4U;
    const auto            qmddPkg            = std::make_unique<dd::Package>(nQubits);
    auto                  quantumComputation = qc::QuantumComputation(nQubits);

    const dd::mEdge& edgeP3P2P1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::PPrime | QmddNodeEdge::NPrime);
    const dd::mEdge& edgeP3P2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, dd::mEdge::one(), dd::mEdge::zero(), dd::mEdge::zero(), edgeP3P2P1Node0);
    const dd::mEdge& edgeP3Node2     = createNonLeafQmddNode(*qmddPkg, 2U, dd::mEdge::one(), dd::mEdge::zero(), dd::mEdge::zero(), edgeP3P2Node1);
    const dd::mEdge& edgeToQmddRoot  = createNonLeafQmddNode(*qmddPkg, 3U, dd::mEdge::one(), dd::mEdge::zero(), dd::mEdge::zero(), edgeP3Node2);
    qmddPkg->incRef(edgeToQmddRoot);

    //QmddNodeAndPathsPerEdge pathsFromNonRootToOneTerminals(*edgeToQmddRoot.p);
    //getPathsToOneTerminalThroughEdgeOfQmddNode(*edgeToQmddRoot.p, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P, pathsFromNonRootToOneTerminals);
    QmddNodeAndPathsPerEdge pathsFromNonRootToOneTerminals(*edgeP3P2P1Node0.p);
    getPathsToOneTerminalThroughEdgeOfQmddNode(*edgeP3P2P1Node0.p, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P, pathsFromNonRootToOneTerminals);
    const auto x          = getNPathsToOneTerminalPerEdgeOfQmddNode(*edgeP3P2P1Node0.p);
    bool       resetQueue = trySwapPathsOfEdgesOfQmddNode(quantumComputation, *qmddPkg, getNPathsToOneTerminalPerEdgeOfQmddNode(*edgeP3P2P1Node0.p));

    ASSERT_FALSE(syrec::tryMakeSharedPathOfQmddNodeUnique(quantumComputation, *qmddPkg, pathsFromNonRootToOneTerminals));
    ASSERT_EQ(0U, quantumComputation.getNops());
}
