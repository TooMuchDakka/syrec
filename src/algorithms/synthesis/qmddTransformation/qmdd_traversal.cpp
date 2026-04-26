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

#include "algorithms/synthesis/qmddTransformation/qmdd_path_generator.hpp"
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
                visitedEdges    = QmddNodeEdge::N;
                currVisitedEdge = QmddNodeEdge::N;
            } else if (*visitedEdges & QmddNodeEdge::N) {
                markAllEdgesAsVisited();
                currVisitedEdge = QmddNodeEdge::P;
            } else {
                currVisitedEdge.reset();
            }
            return currVisitedEdge;
        }

        void markAllEdgesAsVisited() noexcept {
            visitedEdges = QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P;
        }
    };

    void generateQmddPath(OptimizedQmddPath& qmddPathContainer, const dd::Qubit qubitAtRootOfGeneratedQmddPath, const std::vector<VisitedQmddNodeEdgesAggregation>& visitedQmddNodesStack) {
        dd::Qubit qubitAssociatedWithLastProcessedQmddNodeStackEntry = qubitAtRootOfGeneratedQmddPath;
        if (visitedQmddNodesStack.empty()) {
            return;
        }

        const dd::mNode* firstVisitedQmddNodeInStack = visitedQmddNodesStack.front().associatedQmddNode;
        assert(firstVisitedQmddNodeInStack != nullptr);

        auto visitedQmddNodesStackIterator = visitedQmddNodesStack.begin();
        if (dd::mNode::isTerminal(firstVisitedQmddNodeInStack)) {
            const dd::Qubit qubitAssociatedWithFirstVisitedQmddNode = firstVisitedQmddNodeInStack->v;
            dd::Qubit       expectedQubitForNextComponentInQmddPath = 0U;
            if (visitedQmddNodesStack.size() > 1U) {
                assert(visitedQmddNodesStack.at(1U).associatedQmddNode != nullptr);
                expectedQubitForNextComponentInQmddPath = visitedQmddNodesStack.at(1U).associatedQmddNode->v;
            }
            const std::size_t qmddPathGapSize = (qubitAssociatedWithFirstVisitedQmddNode - expectedQubitForNextComponentInQmddPath) + 1U;
            qmddPathContainer.emplace_back(QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = qubitAssociatedWithFirstVisitedQmddNode, .nConsecutiveQubitInGap = qmddPathGapSize}));
            qubitAssociatedWithLastProcessedQmddNodeStackEntry = (qubitAssociatedWithFirstVisitedQmddNode - static_cast<dd::Qubit>(qmddPathGapSize)) + 1U;
            ++visitedQmddNodesStackIterator;
        }

        while (visitedQmddNodesStackIterator != visitedQmddNodesStack.end()) {
            const auto&     toBeVisitedQmddNodesStackEntry                          = *visitedQmddNodesStackIterator;
            const dd::Qubit qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry = toBeVisitedQmddNodesStackEntry.associatedQmddNode->v;
            const dd::Qubit expectedQubitForNextComponentInQmddPath                 = qubitAssociatedWithLastProcessedQmddNodeStackEntry - 1U;

            if (expectedQubitForNextComponentInQmddPath != qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry) {
                // Gap in qmdd path detected due to some node and edges being optimized away by the underlying qmdd data structure due to this "sub-qmdd" representing the identity function for the optimized away qubits.
                // Since the qmdd transformation algorithm is assumed to require all paths in the qmdd requires us to also record these gaps to correctly calculate the number of qmdd paths through an edge in the qmdd.
                const std::size_t qmddPathGapSize = expectedQubitForNextComponentInQmddPath - qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry;
                qmddPathContainer.emplace_back(QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = expectedQubitForNextComponentInQmddPath, .nConsecutiveQubitInGap = qmddPathGapSize}));
                qubitAssociatedWithLastProcessedQmddNodeStackEntry = (expectedQubitForNextComponentInQmddPath - static_cast<dd::Qubit>(qmddPathGapSize)) + 1U;
                continue;
            }

            qmddPathContainer.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry, .qmddEdgeToChildNode = *toBeVisitedQmddNodesStackEntry.currVisitedEdge}));
            if (toBeVisitedQmddNodesStackEntry.associatedQmddNode->e[convertQmddNodeEdgeEnumValueToArrayIdx(*toBeVisitedQmddNodesStackEntry.currVisitedEdge)].isOneTerminal() && qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry != 0U) {
                const dd::Qubit qubitAssociatedWithSuccessorOfQmddNodeEdgeToOneTerminal = qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry - 1U;
                const auto      qmddPathGapSize                                         = static_cast<std::size_t>(qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry);
                qmddPathContainer.emplace_back(QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = qubitAssociatedWithSuccessorOfQmddNodeEdgeToOneTerminal, .nConsecutiveQubitInGap = qmddPathGapSize}));
                return;
            }
            qubitAssociatedWithLastProcessedQmddNodeStackEntry = qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry;
            ++visitedQmddNodesStackIterator;
        }
    }

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
                    const dd::Qubit   firstQubitInOptimizedQmddPathGap = qmddNodeToStartPathsFrom.v - 1U;
                    const std::size_t qmddPathGapSize                  = firstQubitInOptimizedQmddPathGap + 1U;
                    qmddPathToOneTerminal.emplace_back(QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = firstQubitInOptimizedQmddPathGap, .nConsecutiveQubitInGap = qmddPathGapSize}));
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
                    generateQmddPath(qmddPathToOneTerminal, qmddNodeToStartPathsFrom.v, toBeVisitedQmddNodesStack);

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

        std::vector<VisitedQmddNodeEdgesAggregation> toBeVisitedQmddNodesStack;
        toBeVisitedQmddNodesStack.emplace_back(&qmddRootNode);
        while (!toBeVisitedQmddNodesStack.empty()) {
            auto& visitedQmddNode = toBeVisitedQmddNodesStack.back();
            if (visitedQmddNode.visitedAllEdges()) {
                toBeVisitedQmddNodesStack.pop_back();
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
            if (edgeToChildQmddNode.isTerminal() || toBeVisitedNode == nullptr) {
                continue;
            }
            if (toBeVisitedNode->v < qmddNodeToReach.v) {
                visitedQmddNode.markAllEdgesAsVisited();
                continue;
            }

            if (toBeVisitedNode->v == qmddNodeToReach.v) {
                // TODO:
                // We need to check whether the qmdd node associated with the same qubit is actually the node that we are looking for. Otherwise, the search did not take the "correct"
                // edge of the root qmdd node and we need to continue our search in the parent qmdd node.
                if (edgeToChildQmddNode.p == &qmddNodeToReach) {
                    const std::size_t nQubitsBetweenParentAndNodeToReach  = (visitedQmddNode.associatedQmddNode->v - qmddNodeToReach.v) - 1U;
                    const bool        existGapBetweenParentAndNodeToReach = nQubitsBetweenParentAndNodeToReach > 0;

                    OptimizedQmddPath optimizedQmddPathToParentOfNodeToReach;
                    optimizedQmddPathToParentOfNodeToReach.reserve(toBeVisitedQmddNodesStack.size() + static_cast<std::size_t>(existGapBetweenParentAndNodeToReach));
                    generateQmddPath(optimizedQmddPathToParentOfNodeToReach, qmddRootNode.v + 1U, toBeVisitedQmddNodesStack);
                    assert(!optimizedQmddPathToParentOfNodeToReach.empty());

                    if (existGapBetweenParentAndNodeToReach) {
                        optimizedQmddPathToParentOfNodeToReach.emplace_back(QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = static_cast<dd::Qubit>(visitedQmddNode.associatedQmddNode->v - 1U), .nConsecutiveQubitInGap = nQubitsBetweenParentAndNodeToReach}));
                    }

                    auto unoptimizedQmddPathGenerator = QmddPathGenerator(optimizedQmddPathToParentOfNodeToReach);
                    assert(unoptimizedQmddPathGenerator.canGenerateCombinations());

                    for (const UnoptimizedQmddPath* generatedQmddPath = unoptimizedQmddPathGenerator.tryGenerateNextPath(); generatedQmddPath != nullptr; generatedQmddPath = unoptimizedQmddPathGenerator.tryGenerateNextPath()) {
                        containerForFoundQmddpaths.emplace_back(*generatedQmddPath);
                    }
                }
            } else {
                assert(toBeVisitedNode->e.size() == 4);
                // We are still processing a parent qmdd node thus we advance one level down in the qmdd
                toBeVisitedQmddNodesStack.emplace_back(toBeVisitedNode);
            }
        }
        return containerForFoundQmddpaths;
    }
} // namespace syrec
