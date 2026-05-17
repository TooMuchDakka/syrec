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
#include "algorithms/synthesis/qmddTransformation/qmdd_path_operations.hpp"
#include "dd/DDDefinitions.hpp"
#include "dd/Node.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <optional>
#include <vector>

namespace {
    struct VisitedQmddNodeEdgesAggregation {
        const dd::mNode*                   associatedQmddNode;
        std::optional<syrec::QmddNodeEdge> visitedEdges;
        std::optional<syrec::QmddNodeEdge> currVisitedEdge;

        explicit VisitedQmddNodeEdgesAggregation(const dd::mNode* associatedQmddNode):
            associatedQmddNode(associatedQmddNode) {}

        [[nodiscard]] constexpr bool visitedAllEdges() const noexcept {
            return visitedEdges.has_value() && (*visitedEdges & syrec::QmddNodeEdge::N && *visitedEdges & syrec::QmddNodeEdge::PPrime && *visitedEdges & syrec::QmddNodeEdge::NPrime && *visitedEdges & syrec::QmddNodeEdge::P);
        }

        [[maybe_unused]] syrec::QmddNodeEdge advanceToNextEdge() noexcept {
            if (!visitedEdges.has_value()) {
                currVisitedEdge = syrec::QmddNodeEdge::N;
                visitedEdges    = syrec::QmddNodeEdge::N;
                return *currVisitedEdge;
            }
            bool                 advancedToNextEdge = false;
            constexpr std::array qmddNodeEdges{syrec::QmddNodeEdge::P, syrec::QmddNodeEdge::NPrime, syrec::QmddNodeEdge::PPrime, syrec::QmddNodeEdge::N};
            for (std::size_t i = 0; i < 4 && !advancedToNextEdge; ++i) {
                if (const syrec::QmddNodeEdge qmddNodeEdge = qmddNodeEdges[i]; *visitedEdges & qmddNodeEdge) {
                    switch (qmddNodeEdge) {
                        case syrec::QmddNodeEdge::N:
                            currVisitedEdge    = syrec::QmddNodeEdge::PPrime;
                            advancedToNextEdge = true;
                            break;
                        case syrec::QmddNodeEdge::PPrime:
                            currVisitedEdge    = syrec::QmddNodeEdge::NPrime;
                            advancedToNextEdge = true;
                            break;
                        case syrec::QmddNodeEdge::NPrime:
                            currVisitedEdge    = syrec::QmddNodeEdge::P;
                            advancedToNextEdge = true;
                            break;
                        case syrec::QmddNodeEdge::P:
                            currVisitedEdge    = syrec::QmddNodeEdge::N;
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

        [[maybe_unused]] std::optional<syrec::QmddNodeEdge> advanceToNextEdgeInPathFromRootToNode() noexcept {
            if (!visitedEdges.has_value()) {
                visitedEdges    = syrec::QmddNodeEdge::N;
                currVisitedEdge = syrec::QmddNodeEdge::N;
            } else if (*visitedEdges & syrec::QmddNodeEdge::N) {
                markAllEdgesAsVisited();
                currVisitedEdge = syrec::QmddNodeEdge::P;
            } else {
                currVisitedEdge.reset();
            }
            return currVisitedEdge;
        }

        void markAllEdgesAsVisited() noexcept {
            visitedEdges = syrec::QmddNodeEdge::N | syrec::QmddNodeEdge::PPrime | syrec::QmddNodeEdge::NPrime | syrec::QmddNodeEdge::P;
        }
    };

    void generateQmddPath(syrec::OptimizedQmddPath& qmddPathContainer, const dd::Qubit qubitAtRootOfGeneratedQmddPath, const std::vector<VisitedQmddNodeEdgesAggregation>& visitedQmddNodesStack) {
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
            qmddPathContainer.emplace_back(syrec::QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = qubitAssociatedWithFirstVisitedQmddNode, .nConsecutiveQubitInGap = qmddPathGapSize}));
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
                qmddPathContainer.emplace_back(syrec::QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = expectedQubitForNextComponentInQmddPath, .nConsecutiveQubitInGap = qmddPathGapSize}));
                qubitAssociatedWithLastProcessedQmddNodeStackEntry = (expectedQubitForNextComponentInQmddPath - static_cast<dd::Qubit>(qmddPathGapSize)) + 1U;
                continue;
            }

            qmddPathContainer.emplace_back(syrec::QmddPathComponent({.qubitAssociatedWithQmddNode = qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry, .qmddEdgeToChildNode = *toBeVisitedQmddNodesStackEntry.currVisitedEdge}));
            if (toBeVisitedQmddNodesStackEntry.associatedQmddNode->e[syrec::convertQmddNodeEdgeEnumValueToArrayIdx(*toBeVisitedQmddNodesStackEntry.currVisitedEdge)].isOneTerminal() && qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry != 0U) {
                const dd::Qubit qubitAssociatedWithSuccessorOfQmddNodeEdgeToOneTerminal = qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry - 1U;
                const auto      qmddPathGapSize                                         = static_cast<std::size_t>(qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry);
                qmddPathContainer.emplace_back(syrec::QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = qubitAssociatedWithSuccessorOfQmddNodeEdgeToOneTerminal, .nConsecutiveQubitInGap = qmddPathGapSize}));
                return;
            }
            qubitAssociatedWithLastProcessedQmddNodeStackEntry = qubitAssociatedWithCurrentlyProcessedQmddNodeStackEntry;
            ++visitedQmddNodesStackIterator;
        }
    }
} // namespace

namespace syrec {
    const std::vector<OptimizedQmddPath>& QmddNodeAndPathsPerEdge::operator[](const QmddNodeEdge qmddNodeEdge) const {
        return pathsPerEdgeLookup.at(convertQmddNodeEdgeEnumValueToArrayIdx(qmddNodeEdge));
    }

    std::vector<OptimizedQmddPath>& QmddNodeAndPathsPerEdge::operator[](const QmddNodeEdge qmddNodeEdge) {
        return pathsPerEdgeLookup.at(convertQmddNodeEdgeEnumValueToArrayIdx(qmddNodeEdge));
    }

    std::size_t NPathsToOneTerminalPerEdgeOfQmddNode::operator[](const QmddNodeEdge qmddNodeEdge) const {
        return nPathsPerEdgeLookup.at(convertQmddNodeEdgeEnumValueToArrayIdx(qmddNodeEdge));
    }

    std::size_t& NPathsToOneTerminalPerEdgeOfQmddNode::operator[](const QmddNodeEdge qmddNodeEdge) {
        return nPathsPerEdgeLookup.at(convertQmddNodeEdgeEnumValueToArrayIdx(qmddNodeEdge));
    }

    NPathsToOneTerminalPerEdgeOfQmddNode getNPathsToOneTerminalPerEdgeOfQmddNode(const dd::mNode& qmddNode) {
        assert(qmddNode.e.size() == 4);

        NPathsToOneTerminalPerEdgeOfQmddNode nPathsPerEdgeOfQmddNode(qmddNode);
        for (const QmddNodeEdge availableQmddNodeEdge: {QmddNodeEdge::N, QmddNodeEdge::PPrime, QmddNodeEdge::NPrime, QmddNodeEdge::P}) {
            const auto& firstEdgeInQmddNodePath = qmddNode.e[convertQmddNodeEdgeEnumValueToArrayIdx(availableQmddNodeEdge)];
            if (firstEdgeInQmddNodePath.isZeroTerminal()) {
                continue;
            }

            if (firstEdgeInQmddNodePath.isOneTerminal()) {
                auto qmddPathToOneTerminal = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = qmddNode.v, .qmddEdgeToChildNode = availableQmddNodeEdge})});
                if (qmddNode.v > 0U) {
                    const dd::Qubit   firstQubitInOptimizedQmddPathGap = qmddNode.v - 1U;
                    const std::size_t qmddPathGapSize                  = firstQubitInOptimizedQmddPathGap + 1U;
                    qmddPathToOneTerminal.emplace_back(QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = firstQubitInOptimizedQmddPathGap, .nConsecutiveQubitInGap = qmddPathGapSize}));
                }
                nPathsPerEdgeOfQmddNode[availableQmddNodeEdge] = getNumberOfPathsToOneTerminalForQmddPath(qmddPathToOneTerminal);
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
                    qmddPathToOneTerminal.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = qmddNode.v, .qmddEdgeToChildNode = availableQmddNodeEdge}));
                    generateQmddPath(qmddPathToOneTerminal, qmddNode.v, toBeVisitedQmddNodesStack);

                    nPathsPerEdgeOfQmddNode[availableQmddNodeEdge] += getNumberOfPathsToOneTerminalForQmddPath(qmddPathToOneTerminal);
                } else if (!edgeToChildQmddNode.isZeroTerminal()) {
                    // TODO: Refactor into helper function of anonymous namespace?
                    const dd::mNode* toBeVisitedNode = edgeToChildQmddNode.p;
                    assert(toBeVisitedNode->e.size() == 4);
                    toBeVisitedQmddNodesStack.emplace_back(toBeVisitedNode);
                }
            }
        }
        return nPathsPerEdgeOfQmddNode;
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
                containerStoringFoundPaths[availableQmddNodeEdge].emplace_back(qmddPathToOneTerminal);
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

                    containerStoringFoundPaths[availableQmddNodeEdge].emplace_back(qmddPathToOneTerminal);
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
                        // TODO: Only for faster testing but might also be the expected behaviour of this function in the future.
                        return containerForFoundQmddpaths;
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
