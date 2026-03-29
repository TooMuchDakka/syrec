/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "algorithms/synthesis/qmddTransformation/qmdd_path_generator.hpp"

#include "algorithms/synthesis/qmddTransformation/qmdd_path_definitions.hpp"
#include "dd/DDDefinitions.hpp"
#include "ir/Definitions.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <variant>

using namespace syrec;

namespace {
    [[nodiscard]] dd::Qubit getLastQubitCoveredByQmddPathEntry(const std::variant<QmddPathComponent, QmddPathGap>& qmddPathEntry) {
        if (const QmddPathGap* qmddPathEndAsGapEntry = std::get_if<QmddPathGap>(&qmddPathEntry); qmddPathEndAsGapEntry != nullptr) {
            return qmddPathEndAsGapEntry->qubitAssociatedWithFirstQmddNodeOfGap - static_cast<dd::Qubit>(qmddPathEndAsGapEntry->nConsecutiveQubitInGap - 1U);
        }

        assert(std::holds_alternative<QmddPathComponent>(qmddPathEntry));
        return std::get<QmddPathComponent>(qmddPathEntry).qubitAssociatedWithQmddNode;
    }

    [[nodiscard]] bool existsGapBetweenQmddPathNodes(const std::variant<QmddPathComponent, QmddPathGap>& currentQmddPathEntry, const std::variant<QmddPathComponent, QmddPathGap>& predecessorQmddPathEntry) {
        const qc::Qubit lastQubitOfPredecessorPathEntry = getLastQubitCoveredByQmddPathEntry(predecessorQmddPathEntry);
        const qc::Qubit qubitOfCurrentQmddEntry         = std::holds_alternative<QmddPathComponent>(currentQmddPathEntry) ? std::get<QmddPathComponent>(currentQmddPathEntry).qubitAssociatedWithQmddNode : std::get<QmddPathGap>(currentQmddPathEntry).qubitAssociatedWithFirstQmddNodeOfGap;

        return qubitOfCurrentQmddEntry != (lastQubitOfPredecessorPathEntry - 1U);
    }
} // namespace

// TODO: Simplify logic
// TODO: Documentation of interface
QmddPathGenerator::QmddPathGenerator(const OptimizedQmddPath& qmddPath) {
    // The generator will not append any qmdd path entries that are not covered by the reference qmdd path (i.e. the constructor argument) and expects that the latter
    // will define entries that cover the whole qubit range from [U, 0] with U equal to the qubit of the first entry of the reference qmdd path.
    if (qmddPath.empty() || getLastQubitCoveredByQmddPathEntry(qmddPath.back()) != 0U) {
        return;
    }

    nCombinationsToGenerate = 1;
    if (std::ranges::all_of(qmddPath, [](const std::variant<QmddPathComponent, QmddPathGap>& qmddPathComponentVariant) { return std::holds_alternative<QmddPathComponent>(qmddPathComponentVariant); })) {
        // TODO: Could we apply this check for all qmdd path variants (i.e. paths that contain gaps) instead of repeating the same element wise check later for qmdd paths that contain gaps?
        for (std::size_t i = 1; i < qmddPath.size(); ++i) {
            if (existsGapBetweenQmddPathNodes(std::get<QmddPathComponent>(qmddPath.at(i)), std::get<QmddPathComponent>(qmddPath.at(i - 1U)))) {
                nCombinationsToGenerate = 0;
                return;
            }
        }
        lastGeneratedCombination.reserve(qmddPath.size());
        std::ranges::transform(qmddPath, std::back_inserter(lastGeneratedCombination), [](const std::variant<QmddPathComponent, QmddPathGap>& qmddPathComponentVariant) { return std::get<QmddPathComponent>(qmddPathComponentVariant); });
        return;
    }

    std::size_t unoptimizedQmddPathLength = 0U;
    if (const QmddPathGap* qmddPathHeadAsGapEntry = std::get_if<QmddPathGap>(&qmddPath.front()); qmddPathHeadAsGapEntry != nullptr) {
        unoptimizedQmddPathLength = qmddPathHeadAsGapEntry->qubitAssociatedWithFirstQmddNodeOfGap + 1U;
    } else {
        assert(std::holds_alternative<QmddPathComponent>(qmddPath.front()));
        const auto& qmddPathHeadAsQmddPathComponent = std::get<QmddPathComponent>(qmddPath.front());
        unoptimizedQmddPathLength                   = qmddPathHeadAsQmddPathComponent.qubitAssociatedWithQmddNode + 1U;
    }

    lastGeneratedCombination.reserve(unoptimizedQmddPathLength);
    // Qubits in qmdd path are expected to be defined in the same order as the variable ordering of the associated qmdd which in turn defines the variable ordering as starting with the "largest" qubit down to the "lowest" qubit.
    for (const dd::Qubit qubit: std::views::iota(static_cast<dd::Qubit>(0U), static_cast<dd::Qubit>(unoptimizedQmddPathLength)) | std::views::reverse) {
        lastGeneratedCombination.emplace_back(qubit, QmddNodeEdge::P);
    }

    std::size_t qmddPathIndexOfCurrentElement = 0U;
    for (auto qmddPathComponentsIterator = qmddPath.begin(); qmddPathComponentsIterator != qmddPath.end(); ++qmddPathComponentsIterator) {
        // If gaps between the entries of the reference qmdd path exist then the generator should not be able to generate any qmdd paths.
        if (std::distance(qmddPath.begin(), qmddPathComponentsIterator) > 0 && existsGapBetweenQmddPathNodes(*qmddPathComponentsIterator, *std::prev(qmddPathComponentsIterator))) {
            nCombinationsToGenerate = 0;
            return;
        }

        const std::variant<QmddPathComponent, QmddPathGap>& qmddPathComponentVariant = *qmddPathComponentsIterator;
        if (const QmddPathGap* const qmddPathEntryAsGapComponent = std::get_if<QmddPathGap>(&qmddPathComponentVariant); qmddPathEntryAsGapComponent != nullptr) {
            nCombinationsToGenerate *= 2 * qmddPathEntryAsGapComponent->nConsecutiveQubitInGap;
            qmddPathIndexOfCurrentElement += qmddPathEntryAsGapComponent->nConsecutiveQubitInGap;
            continue;
        }

        assert(std::holds_alternative<QmddPathComponent>(*qmddPathComponentsIterator));
        const auto& qmddPathComponentCasted                                         = std::get<QmddPathComponent>(*qmddPathComponentsIterator);
        lastGeneratedCombination[qmddPathIndexOfCurrentElement].qmddEdgeToChildNode = qmddPathComponentCasted.qmddEdgeToChildNode;
        nonGapQmddPathIndices.emplace(qmddPathIndexOfCurrentElement);
        ++qmddPathIndexOfCurrentElement;
    }
}

const UnoptimizedQmddPath* QmddPathGenerator::tryGenerateNextPath() {
    if (nCombinationsToGenerate == nGeneratedCombinations) {
        return nullptr;
    }

    ++nGeneratedCombinations;
    if (nCombinationsToGenerate == 1) {
        return &lastGeneratedCombination;
    }

    assert(nonGapQmddPathIndices.size() != lastGeneratedCombination.size());
    for (const std::size_t lastGeneratedCombinationIdx: std::views::iota(0U, lastGeneratedCombination.size()) | std::views::reverse) {
        // TODO: Check whether we should use the gap indices instead since gaps should occur more rarely (that would be our current assumption).
        if (nonGapQmddPathIndices.contains(lastGeneratedCombinationIdx)) {
            continue;
        }
        QmddNodeEdge& lastGenerationCombinationEntry = lastGeneratedCombination[lastGeneratedCombinationIdx].qmddEdgeToChildNode;
        lastGenerationCombinationEntry               = lastGenerationCombinationEntry == QmddNodeEdge::P ? QmddNodeEdge::N : QmddNodeEdge::P;
        if (lastGenerationCombinationEntry == QmddNodeEdge::P) {
            break;
        }
    }
    return &lastGeneratedCombination;
}
