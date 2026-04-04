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

#include "dd/DDDefinitions.hpp"
#include "dd/Node.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <optional>
#include <stack>
#include <vector>

namespace syrec {
    struct VisitedQmddNodeEdgesAggregation {
        const dd::mNode*            associatedQmddNode;
        std::optional<QmddNodeEdge> visitedEdges;
        std::optional<QmddNodeEdge> currVisitedEdge;

        explicit VisitedQmddNodeEdgesAggregation(const dd::mNode* associatedQmddNode):
            associatedQmddNode(associatedQmddNode) {}

        [[nodiscard]] constexpr bool visitedAllEdges() const noexcept {
            return visitedEdges.has_value() && (*visitedEdges & QmddNodeEdge::N && *visitedEdges & QmddNodeEdge::PPrime && *visitedEdges & QmddNodeEdge::NPrime && *visitedEdges & QmddNodeEdge::P);
        }

        [[maybe_unused]] QmddNodeEdge advanceToNextEdge() noexcept {
            if (!visitedEdges.has_value()) {
                currVisitedEdge = QmddNodeEdge::N;
                visitedEdges    = QmddNodeEdge::N;
                return *currVisitedEdge;
            }
            bool                 advancedToNextEdge = false;
            constexpr std::array qmddNodeEdges{QmddNodeEdge::P, QmddNodeEdge::NPrime, QmddNodeEdge::PPrime, QmddNodeEdge::N};
            for (std::size_t i = 0; i < 4 && !advancedToNextEdge; ++i) {
                if (const QmddNodeEdge qmddNodeEdge = qmddNodeEdges[i]; *visitedEdges & qmddNodeEdge) {
                    switch (qmddNodeEdge) {
                        case QmddNodeEdge::N:
                            currVisitedEdge    = QmddNodeEdge::PPrime;
                            advancedToNextEdge = true;
                            break;
                        case QmddNodeEdge::PPrime:
                            currVisitedEdge    = QmddNodeEdge::NPrime;
                            advancedToNextEdge = true;
                            break;
                        case QmddNodeEdge::NPrime:
                            currVisitedEdge    = QmddNodeEdge::P;
                            advancedToNextEdge = true;
                            break;
                        case QmddNodeEdge::P:
                            currVisitedEdge    = QmddNodeEdge::N;
                            advancedToNextEdge = true;
                            break;
                        default:
                            break;
                    }
                }
            }
            *visitedEdges |= *currVisitedEdge;
            return *currVisitedEdge;
        }

        [[maybe_unused]] std::optional<QmddNodeEdge> advanceToNextEdgeInPathFromRootToNode() noexcept {
            if (!visitedEdges.has_value()) {
                visitedEdges = QmddNodeEdge::N;
                return QmddNodeEdge::N;
            }

            if (*visitedEdges & QmddNodeEdge::N) {
                *visitedEdges = QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P;
                return QmddNodeEdge::P;
            }
            return std::nullopt;
        }

        void markAllEdgesAsVisited() noexcept {
            visitedEdges = QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P;
        }
    };

    void getPathsToOneTerminalThroughEdgeOfQmddNode(const dd::mNode& qmddNodeToStartPathsFrom, const QmddNodeEdge edgesToGeneratePathsFor, QmddNodeAndPathsPerEdge& containerStoringFoundPaths) {
        assert(qmddNodeToStartPathsFrom.e.size() == 4);
        for (const QmddNodeEdge availableQmddNodeEdge: {QmddNodeEdge::N, QmddNodeEdge::PPrime, QmddNodeEdge::NPrime, QmddNodeEdge::P}) {
            if (!(availableQmddNodeEdge & edgesToGeneratePathsFor)) {
                continue;
            }

            const auto& firstEdgeInQmddNodePath = qmddNodeToStartPathsFrom.e[convertQmddNodeEdgeEnumValueToArrayIdx(availableQmddNodeEdge)];
            if (firstEdgeInQmddNodePath.isZeroTerminal()) {
                continue;
            }

            if (firstEdgeInQmddNodePath.isOneTerminal()) {
                auto qmddPathToOneTerminal = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = qmddNodeToStartPathsFrom.v, .qmddEdgeToChildNode = availableQmddNodeEdge})});
                if (qmddNodeToStartPathsFrom.v > 0U) {
                    const dd::Qubit firstQubitInOptimizedQmddPathGap = qmddNodeToStartPathsFrom.v - 1U;
                    qmddPathToOneTerminal.emplace_back(QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = firstQubitInOptimizedQmddPathGap, .nConsecutiveQubitInGap = 1 + (firstQubitInOptimizedQmddPathGap > 0 ? firstQubitInOptimizedQmddPathGap - 1U : 0U)}));
                }
                // TODO: Refactor into helper function of anonymous namespace?
                switch (availableQmddNodeEdge) {
                    case QmddNodeEdge::N:
                        containerStoringFoundPaths.nEdgePaths.emplace_back(qmddPathToOneTerminal);
                        break;
                    case QmddNodeEdge::PPrime:
                        containerStoringFoundPaths.pPrimeEdgePaths.emplace_back(qmddPathToOneTerminal);
                        break;
                    case QmddNodeEdge::NPrime:
                        containerStoringFoundPaths.nPrimeEdgePaths.emplace_back(qmddPathToOneTerminal);
                        break;
                    case QmddNodeEdge::P:
                        containerStoringFoundPaths.pEdgePaths.emplace_back(qmddPathToOneTerminal);
                        break;
                    default:
                        // TODO: Throw an exception in exception-free code?
                        break;
                }
                continue;
            }

            std::vector<VisitedQmddNodeEdgesAggregation> toBeVisitedQmddNodesStack;
            toBeVisitedQmddNodesStack.emplace_back(firstEdgeInQmddNodePath.p);
            while (!toBeVisitedQmddNodesStack.empty()) {
                auto& visitedQmddNodeStackEntry = toBeVisitedQmddNodesStack.back();
                if (visitedQmddNodeStackEntry.visitedAllEdges()) {
                    toBeVisitedQmddNodesStack.pop_back();
                    continue;
                }

                const QmddNodeEdge nextQmddNodeEdgeToVisit = visitedQmddNodeStackEntry.advanceToNextEdge();
                const auto&        edgeToChildQmddNode     = visitedQmddNodeStackEntry.associatedQmddNode->e[convertQmddNodeEdgeEnumValueToArrayIdx(nextQmddNodeEdgeToVisit)];
                if (edgeToChildQmddNode.isOneTerminal()) {
                    OptimizedQmddPath qmddPathToOneTerminal;
                    // Note that is reserve operation does not account for gaps in qmdd path
                    qmddPathToOneTerminal.reserve(toBeVisitedQmddNodesStack.size() + 1U);
                    qmddPathToOneTerminal.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = qmddNodeToStartPathsFrom.v, .qmddEdgeToChildNode = availableQmddNodeEdge}));

                    dd::Qubit qubitAssociatedWithLastProcessedQmddNodeStackEntry = qmddNodeToStartPathsFrom.v;
                    for (const auto& toBeVisitedQmddNodesStackEntry: toBeVisitedQmddNodesStack) {
                        const dd::Qubit qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry = toBeVisitedQmddNodesStackEntry.associatedQmddNode->v;
                        const dd::Qubit expectedQubitForNextComponentInQmddPath                 = qubitAssociatedWithLastProcessedQmddNodeStackEntry - 1U;

                        if (expectedQubitForNextComponentInQmddPath != qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry) {
                            // Gap in qmdd path detected due to some node and edges being optimized away by the underlying qmdd data structure due to this "sub-qmdd" representing the identity function for the optimized away qubits.
                            // Since the qmdd transformation algorithm is assumed to require all paths in the qmdd requires us to also record these gaps to correctly calculate the number of qmdd paths through an edge in the qmdd.
                            const std::size_t qmddPathGapSize = expectedQubitForNextComponentInQmddPath - qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry;
                            qmddPathToOneTerminal.emplace_back(QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = expectedQubitForNextComponentInQmddPath, .nConsecutiveQubitInGap = qmddPathGapSize}));
                            qmddPathToOneTerminal.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry, .qmddEdgeToChildNode = nextQmddNodeEdgeToVisit}));
                        } else {
                            qmddPathToOneTerminal.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry, .qmddEdgeToChildNode = *toBeVisitedQmddNodesStackEntry.currVisitedEdge}));
                        }
                        qubitAssociatedWithLastProcessedQmddNodeStackEntry = qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry;
                    }

                    // TODO: Add a comment as to why this is necessary (check wording)
                    // If the edge of the currently processed qmdd node points to a one terminal then we need to "repeat" our previous check for gaps in
                    // the qmdd path that we performed for the intermediate notes in the visited qmdd nodes stack but this time for the qubits that follow
                    // after the one of the currently processed one. If this qubit is not equal to zero then we need to record the gap from the current qubit
                    // up to the one-terminal in our recorded qmdd path
                    if (toBeVisitedQmddNodesStack.back().associatedQmddNode->v != 0) {
                        const dd::Qubit firstQubitOnOptimizedPathToOneTerminal = toBeVisitedQmddNodesStack.back().associatedQmddNode->v - 1U;
                        qmddPathToOneTerminal.emplace_back(QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = firstQubitOnOptimizedPathToOneTerminal,
                                                                        .nConsecutiveQubitInGap                = static_cast<std::size_t>(firstQubitOnOptimizedPathToOneTerminal + 1U)}));
                    }

                    switch (availableQmddNodeEdge) {
                        case QmddNodeEdge::N:
                            containerStoringFoundPaths.nEdgePaths.emplace_back(qmddPathToOneTerminal);
                            break;
                        case QmddNodeEdge::PPrime:
                            containerStoringFoundPaths.pPrimeEdgePaths.emplace_back(qmddPathToOneTerminal);
                            break;
                        case QmddNodeEdge::NPrime:
                            containerStoringFoundPaths.nPrimeEdgePaths.emplace_back(qmddPathToOneTerminal);
                            break;
                        case QmddNodeEdge::P:
                            containerStoringFoundPaths.pEdgePaths.emplace_back(qmddPathToOneTerminal);
                            break;
                        default:
                            // TODO: Throw an exception in exception-free code?
                            break;
                    }
                    // Some of the qmdd transformation algorithm rules need to check the number of paths through an edge with a path assuming to end in a one-terminal.
                    // However, the data structure used to store the qmdd may optimize paths (i.e. sub-qmdds) that already represent the identity function by simply pointing the
                    // edge to the subtree to the one terminal. However, the calculation for the number of paths to the one-terminal (the paths from the root of the qmdd to the last qubit in the qubit ordering of the associated reversible function)
                    // should not accumulate the "optimized" path to the one-terminal representing the sub-qmdd by assuming a single path but should instead count all paths in the sub-qmdd.
                    //qmddPathToOneTerminal.nonTruncatedPathComponents.pop_back();
                    //toBeVisitedQmddNodesStack.pop_back();
                } else if (!edgeToChildQmddNode.isZeroTerminal()) {
                    // TODO: Refactor into helper function of anonymous namespace?
                    const dd::mNode* toBeVisitedNode = edgeToChildQmddNode.p;
                    assert(toBeVisitedNode->e.size() == 4);
                    toBeVisitedQmddNodesStack.emplace_back(toBeVisitedNode);
                }
            }
        }
    }

    std::vector<UnoptimizedQmddPath> getAllPathsFromRootToNode(const dd::mNode& qmddRootNode, const dd::mNode& qmddNodeToReach) {
        if (qmddRootNode.v == qmddNodeToReach.v) {
            return {};
        }

        assert(qmddRootNode.e.size() == 4);
        std::vector<UnoptimizedQmddPath> containerForFoundQmddpaths;
        UnoptimizedQmddPath              currQmddPathToOneTerminal;

        std::stack<VisitedQmddNodeEdgesAggregation> toBeVisitedQmddNodesStack;
        toBeVisitedQmddNodesStack.emplace(&qmddRootNode);
        while (!toBeVisitedQmddNodesStack.empty()) {
            auto& visitedQmddNode = toBeVisitedQmddNodesStack.top();
            if (visitedQmddNode.visitedAllEdges()) {
                toBeVisitedQmddNodesStack.pop();
                if (!currQmddPathToOneTerminal.empty()) {
                    currQmddPathToOneTerminal.pop_back();
                }
                continue;
            }

            // TODO: The searched for node should be reachable by only traversing the N or P edges.
            // TODO: At the moment all possible edges of a node are checked.
            const std::optional<QmddNodeEdge> nextQmddNodeEdgeToVisit = visitedQmddNode.advanceToNextEdgeInPathFromRootToNode();
            if (!nextQmddNodeEdgeToVisit.has_value()) {
                return {};
            }
            const dd::mEdge& edgeToChildQmddNode = visitedQmddNode.associatedQmddNode->e[convertQmddNodeEdgeEnumValueToArrayIdx(*nextQmddNodeEdgeToVisit)];
            const dd::mNode* toBeVisitedNode     = edgeToChildQmddNode.p;
            if (edgeToChildQmddNode.isTerminal() || toBeVisitedNode == nullptr || toBeVisitedNode->v < qmddNodeToReach.v) {
                if (!edgeToChildQmddNode.isTerminal()) {
                    visitedQmddNode.markAllEdgesAsVisited();
                }
                continue;
            }

            if (toBeVisitedNode->v == qmddNodeToReach.v) {
                // TODO:
                // We need to check whether the qmdd node associated with the same qubit is actually the node that we are looking for. Otherwise, the search did not take the "correct"
                // edge of the root qmdd node and we need to continue our search in the parent qmdd node.
                if (edgeToChildQmddNode.p == &qmddNodeToReach) {
                    currQmddPathToOneTerminal.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = visitedQmddNode.associatedQmddNode->v, .qmddEdgeToChildNode = *nextQmddNodeEdgeToVisit}));
                    containerForFoundQmddpaths.emplace_back(currQmddPathToOneTerminal);
                }
            } else {
                assert(toBeVisitedNode->e.size() == 4);
                // We are still processing a parent qmdd node thus we advance one level down in the qmdd
                toBeVisitedQmddNodesStack.emplace(toBeVisitedNode);
                currQmddPathToOneTerminal.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = visitedQmddNode.associatedQmddNode->v, .qmddEdgeToChildNode = *nextQmddNodeEdgeToVisit}));
            }
        }
        return containerForFoundQmddpaths;
    }
} // namespace syrec
