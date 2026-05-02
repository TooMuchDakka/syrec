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

#include "algorithms/synthesis/qmddTransformation/qmdd_path_definitions.hpp"
#include "dd/Node.hpp"

#include <functional>
#include <optional>
#include <vector>

namespace syrec {
    struct QmddNodeAndPathsPerEdge {
        std::reference_wrapper<const dd::mNode>        associatedQmddNode;
        std::array<std::vector<OptimizedQmddPath>, 4U> pathsPerEdgeLookup{};

        QmddNodeAndPathsPerEdge() = delete;
        explicit QmddNodeAndPathsPerEdge(const dd::mNode& associatedQmddNode): associatedQmddNode(associatedQmddNode) {}

        const std::vector<OptimizedQmddPath>& operator[](QmddNodeEdge qmddNodeEdge) const;
        std::vector<OptimizedQmddPath>&       operator[](QmddNodeEdge qmddNodeEdge);
    };

    struct NPathsToOneTerminalPerEdgeOfQmddNode {
        std::reference_wrapper<const dd::mNode> associatedQmddNode;
        std::array<std::size_t, 4>              nPathsPerEdgeLookup{0U, 0U, 0U, 0U};

        NPathsToOneTerminalPerEdgeOfQmddNode() = delete;
        explicit NPathsToOneTerminalPerEdgeOfQmddNode(const dd::mNode& associatedQmddNode): associatedQmddNode(associatedQmddNode) {}

        std::size_t  operator[](QmddNodeEdge qmddNodeEdge) const;
        std::size_t& operator[](QmddNodeEdge qmddNodeEdge);
    };

    [[nodiscard]] NPathsToOneTerminalPerEdgeOfQmddNode getNPathsToOneTerminalPerEdgeOfQmddNode(const dd::mNode& qmddNode);
    void                                               getPathsToOneTerminalThroughEdgeOfQmddNode(const dd::mNode& qmddNodeToStartPathsFrom, QmddNodeEdge edgesToGeneratePathsFor, QmddNodeAndPathsPerEdge& containerStoringFoundPaths);
    [[nodiscard]] std::vector<UnoptimizedQmddPath>     getAllPathsFromRootToNode(const dd::mNode& qmddRootNode, const dd::mNode& qmddNodeToReach);
} // namespace syrec
