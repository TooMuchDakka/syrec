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
            std::optional<dd::Qubit> qubitAssociatedWithQmddNodeThatHasIncomingEdge;
            QmddEdgeIndex            incomingEdgeFromParentQmddNode;
        };

        using QmddPath = std::vector<QmddPathComponent>;

        struct QmddPathsStartingFromNode {
            std::reference_wrapper<dd::mNode> associatedQmddNode;
            std::vector<QmddPath>             nEdgePaths;
            std::vector<QmddPath>             pPrimeEdgePaths;
            std::vector<QmddPath>             nPrimeEdgePaths;
            std::vector<QmddPath>             pEdgePaths;
        };

        struct QmddEdgeTraversalHelper {
            const dd::mNode* refToQmddNode;
            QmddEdgeIndex    lastProcessedEdge;
            bool             visitedAllEdgesOfNodeFlag;
        };

        std::reference_wrapper<qc::QuantumComputation> qc;
        std::reference_wrapper<dd::Package>            qmddPackage;

        void applyOperation(qc::Qubit targetBit, dd::mEdge& to, const qc::Controls& ctrl) const;

        [[nodiscard]] bool swapPaths(dd::mEdge& src, const QmddPathsStartingFromNode& qmddNodePathSignatures) const;
        void               shiftUniquePaths(dd::mEdge& src, QmddPathsStartingFromNode& qmddNodePathSignatures) const;
        void               makePathsOfEdgeOfQmddNodeUnique(dd::mEdge& src, QmddEdgeIndex processedEdge, QmddPathsStartingFromNode& qmddNodePathSignatures) const;
        void               shiftPathsFromPPrimeEdgeToNPrimeEdge(dd::mEdge& src) const;

        [[nodiscard]] static bool                       terminate(const dd::mNode& nodeToCheck);
        [[maybe_unused]] static constexpr QmddEdgeIndex increment(QmddEdgeIndex& qmddEdgeIndex) noexcept;
        [[maybe_unused]] static constexpr QmddEdgeIndex decrement(QmddEdgeIndex& qmddEdgeIndex) noexcept;
        [[nodiscard]] static constexpr bool             getBooleanSignatureComponentForQmddEdge(QmddEdgeIndex qmddEdgeIndex) noexcept;

        friend constexpr QmddEdgeIndex               operator&(QmddEdgeIndex lOperand, QmddEdgeIndex rOperand) noexcept;
        friend constexpr QmddEdgeIndex               operator|(QmddEdgeIndex lOperand, QmddEdgeIndex rOperand) noexcept;
        friend constexpr void                        operator|=(QmddEdgeIndex& assignedToOperand, QmddEdgeIndex rOperand) noexcept;
        [[nodiscard]] static std::optional<QmddPath> determineUniquePathFromCollection(const std::vector<QmddPath>& qmddPaths);

        // TODO: How should garbage qubits be handled? Can their path components be skipped?
        // TODO: One should be able to pass a whole existing path signature as a parameter to define the path from the root to the current node
        // TODO: Memoize intermediate results?
        [[nodiscard]] static std::vector<QmddPath>       getAllPathsStartingFromNode(const dd::mNode* node, QmddEdgeIndex qmddPathToTake);
        [[nodiscard]] static qc::Controls                getControlQubitsForQmddPathFromRootToNode(qc::Qubit qubitAssociatedToQmddRootNode, const QmddPath& qmddPath);
        [[nodiscard]] static qc::Controls                getControlQubitsForPathFromQmddNodeToTerminal(const QmddPath& qmddPath);
        [[nodiscard]] static std::vector<QmddPath>       getAllPathsFromRootToNode(const dd::mNode& root, const dd::mNode& node);
        [[nodiscard]] static std::vector<std::size_t>    getIndicesOfUniquePathsForQmddNodeEdge(const std::vector<QmddPath>& collectionOfPathsToExtractUniqueOnesFrom, const std::vector<QmddPath>& collectionOfPathsUsedToIdentifyDuplicates);
        [[nodiscard]] static std::optional<std::size_t>  getIndexOfFirstSharedPathBetweenQmddNodeEdgeSubtrees(const std::vector<QmddPath>& collectionOfPathsToFindSharedOneFrom, const std::vector<QmddPath>& collectionUsedToDetermineWhetherDuplicatePathExists);
        [[nodiscard]] static const dd::mNode*            getRootNode(dd::Package& qmddPackage);
        [[nodiscard]] static bool                        doQmddPathsOverlap(const QmddPath& lQmddPath, const QmddPath& rQmddPath);
        [[nodiscard]] static constexpr qc::Control::Type getControlQubitPolarityForQmddEdge(QmddEdgeIndex qmddEdge) noexcept;
        [[nodiscard]] static std::optional<qc::Qubit>    getQubitOfQmddNodeReachedByEdge(const dd::mNode* qmddNodeBeingOriginOfEdge, QmddEdgeIndex edgeToTake);
    };
} // namespace syrec
