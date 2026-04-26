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

namespace syrec {
    constexpr std::array<dd::mEdge, 4U> createEdgeArrayForLeafNode(const QmddNodeEdge aggregateOfEdgesToOneTerminals) {
        static_assert(convertQmddNodeEdgeEnumValueToArrayIdx(QmddNodeEdge::N) == 0U);
        static_assert(convertQmddNodeEdgeEnumValueToArrayIdx(QmddNodeEdge::PPrime) == 1U);
        static_assert(convertQmddNodeEdgeEnumValueToArrayIdx(QmddNodeEdge::NPrime) == 2U);
        static_assert(convertQmddNodeEdgeEnumValueToArrayIdx(QmddNodeEdge::P) == 3U);

        return {
                aggregateOfEdgesToOneTerminals & QmddNodeEdge::N ? dd::mEdge::one() : dd::mEdge::zero(),
                aggregateOfEdgesToOneTerminals & QmddNodeEdge::PPrime ? dd::mEdge::one() : dd::mEdge::zero(),
                aggregateOfEdgesToOneTerminals & QmddNodeEdge::NPrime ? dd::mEdge::one() : dd::mEdge::zero(),
                aggregateOfEdgesToOneTerminals & QmddNodeEdge::P ? dd::mEdge::one() : dd::mEdge::zero()};
    }

    inline dd::mEdge createLeafQmddNode(dd::Package& ddPkg, const dd::Qubit qubit, const QmddNodeEdge aggregateOfEdgesToOneTerminals) {
        return ddPkg.makeDDNode<dd::mNode>(qubit, createEdgeArrayForLeafNode(aggregateOfEdgesToOneTerminals));
    }

    inline dd::mEdge createNonLeafQmddNode(dd::Package& ddPkg, const dd::Qubit qubit,
                                           const dd::mEdge& nEdge      = dd::mEdge::zero(),
                                           const dd::mEdge& pPrimeEdge = dd::mEdge::zero(),
                                           const dd::mEdge& nPrimeEdge = dd::mEdge::zero(),
                                           const dd::mEdge& pEdge      = dd::mEdge::zero()) {
        // TODO: Supply set of non-zero edges instead of having to explicitly define all edges
        static_assert(convertQmddNodeEdgeEnumValueToArrayIdx(QmddNodeEdge::N) == 0U);
        static_assert(convertQmddNodeEdgeEnumValueToArrayIdx(QmddNodeEdge::PPrime) == 1U);
        static_assert(convertQmddNodeEdgeEnumValueToArrayIdx(QmddNodeEdge::NPrime) == 2U);
        static_assert(convertQmddNodeEdgeEnumValueToArrayIdx(QmddNodeEdge::P) == 3U);

        return ddPkg.makeDDNode<dd::mNode>(qubit, std::array<dd::mEdge, 4U>({nEdge, pPrimeEdge, nPrimeEdge, pEdge}));
    }
} // namespace syrec
