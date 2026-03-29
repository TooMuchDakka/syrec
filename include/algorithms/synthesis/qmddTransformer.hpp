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
#include "dd/Package.hpp"
#include "ir/Definitions.hpp"
#include "ir/QuantumComputation.hpp"
#include "ir/operations/Control.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace syrec {
    class QmddTransformer {
    public:
        struct QmddTransformationStatistic {
            std::uint64_t transformationRuntimeInMilliseconds;
        };

        struct QmddDumpConfig {
            std::string pathToDumpFile;
        };

        explicit QmddTransformer(const std::reference_wrapper<qc::QuantumComputation> quantumComputation, const std::reference_wrapper<dd::Package> qmddPkg):
            qc(quantumComputation), qmddPkg(qmddPkg) {}

        [[nodiscard]] bool             synthesizeQmdd(dd::mEdge edgeToQmddRoot, QmddTransformationStatistic* optionalTransformationStatistics = nullptr, const std::optional<QmddDumpConfig>& optionalQmddDumpConfig = std::nullopt);
        [[nodiscard]] static dd::mEdge constructQmddFromGatesOfQuantumComputation(const qc::QuantumComputation& quantumComputation, dd::Package& qmddPackage, const std::optional<QmddDumpConfig>& optionalQmddDumpConfig = std::nullopt);

    protected:
        struct QmddNodeAndPathsPerEdge {
            std::reference_wrapper<const dd::mNode> associatedQmddNode;
            std::vector<OptimizedQmddPath>          nEdgePaths;
            std::vector<OptimizedQmddPath>          pPrimeEdgePaths;
            std::vector<OptimizedQmddPath>          nPrimeEdgePaths;
            std::vector<OptimizedQmddPath>          pEdgePaths;
        };

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
        [[nodiscard]] static std::vector<UnoptimizedQmddPath> getAllPathsFromRootToNode(const dd::mNode& qmddRootNode, const dd::mNode& qmddNodeToReach);
        [[nodiscard]] static qc::Controls                     getControlQubitsFromSignatureOfQmddPathComponents(const std::vector<QmddPathComponent>& qmddPathComponents) noexcept;

        [[nodiscard]] static std::optional<UnoptimizedQmddPath> getFirstQmddPathWithUniqueSignature(const std::vector<OptimizedQmddPath>& qmddPathsToSearchForUniqueOne, const std::vector<OptimizedQmddPath>& qmddPathsDefiningComparedToSignatures);

        struct TransformationToUniqueQmddPathData {
            qc::Controls controlQubitsFromFirstNodeInPathToTargetQubit;
            qc::Qubit    targetQubit;
        };
        [[nodiscard]] static std::optional<TransformationToUniqueQmddPathData> getTransformationDataToMakeAnyQmddPathUniqueViaSingleSignatureBitFlip(const std::vector<OptimizedQmddPath>& qmddPathsContainingPotentiallyTransformableOne, const std::vector<OptimizedQmddPath>& comparedToQmddPaths);
        [[nodiscard]] static std::optional<TransformationToUniqueQmddPathData> getTransformationDataToMakeQmddPathUniqueViaSingleSignatureBitFlip(const OptimizedQmddPath& qmddPathToTurnUnique, const std::vector<OptimizedQmddPath>& comparedToQmddPaths);
        [[nodiscard]] static std::size_t                                       getNumberOfPathsToOneTerminalForQmddPath(const OptimizedQmddPath& qmddPath) noexcept;
        [[nodiscard]] static std::size_t                                       getNumberOfPathsToOneTerminalForQmddPaths(const std::vector<OptimizedQmddPath>& qmddPaths) noexcept;

        std::reference_wrapper<qc::QuantumComputation> qc;
        std::reference_wrapper<dd::Package>            qmddPkg;
    };
} // namespace syrec
