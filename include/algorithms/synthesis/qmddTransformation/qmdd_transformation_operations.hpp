/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#pragma once

#include "algorithms/synthesis/qmddTransformation/qmdd_traversal.hpp"
#include "dd/Node.hpp"
#include "dd/Package.hpp"
#include "ir/QuantumComputation.hpp"

namespace syrec {
    void               applyMCXGateToQmdd(qc::QuantumComputation& quantumComputation, dd::Package& qmddPkg, const dd::mEdge& edgeToRootNodeOfQmdd, qc::Qubit targetQubit, const qc::Controls& controlQubits);
    [[nodiscard]] bool trySwapPathsOfEdgesOfQmddNode(qc::QuantumComputation& quantumComputation, dd::Package& qmddPkg, const NPathsToOneTerminalPerEdgeOfQmddNode& nPathsToOneTerminalPerEdgeOfQmddNode);
    [[nodiscard]] bool tryShiftUniquePathsOfQmddNode(qc::QuantumComputation& quantumComputation, dd::Package& qmddPkg, const QmddNodeAndPathsPerEdge& qmddNodeAndEdgePaths);
    [[nodiscard]] bool tryMakeSharedPathOfQmddNodeUnique(qc::QuantumComputation& quantumComputation, dd::Package& qmddPkg, const QmddNodeAndPathsPerEdge& qmddNodeAndEdgePaths);

    [[nodiscard]] const dd::mEdge* tryGetEdgeToQmddRootNode(dd::Package& qmddPkgToGetRootFrom);
    [[nodiscard]] bool             terminate(const dd::mNode& nodeToCheck);
} // namespace syrec
