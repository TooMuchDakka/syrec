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

#include "dd/Node.hpp"
#include "dd/Package.hpp"
#include "ir/QuantumComputation.hpp"
#include "ir/operations/Control.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace syrec {
    class QmddTransformer {
    public:
        struct QmddTransformationStatistic {
            long long transformationRuntimeInMilliseconds;
        };

        struct QmddDumpConfig {
            std::string pathToDumpFile;
        };

        explicit QmddTransformer(const std::reference_wrapper<qc::QuantumComputation> quantumComputation, const std::reference_wrapper<dd::Package> qmddPkg):
            qc(quantumComputation), qmddPkg(qmddPkg) {}

        [[nodiscard]] bool             synthesizeQmdd(dd::mEdge edgeToQmddRoot, QmddTransformationStatistic* optionalTransformationStatistics = nullptr, const std::optional<QmddDumpConfig>& optionalQmddDumpConfig = std::nullopt);
        [[nodiscard]] static dd::mEdge constructQmddFromGatesOfQuantumComputation(const qc::QuantumComputation& quantumComputation, dd::Package& qmddPackage, const std::optional<QmddDumpConfig>& optionalQmddDumpConfig = std::nullopt);

    protected:
        enum class QmddNodeEdge : std::uint8_t {
            N       = 1,
            P_Prime = 2,
            N_Prime = 4,
            P       = 8
        };

        struct QmddPathComponent {
            dd::Qubit    qubitAssociatedWithQmddNode = 0U;
            QmddNodeEdge qmddEdgeToChildNode         = QmddNodeEdge::N;
        };

        struct QmddPath {
            std::vector<QmddPathComponent> nonTruncatedPathComponents;
            std::optional<dd::Qubit>       firstQubitOfTruncatedPathToOneTerminal;
        };

        struct QmddNodeAndPathsPerEdge {
            std::reference_wrapper<const dd::mNode> associatedQmddNode;
            std::vector<QmddPath>                   nEdgePaths;
            std::vector<QmddPath>                   pPrimeEdgePaths;
            std::vector<QmddPath>                   nPrimeEdgePaths;
            std::vector<QmddPath>                   pEdgePaths;
        };

        struct VisitedQmddNodeEdgesAggregation {
            const dd::mNode*            associatedQmddNode;
            std::optional<QmddNodeEdge> visitedEdges;
            std::optional<QmddNodeEdge> currVisitedEdge;

            explicit VisitedQmddNodeEdgesAggregation(const dd::mNode* associatedQmddNode): associatedQmddNode(associatedQmddNode) {}

            [[nodiscard]] constexpr bool visitedAllEdges() const noexcept {
                return visitedEdges.has_value() && (*visitedEdges & QmddNodeEdge::N && *visitedEdges & QmddNodeEdge::P_Prime && *visitedEdges & QmddNodeEdge::N_Prime && *visitedEdges & QmddNodeEdge::P);
            }

            [[maybe_unused]] QmddNodeEdge advanceToNextEdge() noexcept {
                if (!visitedEdges.has_value()) {
                    currVisitedEdge = QmddNodeEdge::N;
                    visitedEdges    = QmddNodeEdge::N;
                    return *currVisitedEdge;
                }
                bool                   advancedToNextEdge = false;
                constexpr QmddNodeEdge qmddNodeEdges[4]   = {QmddNodeEdge::P, QmddNodeEdge::N_Prime, QmddNodeEdge::P_Prime, QmddNodeEdge::N};
                for (std::size_t i = 0; i < 4 && !advancedToNextEdge; ++i) {
                    if (const QmddNodeEdge qmddNodeEdge = qmddNodeEdges[i]; *visitedEdges & qmddNodeEdge) {
                        switch (qmddNodeEdge) {
                            case QmddNodeEdge::N:
                                currVisitedEdge    = QmddNodeEdge::P_Prime;
                                advancedToNextEdge = true;
                                break;
                            case QmddNodeEdge::P_Prime:
                                currVisitedEdge    = QmddNodeEdge::N_Prime;
                                advancedToNextEdge = true;
                                break;
                            case QmddNodeEdge::N_Prime:
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
                    *visitedEdges = QmddNodeEdge::N | QmddNodeEdge::P_Prime | QmddNodeEdge::N_Prime | QmddNodeEdge::P;
                    return QmddNodeEdge::P;
                }
                return std::nullopt;
            }

            void markAllEdgesAsVisited() noexcept {
                visitedEdges = QmddNodeEdge::N | QmddNodeEdge::P_Prime | QmddNodeEdge::N_Prime | QmddNodeEdge::P;
            }
        };

        friend constexpr bool operator&(const QmddNodeEdge lQmddNodeEdge, const QmddNodeEdge rQmddNodeEdge) noexcept {
            return (static_cast<std::underlying_type_t<QmddNodeEdge>>(lQmddNodeEdge) & static_cast<std::underlying_type_t<QmddNodeEdge>>(rQmddNodeEdge)) > 0;
        }

        friend constexpr QmddNodeEdge operator|(const QmddNodeEdge lQmddNodeEdge, const QmddNodeEdge rQmddNodeEdge) noexcept {
            return static_cast<QmddNodeEdge>(static_cast<std::underlying_type_t<QmddNodeEdge>>(lQmddNodeEdge) | static_cast<std::underlying_type_t<QmddNodeEdge>>(rQmddNodeEdge));
        }

        friend constexpr void operator|=(QmddNodeEdge& lQmddNodeEdge, const QmddNodeEdge rQmddNodeEdge) noexcept {
            lQmddNodeEdge = lQmddNodeEdge | rQmddNodeEdge;
        }

        [[nodiscard]] static constexpr std::size_t convertQmddNodeEdgeEnumValueToArrayIdx(const QmddNodeEdge qmddNodeEdge) {
            switch (qmddNodeEdge) {
                case QmddNodeEdge::N:
                    return 0U;
                case QmddNodeEdge::P_Prime:
                    return 1U;
                case QmddNodeEdge::N_Prime:
                    return 2U;
                default:
                    return 3;
            }
        }

        [[maybe_unused]] dd::mEdge applyMCXGateToQmdd(const dd::mEdge& edgeToRootNodeOfQmdd, qc::Qubit targetQubit, const qc::Controls& controlQubits) const;
        [[nodiscard]] bool         trySwapPathsOfEdgesOfQmddNode(QmddNodeAndPathsPerEdge& qmddNodeAndEdgePaths) const;
        [[nodiscard]] bool         tryShiftUniquePathsOfQmddNode(QmddNodeAndPathsPerEdge& qmddNodeAndEdgePaths) const;
        [[nodiscard]] bool         tryMakeSharedPathOfQmddNodeUnique(QmddNodeAndPathsPerEdge& qmddNodeAndEdgePaths) const;

        [[nodiscard]] static const dd::mEdge* tryGetEdgeToQmddRootNode(dd::Package& qmddPkgToGetRootFrom);
        [[nodiscard]] static bool             terminate(const dd::mNode& nodeToCheck);

        enum class QmddExportOutputStreamOperation : std::uint8_t {
            OverwriteExisting,
            Append
        };
        static void exportQmddToFile(const dd::mEdge* edgeToRootNodeOfQmdd, const std::optional<QmddDumpConfig>& optionalQmddDumpConfig = std::nullopt, QmddExportOutputStreamOperation qmddExportOutputStreamOperation = QmddExportOutputStreamOperation::Append);
        static void getPathsToOneTerminalThroughEdgeOfQmddNode(const dd::mNode& qmddNodeToStartPathsFrom, QmddNodeEdge edgesToGeneratePathsFor, QmddNodeAndPathsPerEdge& containerStoringFoundPaths);
        // TODO: A node should only be reachable from the root node by traversing either the P or N edge of the traversed node until the searched for node is found.
        // TODO: "Truncated" nodes from the "original" qmdd root to the current qmdd root should be ignorable?
        [[nodiscard]] static std::vector<QmddPath>       getAllPathsFromRootToNode(const dd::mNode& qmddRootNode, const dd::mNode& qmddNodeToReach);
        [[nodiscard]] static qc::Controls                getControlQubitsFromSignatureOfQmddPathComponents(const std::vector<QmddPathComponent>& qmddPathComponents) noexcept;
        [[nodiscard]] static qc::Control                 getControlQubitForQmddPathComponent(QmddPathComponent qmddPathComponent);
        [[nodiscard]] static constexpr qc::Control::Type getControlQubitTypeForQmddNodeEdge(const QmddNodeEdge qmddNodeEdge) noexcept {
            return qmddNodeEdge == QmddNodeEdge::P || qmddNodeEdge == QmddNodeEdge::P_Prime ? qc::Control::Type::Pos : qc::Control::Type::Neg;
        }
        [[nodiscard]] static std::optional<QmddPath> getFirstQmddPathWithUniqueSignature(const std::vector<QmddPath>& qmddPathsToSearchForUniqueOne, const std::vector<QmddPath>& qmddPathsDefiningComparedToSignatures);

        struct TransformationToUniqueQmddPathData {
            qc::Controls controlQubitsFromFirstNodeInPathToTargetQubit;
            qc::Qubit    targetQubit;
        };
        [[nodiscard]] static std::optional<TransformationToUniqueQmddPathData> getTransformationDataToMakeAnyQmddPathUniqueViaSingleSignatureBitFlip(const std::vector<QmddPath>& qmddPathsContainingPotentiallyTransformableOne, const std::vector<QmddPath>& comparedToQmddPaths);
        [[nodiscard]] static std::optional<TransformationToUniqueQmddPathData> getTransformationDataToMakeQmddPathUniqueViaSingleSignatureBitFlip(const QmddPath& qmddPathToTurnUnique, const std::vector<QmddPath>& comparedToQmddPaths);
        [[nodiscard]] static std::size_t                                       getNumberOfPathsToOneTerminalForQmddPath(const QmddPath& qmddPath) noexcept;
        [[nodiscard]] static std::size_t                                       getNumberOfPathsToOneTerminalForQmddPaths(const std::vector<QmddPath>& qmddPaths) noexcept;

        std::reference_wrapper<qc::QuantumComputation> qc;
        std::reference_wrapper<dd::Package>            qmddPkg;

        struct QmddPathGenerator {
            std::vector<QmddPathComponent> lastGeneratedCombination;
            bool                           isGeneratingMoreThanOneCombination;
            bool                           hasGeneratedFirstCombination;
            dd::Qubit                      firstNonTruncatedQubit;

            explicit QmddPathGenerator(const QmddPath& qmddPath) {
                if (qmddPath.nonTruncatedPathComponents.empty() && !qmddPath.firstQubitOfTruncatedPathToOneTerminal.has_value()) {
                    isGeneratingMoreThanOneCombination = false;
                    hasGeneratedFirstCombination       = true;
                    firstNonTruncatedQubit             = 0U;
                    return;
                }

                isGeneratingMoreThanOneCombination = qmddPath.firstQubitOfTruncatedPathToOneTerminal.has_value();
                hasGeneratedFirstCombination       = false;
                firstNonTruncatedQubit             = qmddPath.nonTruncatedPathComponents.back().qubitAssociatedWithQmddNode;

                lastGeneratedCombination = qmddPath.nonTruncatedPathComponents;
                if (isGeneratingMoreThanOneCombination) {
                    const std::size_t nQubitsOptimizedInTruncatedPathComponent = qmddPath.firstQubitOfTruncatedPathToOneTerminal.value() + 1U;
                    lastGeneratedCombination.reserve(qmddPath.nonTruncatedPathComponents.size() + nQubitsOptimizedInTruncatedPathComponent);
                    for (dd::Qubit i = 0U; i < nQubitsOptimizedInTruncatedPathComponent; ++i) {
                        // Qubits in qmdd path are expected to be defined in the same order as the variable ordering of the associated qmdd which in turn defines the variable ordering as starting with the "largest" qubit down to the "lowest" qubit.
                        lastGeneratedCombination.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = static_cast<dd::Qubit>(*qmddPath.firstQubitOfTruncatedPathToOneTerminal - i), .qmddEdgeToChildNode = QmddNodeEdge::N}));
                    }
                    lastGeneratedCombination.back().qmddEdgeToChildNode = QmddNodeEdge::P;
                }
            }

            [[nodiscard]] bool generateNextCombination() {
                if (!isGeneratingMoreThanOneCombination) {
                    if (hasGeneratedFirstCombination) {
                        return false;
                    }
                    hasGeneratedFirstCombination = true;
                    return true;
                }

                bool advanceModificationToNextPosition = true;
                for (auto lastGeneratedCombinationIterator = lastGeneratedCombination.rbegin(); lastGeneratedCombinationIterator != lastGeneratedCombination.rend() && advanceModificationToNextPosition; ++lastGeneratedCombinationIterator) {
                    if (lastGeneratedCombinationIterator->qubitAssociatedWithQmddNode == firstNonTruncatedQubit) {
                        return false;
                    }
                    lastGeneratedCombinationIterator->qmddEdgeToChildNode = lastGeneratedCombinationIterator->qmddEdgeToChildNode == QmddNodeEdge::N ? QmddNodeEdge::P : QmddNodeEdge::N;
                    advanceModificationToNextPosition                     = hasGeneratedFirstCombination ? lastGeneratedCombinationIterator->qmddEdgeToChildNode == QmddNodeEdge::N : false;
                    hasGeneratedFirstCombination                          = true;
                }
                return !advanceModificationToNextPosition;
            }
        };
    };
} // namespace syrec
