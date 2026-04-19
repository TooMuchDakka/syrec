/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "algorithms/synthesis/qmddTransformation/qmdd_path_definitions.hpp"
#include "algorithms/synthesis/qmddTransformation/qmdd_traversal.hpp"
#include "dd/Node.hpp"
#include "dd/Package.hpp"

#include "gmock/gmock-matchers.h"
#include <gtest/gtest.h>
using namespace syrec;

namespace {
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

    dd::mEdge createLeafQmddNode(dd::Package& ddPkg, const dd::Qubit qubit, const QmddNodeEdge aggregateOfEdgesToOneTerminals) {
        return ddPkg.makeDDNode<dd::mNode>(qubit, createEdgeArrayForLeafNode(aggregateOfEdgesToOneTerminals));
    }

    dd::mEdge createNonLeafQmddNode(dd::Package& ddPkg, const dd::Qubit qubit,
                                    const dd::mEdge& nEdge      = dd::mEdge::zero(),
                                    const dd::mEdge& pPrimeEdge = dd::mEdge::zero(),
                                    const dd::mEdge& nPrimeEdge = dd::mEdge::zero(),
                                    const dd::mEdge& pEdge      = dd::mEdge::zero()) {
        static_assert(convertQmddNodeEdgeEnumValueToArrayIdx(QmddNodeEdge::N) == 0U);
        static_assert(convertQmddNodeEdgeEnumValueToArrayIdx(QmddNodeEdge::PPrime) == 1U);
        static_assert(convertQmddNodeEdgeEnumValueToArrayIdx(QmddNodeEdge::NPrime) == 2U);
        static_assert(convertQmddNodeEdgeEnumValueToArrayIdx(QmddNodeEdge::P) == 3U);

        return ddPkg.makeDDNode<dd::mNode>(qubit, std::array<dd::mEdge, 4U>({nEdge, pPrimeEdge, nPrimeEdge, pEdge}));
    }

    bool operator==(const OptimizedQmddPath& expectedQmddPath, const OptimizedQmddPath& actualQmddPath) {
        return expectedQmddPath.size() == actualQmddPath.size() && std::find_first_of(
                                                                           expectedQmddPath.cbegin(), expectedQmddPath.cend(),
                                                                           actualQmddPath.cbegin(), actualQmddPath.cbegin(),
                                                                           [](const std::variant<QmddPathComponent, QmddPathGap>& expected, const std::variant<QmddPathComponent, QmddPathGap>& actual) {
                                                                               const QmddPathComponent* expectedQmddPathComponent = std::get_if<QmddPathComponent>(&expected);
                                                                               const QmddPathComponent* actualQmddPathComponent   = std::get_if<QmddPathComponent>(&actual);
                                                                               const bool               doQmddPathComponentsMatch = expectedQmddPathComponent != nullptr ? actualQmddPathComponent != nullptr && *expectedQmddPathComponent == *actualQmddPathComponent : false;

                                                                               const QmddPathGap* expectedQmddPathGap = std::get_if<QmddPathGap>(&expected);
                                                                               const QmddPathGap* actualQmddPathGap   = std::get_if<QmddPathGap>(&actual);
                                                                               const bool         doQmddPathGapsMatch = expectedQmddPathGap != nullptr ? actualQmddPathGap != nullptr && *expectedQmddPathGap == *actualQmddPathGap : false;
                                                                               return doQmddPathComponentsMatch || doQmddPathGapsMatch;
                                                                           }) == expectedQmddPath.cend();
    }

    void assertQmddPathCollectionsMatch(const std::vector<OptimizedQmddPath>& expected, const std::vector<OptimizedQmddPath>& actual) {
        ASSERT_EQ(expected.size(), actual.size()) << "Number of found qmdd paths does not match!";
        ASSERT_THAT(actual, testing::UnorderedElementsAreArray(expected));
    }

    void assertQmddPathCollectionsMatch(const std::vector<UnoptimizedQmddPath>& expected, const std::vector<UnoptimizedQmddPath>& actual) {
        ASSERT_THAT(actual, testing::UnorderedElementsAreArray(expected));
    }

    void getAllPathsForQmddEdgeAndAssertAllExpectedOnesAreFound(const QmddNodeEdge expectedOriginEdgeOfPaths) {
        /* Generated via: https://asciiflow.com/
         *                                         +---+
         *                            +----------X-+ 3 |
         *                            |            +---+
         *                            |
         *                            |
         *                          +-+-+
         *            +----n--------+ 2 +-----p---+
         *            |             ++-++         |
         *            |              | |          |
         *          +-+-+     +---+-p' n'       +-+-+
         *          | 1 |     | 1 |    |        | 1 |
         *          ++-++     +-+-+    1        +-+-+
         *           | |        |                 n
         * +---+-p'--+ |        n                 |
         * | 0 |       p        |                 1
         * +-+-+ +---+-+      +-+-+
         *   |   | 0 |        | 0 |
         *   n   +-+-+        +-+-+
         *   |     p            p'
         *   1     |            |
         *         1            1
         */
        const auto      ddPkg            = std::make_unique<dd::Package>(4U);
        const dd::mEdge x3n2pPrime1Node0 = createLeafQmddNode(*ddPkg, 0U, QmddNodeEdge::N);
        const dd::mEdge x3n2p1Node0      = createLeafQmddNode(*ddPkg, 0U, QmddNodeEdge::P);
        const dd::mEdge x3n2Node1        = createNonLeafQmddNode(*ddPkg, 1U, dd::mEdge::zero(), x3n2pPrime1Node0, dd::mEdge::zero(), x3n2p1Node0);

        const dd::mEdge x3pPrime2n1Node0 = createLeafQmddNode(*ddPkg, 0U, QmddNodeEdge::PPrime);
        const dd::mEdge x3pPrime2Node1   = createNonLeafQmddNode(*ddPkg, 1U, x3pPrime2n1Node0, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());

        const dd::mEdge x3p2Node1 = createLeafQmddNode(*ddPkg, 1U, QmddNodeEdge::N);
        const dd::mEdge x3Node2   = createNonLeafQmddNode(*ddPkg, 2U, x3n2Node1, x3pPrime2Node1, dd::mEdge::one(), x3p2Node1);
        const dd::mEdge node3     = createNonLeafQmddNode(*ddPkg, 3U,
                                                      expectedOriginEdgeOfPaths == QmddNodeEdge::N ? x3Node2 : dd::mEdge::zero(),
                                                      expectedOriginEdgeOfPaths == QmddNodeEdge::PPrime ? x3Node2 : dd::mEdge::zero(),
                                                      expectedOriginEdgeOfPaths == QmddNodeEdge::NPrime ? x3Node2 : dd::mEdge::zero(),
                                                      expectedOriginEdgeOfPaths == QmddNodeEdge::P ? x3Node2 : dd::mEdge::zero());

        QmddNodeAndPathsPerEdge qmddPathsContainer = {.associatedQmddNode = *node3.p, .nEdgePaths = {}, .pPrimeEdgePaths = {}, .nPrimeEdgePaths = {}, .pEdgePaths = {}};
        getPathsToOneTerminalThroughEdgeOfQmddNode(qmddPathsContainer.associatedQmddNode, expectedOriginEdgeOfPaths, qmddPathsContainer);

        const auto expectedEdgePaths = std::vector({OptimizedQmddPath({
                                                            QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = expectedOriginEdgeOfPaths}),
                                                            QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                    }),
                                                    OptimizedQmddPath({
                                                            QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = expectedOriginEdgeOfPaths}),
                                                            QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                    }),
                                                    OptimizedQmddPath({
                                                            QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = expectedOriginEdgeOfPaths}),
                                                            QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                    }),
                                                    OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = expectedOriginEdgeOfPaths}),
                                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                       QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})}),
                                                    OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = expectedOriginEdgeOfPaths}),
                                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                       QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 0U, .nConsecutiveQubitInGap = 1U})})});
        ASSERT_NO_FATAL_FAILURE(assertQmddPathCollectionsMatch(expectedOriginEdgeOfPaths == QmddNodeEdge::N ? expectedEdgePaths : std::vector<OptimizedQmddPath>(), qmddPathsContainer.nEdgePaths));
        ASSERT_NO_FATAL_FAILURE(assertQmddPathCollectionsMatch(expectedOriginEdgeOfPaths == QmddNodeEdge::PPrime ? expectedEdgePaths : std::vector<OptimizedQmddPath>(), qmddPathsContainer.pPrimeEdgePaths));
        ASSERT_NO_FATAL_FAILURE(assertQmddPathCollectionsMatch(expectedOriginEdgeOfPaths == QmddNodeEdge::NPrime ? expectedEdgePaths : std::vector<OptimizedQmddPath>(), qmddPathsContainer.nPrimeEdgePaths));
        ASSERT_NO_FATAL_FAILURE(assertQmddPathCollectionsMatch(expectedOriginEdgeOfPaths == QmddNodeEdge::P ? expectedEdgePaths : std::vector<OptimizedQmddPath>(), qmddPathsContainer.pEdgePaths));
    }

    void initializeBigQmdd(std::unique_ptr<dd::Package>& qmddPkg, dd::mEdge& edgeToRootNode) {
        /* Generated via: https://asciiflow.com/
         *                                                +---+
         *                            +----------n--------+ 4 +----------p--------+
         *                            |                   +---+                   |
         *                            |                                           |
         *                            |                                           |
         *                          +-+-+                                       +-+-+
         *            +----n--------+ 3 +------p--------+                       | 2 |
         *            |             ++-++               |                       +-+-+
         *            |              | |                |                         |
         *          +-+-+     +---+-p' +n'+---+       +-+-+                       p'
         *          | 2 |     | 2 |       | 2 |       | 2 |                       |
         *          ++-++     +-+-+       +-+-+       ++-++                     +-+-+
         *           | |        |           | |        | |                      | 0 |
         * +---+-n---+ |        n        +--p'|-n'+    n p----+                 ++-++
         * | 1 |       p'       |        |        |    |      |                  | |
         * +-+-+ +---+-+      +-+-+    +-+-+   +--++   +---+  1                  n n'
         *   |   | 1 |        | 1 |    | 1 |   | 1 |   | 1 |                     | |
         *   p'  +-+-+        ++--+    +-+-+   +-+-+   +-+-+                     1 1
         *   |     |           |  |    |       |         |
         *   1     n           n  p    n       p         n--1
         *         |           |  |    |       |
         *         1    +---+--+ ++--+ 1       +----+---+
         *              | 0 |    | 0 |              | 0 +--p--1
         *              +-+-+    ++-++              +++++
         *                |       | |                |||
         *                p'      n'p             -n-+|+-n'-
         *                |       | |             |   p'   |
         *                1       1 1             1   1    1
         */
        qmddPkg                          = std::make_unique<dd::Package>(5U);
        const dd::mEdge n4n3n2node1      = createLeafQmddNode(*qmddPkg, 1U, QmddNodeEdge::PPrime);
        const dd::mEdge n4n3pPrime2node1 = createLeafQmddNode(*qmddPkg, 1U, QmddNodeEdge::N);
        const dd::mEdge n4n3node2        = createNonLeafQmddNode(*qmddPkg, 2U, n4n3n2node1, n4n3pPrime2node1, dd::mEdge::zero(), dd::mEdge::zero());

        const dd::mEdge n4pPrime3n2n1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::PPrime);
        const dd::mEdge n4pPrime3n2p1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::NPrime | QmddNodeEdge::P);
        const dd::mEdge n4pPrime3n2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, n4pPrime3n2n1Node0, dd::mEdge::zero(), dd::mEdge::zero(), n4pPrime3n2p1Node0);
        const dd::mEdge n4pPrime3Node2     = createNonLeafQmddNode(*qmddPkg, 2U, n4pPrime3n2Node1, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());

        const dd::mEdge n4nPrime3pPrime2Node1   = createLeafQmddNode(*qmddPkg, 1U, QmddNodeEdge::N);
        const dd::mEdge n4nPrime3nPrime2p1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P);
        const dd::mEdge n4nPrime3nPrime2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero(), n4nPrime3nPrime2p1Node0);
        const dd::mEdge n4nPrime3Node2          = createNonLeafQmddNode(*qmddPkg, 2U, dd::mEdge::zero(), n4nPrime3pPrime2Node1, n4nPrime3nPrime2Node1, dd::mEdge::zero());

        const dd::mEdge n4p3n2Node1 = createLeafQmddNode(*qmddPkg, 1U, QmddNodeEdge::N);
        const dd::mEdge n4p3Node2   = createNonLeafQmddNode(*qmddPkg, 2U, n4p3n2Node1, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::one());
        const dd::mEdge n4Node3     = createNonLeafQmddNode(*qmddPkg, 3U, n4n3node2, n4pPrime3Node2, n4nPrime3Node2, n4p3Node2);

        const dd::mEdge p4p2Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::N | QmddNodeEdge::NPrime);
        const dd::mEdge p4Node2   = createNonLeafQmddNode(*qmddPkg, 2U, dd::mEdge::zero(), p4p2Node0, dd::mEdge::zero(), dd::mEdge::zero());
        const dd::mEdge rootNode  = createNonLeafQmddNode(*qmddPkg, 4U, n4Node3, dd::mEdge::zero(), dd::mEdge::zero(), p4Node2);
        qmddPkg->incRef(rootNode);

        const auto& rootSet = qmddPkg->getRootSet<dd::mNode>();
        ASSERT_EQ(1U, rootSet.size());
        ASSERT_EQ(rootNode.p, rootSet.cbegin()->first.p);
        edgeToRootNode = rootSet.cbegin()->first;
    }
} // namespace

TEST(QmddTraversalTest, CheckAllPathsToOneTerminalInSubtreeFound) {
    /* Generated via: https://asciiflow.com/
         *                                                +---+
         *                            +----------n--------+ 4 +----------p--------+
         *                            |                   +---+                   |
         *                            |                                           |
         *                            |                                           |
         *                          +-+-+                                       +-+-+
         *            +----n--------+ 3 +------p--------+                       | 2 |
         *            |             ++-++               |                       +-+-+
         *            |              | |                |                         |
         *          +-+-+     +---+-p' +n'+---+       +-+-+                       p'
         *          | 2 |     | 2 |       | 2 |       | 2 |                       |
         *          ++-++     +-+-+       +-+-+       ++-++                     +-+-+
         *           | |        |           | |        | |                      | 0 |
         * +---+-n---+ |        n        +--p'|-n'+    n p----+                 ++-++
         * | 1 |       p'       |        |        |    |      |                  | |
         * +-+-+ +---+-+      +-+-+    +-+-+   +--++   +---+  1                  n n'
         *   |   | 1 |        | 1 |    | 1 |   | 1 |   | 1 |                     | |
         *   p'  +-+-+        ++--+    +-+-+   +-+-+   +-+-+                     1 1
         *   |     |           |  |    |       |         |
         *   1     n           n  p    n       p         n--1
         *         |           |  |    |       |
         *         1    +---+--+ ++--+ 1       +----+---+
         *              | 0 |    | 0 |              | 0 +--p--1
         *              +-+-+    ++-++              +++++
         *                |       | |                |||
         *                p'      n'p             -n-+|+-n'-
         *                |       | |             |   p'   |
         *                1       1 1             1   1    1
         */
    const auto      qmddPkg          = std::make_unique<dd::Package>(5U);
    const dd::mEdge n4n3n2node1      = createLeafQmddNode(*qmddPkg, 1U, QmddNodeEdge::PPrime);
    const dd::mEdge n4n3pPrime2node1 = createLeafQmddNode(*qmddPkg, 1U, QmddNodeEdge::N);
    const dd::mEdge n4n3node2        = createNonLeafQmddNode(*qmddPkg, 2U, n4n3n2node1, n4n3pPrime2node1, dd::mEdge::zero(), dd::mEdge::zero());

    const dd::mEdge n4pPrime3n2n1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::PPrime);
    const dd::mEdge n4pPrime3n2p1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::NPrime | QmddNodeEdge::P);
    const dd::mEdge n4pPrime3n2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, n4pPrime3n2n1Node0, dd::mEdge::zero(), dd::mEdge::zero(), n4pPrime3n2p1Node0);
    const dd::mEdge n4pPrime3Node2     = createNonLeafQmddNode(*qmddPkg, 2U, n4pPrime3n2Node1, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());

    const dd::mEdge n4nPrime3pPrime2Node1   = createLeafQmddNode(*qmddPkg, 1U, QmddNodeEdge::N);
    const dd::mEdge n4nPrime3nPrime2p1Node0 = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::N | QmddNodeEdge::PPrime | QmddNodeEdge::NPrime | QmddNodeEdge::P);
    const dd::mEdge n4nPrime3nPrime2Node1   = createNonLeafQmddNode(*qmddPkg, 1U, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero(), n4nPrime3nPrime2p1Node0);
    const dd::mEdge n4nPrime3Node2          = createNonLeafQmddNode(*qmddPkg, 2U, dd::mEdge::zero(), n4nPrime3pPrime2Node1, n4nPrime3nPrime2Node1, dd::mEdge::zero());

    const dd::mEdge n4p3n2Node1 = createLeafQmddNode(*qmddPkg, 1U, QmddNodeEdge::N);
    const dd::mEdge n4p3Node2   = createNonLeafQmddNode(*qmddPkg, 2U, n4p3n2Node1, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::one());
    const dd::mEdge n4Node3     = createNonLeafQmddNode(*qmddPkg, 3U, n4n3node2, n4pPrime3Node2, n4nPrime3Node2, n4p3Node2);

    const dd::mEdge p4p2Node0      = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::N | QmddNodeEdge::NPrime);
    const dd::mEdge p4Node2        = createNonLeafQmddNode(*qmddPkg, 2U, dd::mEdge::zero(), p4p2Node0, dd::mEdge::zero(), dd::mEdge::zero());
    const dd::mEdge edgeToRootNode = createNonLeafQmddNode(*qmddPkg, 4U, n4Node3, dd::mEdge::zero(), dd::mEdge::zero(), p4Node2);
    qmddPkg->incRef(edgeToRootNode);

    const auto& rootSet = qmddPkg->getRootSet<dd::mNode>();
    ASSERT_EQ(1U, rootSet.size());
    ASSERT_EQ(edgeToRootNode.p, rootSet.cbegin()->first.p);
    const dd::mNode* rootNode = edgeToRootNode.p;

    QmddNodeAndPathsPerEdge qmddPathsContainer = {.associatedQmddNode = *rootNode, .nEdgePaths = {}, .pPrimeEdgePaths = {}, .nPrimeEdgePaths = {}, .pEdgePaths = {}};
    getPathsToOneTerminalThroughEdgeOfQmddNode(qmddPathsContainer.associatedQmddNode, QmddNodeEdge::N, qmddPathsContainer);

    const auto expectedNEdgePaths = std::vector({OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                    QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 0U, .nConsecutiveQubitInGap = 1U})}),
                                                 OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 0U, .nConsecutiveQubitInGap = 1U})}),
                                                 OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})}),
                                                 OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})}),
                                                 OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})}),
                                                 OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 0U, .nConsecutiveQubitInGap = 1U})}),
                                                 OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N})}),
                                                 OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})}),
                                                 OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})}),
                                                 OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})}),
                                                 OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 0U, .nConsecutiveQubitInGap = 1U})}),
                                                 OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                    QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})})});
    const auto expectedPEdgePaths = std::vector({OptimizedQmddPath({
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 1U}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                         QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 1U}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                 }),
                                                 OptimizedQmddPath({
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 1U}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                         QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 1U}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                 })});
    ASSERT_NO_FATAL_FAILURE(assertQmddPathCollectionsMatch(expectedNEdgePaths, qmddPathsContainer.nEdgePaths));
    ASSERT_NO_FATAL_FAILURE(assertQmddPathCollectionsMatch({}, qmddPathsContainer.pPrimeEdgePaths));
    ASSERT_NO_FATAL_FAILURE(assertQmddPathCollectionsMatch({}, qmddPathsContainer.nPrimeEdgePaths));
    ASSERT_NO_FATAL_FAILURE(assertQmddPathCollectionsMatch({}, qmddPathsContainer.pEdgePaths));

    getPathsToOneTerminalThroughEdgeOfQmddNode(qmddPathsContainer.associatedQmddNode, QmddNodeEdge::P, qmddPathsContainer);
    ASSERT_NO_FATAL_FAILURE(assertQmddPathCollectionsMatch(expectedNEdgePaths, qmddPathsContainer.nEdgePaths));
    ASSERT_NO_FATAL_FAILURE(assertQmddPathCollectionsMatch({}, qmddPathsContainer.pPrimeEdgePaths));
    ASSERT_NO_FATAL_FAILURE(assertQmddPathCollectionsMatch({}, qmddPathsContainer.nPrimeEdgePaths));
    ASSERT_NO_FATAL_FAILURE(assertQmddPathCollectionsMatch(expectedPEdgePaths, qmddPathsContainer.pEdgePaths));
}

TEST(QmddTraversalTest, CheckAllPathsForNEdgeAreFound) {
    getAllPathsForQmddEdgeAndAssertAllExpectedOnesAreFound(QmddNodeEdge::N);
}

TEST(QmddTraversalTest, CheckAllPathsForPPrimeEdgeAreFound) {
    getAllPathsForQmddEdgeAndAssertAllExpectedOnesAreFound(QmddNodeEdge::PPrime);
}

TEST(QmddTraversalTest, CheckAllPathsForNPrimeEdgeAreFound) {
    getAllPathsForQmddEdgeAndAssertAllExpectedOnesAreFound(QmddNodeEdge::NPrime);
}

TEST(QmddTraversalTest, CheckAllPathsForPEdgeAreFound) {
    getAllPathsForQmddEdgeAndAssertAllExpectedOnesAreFound(QmddNodeEdge::P);
}

TEST(QmddTraversalTest, CheckPathsFromRootToNotExistingChildAreEmpty) {
    std::unique_ptr<dd::Package> qmddPkg;
    dd::mEdge                    edgeToRootNode;
    ASSERT_NO_FATAL_FAILURE(initializeBigQmdd(qmddPkg, edgeToRootNode));
    const dd::mNode* rootNode = edgeToRootNode.p;

    const std::vector<UnoptimizedQmddPath> expectedFoundQmddPaths;
    const std::vector<UnoptimizedQmddPath> actualFoundQmddPaths = getAllPathsFromRootToNode(*rootNode, dd::mNode());
    assertQmddPathCollectionsMatch(expectedFoundQmddPaths, actualFoundQmddPaths);
}

TEST(QmddTraversalTest, CheckNoPathFromRootToRootFound) {
    std::unique_ptr<dd::Package> qmddPkg;
    dd::mEdge                    edgeToRootNode;
    ASSERT_NO_FATAL_FAILURE(initializeBigQmdd(qmddPkg, edgeToRootNode));
    const dd::mNode* rootNode = edgeToRootNode.p;

    const std::vector<UnoptimizedQmddPath> expectedFoundQmddPaths;
    const std::vector<UnoptimizedQmddPath> actualFoundQmddPaths = getAllPathsFromRootToNode(*rootNode, *rootNode);
    assertQmddPathCollectionsMatch(expectedFoundQmddPaths, actualFoundQmddPaths);
}

TEST(QmddTraversalTest, AssertAllPathsFromRootToNodeAreFound) {
    /* Generated via: https://asciiflow.com/
         *                    +---+
         *          +----n----+ 4 +---p-----+
         *          |         ++-++         |
         *       +--++         | n'       +-+-+
         *   +---+ 3 |         p'|        | 3 |
         *   n   +---++   +---+| |  +---+ +-+-+
         *   |        |   | 3 ++ +--+ 3 |   p
         * +-+-+      |   +-+-+   +-+---+   |
         * | 2 |      |     |     |       +-+-+
         * +-+-+      |     |     |  +----+ 2 |
         *   |        |     p'    |  |    +-+-+
         *   n        p     |     n  |      n'
         *   |        |     |     |  |      |
         *   |      +-+-+   |     |  |    +-+-+
         *   +------+ 1 +---+     |  n    | 1 |
         *          +--++         |  |    ++--+
         *             |          |  |     |
         *             n          |  |     p
         *             |       +--++-+     |
         *             +-------+ 0 +-------+
         *                     +-+-+
         *                       |
         *                       p
         *                       |
         *                       1
         */
    const auto qmddPkg = std::make_unique<dd::Package>(5U);

    const dd::mEdge nodeOfInterest = createLeafQmddNode(*qmddPkg, 0U, QmddNodeEdge::P);
    const auto      n4n3n2Node1    = createNonLeafQmddNode(*qmddPkg, 1U, nodeOfInterest, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    const auto      n4n3Node2      = createNonLeafQmddNode(*qmddPkg, 2U, n4n3n2Node1, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());
    const auto      n4Node3        = createNonLeafQmddNode(*qmddPkg, 3U, n4n3Node2, dd::mEdge::zero(), dd::mEdge::zero(), n4n3n2Node1);

    const auto pPrime4Node3 = createNonLeafQmddNode(*qmddPkg, 3U, dd::mEdge::zero(), n4n3n2Node1, dd::mEdge::zero(), dd::mEdge::zero());
    const auto nPrime4Node3 = createNonLeafQmddNode(*qmddPkg, 3U, nodeOfInterest, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero());

    const auto p4p3nPrime2Node1 = createNonLeafQmddNode(*qmddPkg, 1U, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero(), nodeOfInterest);
    const auto p4p3Node2        = createNonLeafQmddNode(*qmddPkg, 2U, nodeOfInterest, dd::mEdge::zero(), p4p3nPrime2Node1, dd::mEdge::zero());
    const auto p4Node3          = createNonLeafQmddNode(*qmddPkg, 3U, dd::mEdge::zero(), dd::mEdge::zero(), dd::mEdge::zero(), p4p3Node2);
    const auto edgeToRootNode   = createNonLeafQmddNode(*qmddPkg, 4U, n4Node3, pPrime4Node3, nPrime4Node3, p4Node3);
    qmddPkg->incRef(edgeToRootNode);

    const auto& rootSet = qmddPkg->getRootSet<dd::mNode>();
    ASSERT_EQ(1U, rootSet.size());
    ASSERT_EQ(edgeToRootNode.p, rootSet.cbegin()->first.p);

    const dd::mNode* rootNode   = rootSet.cbegin()->first.p;
    const dd::mNode* nodeToFind = nodeOfInterest.p;

    const auto                             expectedPathsFromRootToNode = std::vector({UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N})}),
                                                                                      UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N})}),
                                                                                      UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N})}),
                                                                                      UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N})}),
                                                                                      UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P})})});
    const std::vector<UnoptimizedQmddPath> actualPathsFromRootToNode   = getAllPathsFromRootToNode(*rootNode, *nodeToFind);
    assertQmddPathCollectionsMatch(expectedPathsFromRootToNode, actualPathsFromRootToNode);
}
