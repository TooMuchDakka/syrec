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
        using QmddPath = std::vector<QmddPathComponent>;

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

            explicit VisitedQmddNodeEdgesAggregation(const dd::mNode* associatedQmddNode): associatedQmddNode(associatedQmddNode) {}

            [[nodiscard]] constexpr bool visitedAllEdges() const noexcept {
                return visitedEdges.has_value() && (*visitedEdges & QmddNodeEdge::N && *visitedEdges & QmddNodeEdge::P_Prime && *visitedEdges & QmddNodeEdge::N_Prime && *visitedEdges & QmddNodeEdge::P);
            }

            [[maybe_unused]] QmddNodeEdge advanceToNextEdge() noexcept {
                if (!visitedEdges.has_value()) {
                    visitedEdges = QmddNodeEdge::N;
                    return QmddNodeEdge::N;
                }

                for (const QmddNodeEdge qmddNodeEdge: {QmddNodeEdge::P, QmddNodeEdge::N_Prime, QmddNodeEdge::P_Prime, QmddNodeEdge::N}) {
                    if (*visitedEdges & qmddNodeEdge) {
                        switch (qmddNodeEdge) {
                            case QmddNodeEdge::N:
                                *visitedEdges |= QmddNodeEdge::P_Prime;
                                return QmddNodeEdge::P_Prime;
                            case QmddNodeEdge::P_Prime:
                                *visitedEdges |= QmddNodeEdge::N_Prime;
                                return QmddNodeEdge::N_Prime;
                            case QmddNodeEdge::N_Prime:
                                *visitedEdges |= QmddNodeEdge::P;
                                return QmddNodeEdge::P;
                            case QmddNodeEdge::P:
                                *visitedEdges |= QmddNodeEdge::N;
                                return QmddNodeEdge::N;
                            default:
                                break;
                        }
                    }
                }
                // TODO:
                return QmddNodeEdge::N;
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
        static void                                      exportQmddToFile(const dd::mEdge* edgeToRootNodeOfQmdd, const std::optional<QmddDumpConfig>& optionalQmddDumpConfig = std::nullopt, QmddExportOutputStreamOperation qmddExportOutputStreamOperation = QmddExportOutputStreamOperation::Append);
        static void                                      getPathsThroughEdgeStartingFromQmddNode(const dd::mNode& qmddNodeToStartPathsFrom, QmddNodeEdge edgesToGeneratePathsFor, QmddNodeAndPathsPerEdge& containerStoringFoundPaths);
        [[nodiscard]] static std::vector<QmddPath>       getAllPathsFromRootToNode(const dd::mNode& qmddRootNode, const dd::mNode& qmddNodeToReach);
        [[nodiscard]] static qc::Controls                getControlQubitsFromSignatureOfQmddPath(const QmddPath& qmddPath) noexcept;
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

        std::reference_wrapper<qc::QuantumComputation> qc;
        std::reference_wrapper<dd::Package>            qmddPkg;
    };
} // namespace syrec
