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

#include "core/truthTable/truth_table.hpp"
#include "dd/DDDefinitions.hpp"
#include "ir/QuantumComputation.hpp"

#include <cstdint>
#include <vector>

namespace syrec {
    class QmddToIdentityTransformer {
    public:
        explicit QmddToIdentityTransformer(const std::reference_wrapper<qc::QuantumComputation> quantumComputation, const std::reference_wrapper<dd::Package> qmddPackage): qc(quantumComputation), qmddPackage(qmddPackage) {}

        [[nodiscard]] bool synthesize(dd::mEdge src);

    protected:
        enum class QmddEdgeIndex : std::uint8_t {
            N_Path       = 0,
            P_Prime_Path = 1,
            N_Prime_Path = 2,
            P_Path       = 3
        };

        struct QmddPathComponent {
            dd::Qubit     qubitAssociatedWithNodeDefiningOriginOfEdge;
            QmddEdgeIndex edgeIndex;
        };

        using QmddPath = std::vector<QmddPathComponent>;

        struct QmddPathsStartingFromNode {
            std::vector<QmddPath> nEdgePaths;
            std::vector<QmddPath> pPrimeEdgePaths;
            std::vector<QmddPath> nPrimeEdgePaths;
            std::vector<QmddPath> pEdgePaths;
        };

        struct QmddEdgeTraversalHelper {
            const dd::mNode* refToQmddNode;
            QmddEdgeIndex    lastProcessedEdge;
            bool             visitedAllEdgesOfNodeFlag;
        };

        std::reference_wrapper<qc::QuantumComputation> qc;
        std::reference_wrapper<dd::Package>            qmddPackage;

        void applyOperation(qc::Qubit targetBit, dd::mEdge& to, const qc::Controls& ctrl) const;

        [[nodiscard]] dd::mEdge swapPaths(dd::mEdge src, dd::mEdge const& current, const QmddPathsStartingFromNode& qmddNodePathSignatures);
        [[nodiscard]] dd::mEdge shiftUniquePaths(dd::mEdge src, const dd::mEdge& current, const QmddPathsStartingFromNode& qmddNodePathSignatures);
        [[nodiscard]] dd::mEdge unifyPath(dd::mEdge src, const dd::mEdge& current, const QmddPathsStartingFromNode& p1SigVec, const QmddPathsStartingFromNode& p2SigVec);
        [[nodiscard]] dd::mEdge shiftingPaths(const dd::mEdge& src, const dd::mEdge& current);

        [[nodiscard]] static bool                       terminate(const dd::mNode& nodeToCheck);
        [[maybe_unused]] static constexpr QmddEdgeIndex increment(QmddEdgeIndex& qmddEdgeIndex) noexcept;
        [[nodiscard]] static constexpr bool             getBooleanSignatureComponentForQmddEdge(QmddEdgeIndex qmddEdgeIndex) noexcept;

        // TODO: How should garbage qubits be handled? Can their path components be skipped?
        // TODO: One should be able to pass a whole existing path signature as a parameter to define the path from the root to the current node
        // TODO: Memoize intermediate results?
        [[nodiscard]] static TruthTable::Cube::Set getAllPathSignaturesStartingFromNode(QmddEdgeIndex qmddPathTakenToReachNode, const dd::mNode& node);
        [[nodiscard]] static std::vector<QmddPath> getAllPathsStartingFromNode(QmddEdgeIndex qmddPathTakenToReachNode, const dd::mNode& node);
        [[nodiscard]] static qc::Controls          getControlsQubitsFromQmddPath(const QmddPath& qmddPath);
        [[nodiscard]] static std::vector<QmddPath> getAllPathsFromRootToNode(const dd::mNode& root, const dd::mNode& node);
        [[nodiscard]] static std::vector<QmddPath> getUniquePathsForQmddNodeEdge(const std::vector<QmddPath>& collectionOfPathsToExtractUniqueOnesFrom, const std::vector<QmddPath>& collectionOfPathsUsedToIdentifyDuplicates);
        [[nodiscard]] static const dd::mNode*      getRootNode(dd::Package& qmddPackage);
    };
} // namespace syrec
