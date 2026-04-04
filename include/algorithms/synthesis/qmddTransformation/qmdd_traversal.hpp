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
#include <vector>

namespace syrec {
    struct QmddNodeAndPathsPerEdge {
        std::reference_wrapper<const dd::mNode> associatedQmddNode;
        std::vector<OptimizedQmddPath>          nEdgePaths;
        std::vector<OptimizedQmddPath>          pPrimeEdgePaths;
        std::vector<OptimizedQmddPath>          nPrimeEdgePaths;
        std::vector<OptimizedQmddPath>          pEdgePaths;
    };

    void getPathsToOneTerminalThroughEdgeOfQmddNode(const dd::mNode& qmddNodeToStartPathsFrom, QmddNodeEdge edgesToGeneratePathsFor, QmddNodeAndPathsPerEdge& containerStoringFoundPaths);
    // TODO: A node should only be reachable from the root node by traversing either the P or N edge of the traversed node until the searched for node is found.
    // TODO: "Truncated" nodes from the "original" qmdd root to the current qmdd root should be ignorable?
    std::vector<UnoptimizedQmddPath> getAllPathsFromRootToNode(const dd::mNode& qmddRootNode, const dd::mNode& qmddNodeToReach);
} // namespace syrec
