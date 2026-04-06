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
#include "algorithms/synthesis/qmddTransformation/qmdd_path_operations.hpp"
#include "dd/DDDefinitions.hpp"

#include "gmock/gmock-matchers.h"
#include <cstddef>
#include <gtest/gtest.h>

using namespace syrec;

namespace {
    void assertUnoptimizedQmddPathsMatch(const std::optional<UnoptimizedQmddPath>& expected, const std::optional<UnoptimizedQmddPath>& actual) {
        if (expected.has_value()) {
            ASSERT_TRUE(actual.has_value()) << "Expected unoptimized qmdd path to exist!";
            ASSERT_THAT(*actual, testing::ElementsAreArray(*expected));
        } else {
            ASSERT_FALSE(actual.has_value()) << "Expected unoptimized qmdd path to not exist!";
        }
    }

    void assertOperandsToTurnQmddPathUniqueMatch(const std::optional<ToUniqueQmddPathSignatureOperands>& expected, const std::optional<ToUniqueQmddPathSignatureOperands>& actual) {
        if (expected.has_value()) {
            ASSERT_TRUE(actual.has_value()) << "Expected operands to turn qmdd path unique to exist!";
            ASSERT_EQ(*expected, *actual);
        } else {
            ASSERT_FALSE(actual.has_value()) << "Expected operands to turn qmdd path unique to not exist!";
        }
    }
} // namespace

TEST(QmddPathOperationTests, CheckUnrolledLengthOfEmptyOptimizedQmddPath) {
    ASSERT_EQ(0U, getUnrolledLengthOfOptimizedQmddPath(OptimizedQmddPath()));
}

TEST(QmddPathOperationTests, CheckUnrolledLengthOfOptimizedQmddPathContainingSingleEntry) {
    const auto referenceQmddPath = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 4U})});
    ASSERT_EQ(4U, getUnrolledLengthOfOptimizedQmddPath(referenceQmddPath));
}

TEST(QmddPathOperationTests, CheckUnrolledLengthOfOptimizedQmddPathContainingNoGaps) {
    const auto referenceQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                      QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})});
    ASSERT_EQ(2U, getUnrolledLengthOfOptimizedQmddPath(referenceQmddPath));
}

TEST(QmddPathOperationTests, CheckUnrolledLengthOfOptimizedQmddPathContainingGaps) {
    const auto referenceQmddPath = OptimizedQmddPath({
            QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 4U, .nConsecutiveQubitInGap = 2U}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U}),
    });
    ASSERT_EQ(5U, getUnrolledLengthOfOptimizedQmddPath(referenceQmddPath));
}

TEST(QmddPathOperationTests, CheckNumberOfPathsForEmptyQmddPath) {
    ASSERT_EQ(0U, getNumberOfPathsToOneTerminalForQmddPath(OptimizedQmddPath()));
}

TEST(QmddPathOperationTests, CheckNumberOfPathsForQmddPathNotContainingOptimizedGaps) {
    auto qmddPathWithoutGaps = OptimizedQmddPath();
    for (std::size_t i = 0; i < 3U; ++i) {
        qmddPathWithoutGaps.emplace_back(QmddPathComponent({.qubitAssociatedWithQmddNode = static_cast<dd::Qubit>(i), .qmddEdgeToChildNode = QmddNodeEdge::N}));
    }
    ASSERT_EQ(1U, getNumberOfPathsToOneTerminalForQmddPath(qmddPathWithoutGaps));
}

TEST(QmddPathOperationTests, CheckNumberPathsForQmddPathContainingOptimizedGaps) {
    const auto qmddPathWithGaps = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 5U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                     QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 4U, .nConsecutiveQubitInGap = 2U}),
                                                     QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
                                                     QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N})});
    ASSERT_EQ(16U, getNumberOfPathsToOneTerminalForQmddPath(qmddPathWithGaps));
}

TEST(QmddPathOperationTests, CheckNumberOfPathsForEmptyCollectionOfQmddPaths) {
    ASSERT_EQ(0U, getNumberOfPathsToOneTerminalForQmddPaths({}));
}

TEST(QmddPathOperationTests, CheckNumberOfPathsForCollectionOfQmddPaths) {
    auto       qmddPathCollection  = std::vector<OptimizedQmddPath>();
    const auto qmddPathWithoutGaps = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                        QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                        QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})});
    const auto emptyQmddPath       = OptimizedQmddPath();
    const auto qmddPathWithGaps    = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                        QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})});
    ASSERT_EQ(1U + 4U, getNumberOfPathsToOneTerminalForQmddPaths({qmddPathWithoutGaps, emptyQmddPath, qmddPathWithGaps}));
}

TEST(QmddPathOperationTests, CheckQmddPathsMatch) {
    const auto referenceQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                        QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                        QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})});

    const auto qmddPathWithQmddEdgesOfSamePolarity = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    ASSERT_TRUE(doQmddPathSignaturesMatch(referenceQmddPath, referenceQmddPath, false));
    ASSERT_TRUE(doQmddPathSignaturesMatch(referenceQmddPath, qmddPathWithQmddEdgesOfSamePolarity, false));
}

TEST(QmddPathOperationTests, CheckQmddPathsDoNotMatchIfPathLengthsDoNotMatch) {
    const auto referenceQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                        QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                        QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})});

    const auto truncatedReferenceQmddPath = UnoptimizedQmddPath({referenceQmddPath.at(0), referenceQmddPath.at(1)});
    ASSERT_FALSE(doQmddPathSignaturesMatch(referenceQmddPath, truncatedReferenceQmddPath, false));
    ASSERT_FALSE(doQmddPathSignaturesMatch(truncatedReferenceQmddPath, referenceQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathsDoNotMatchIfQubitsOfQmddNodeDoNotMatch) {
    const auto referenceQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                        QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})});

    const auto comparedToQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P})});
    ASSERT_FALSE(doQmddPathSignaturesMatch(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathsDoNotMatchIfSignatureOfQmddEdgesDoNotMatch) {
    const auto referenceQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                        QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})});

    const auto comparedToQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N})});
    ASSERT_FALSE(doQmddPathSignaturesMatch(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathsMatchIfBothAreEmpty) {
    ASSERT_TRUE(doQmddPathSignaturesMatch({}, {}, false));
}

TEST(QmddPathOperationTests, CheckQmddPathDoNotMatchIfOneIfEmpty) {
    const auto referenceQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})});
    ASSERT_FALSE(doQmddPathSignaturesMatch(referenceQmddPath, {}, false));
    ASSERT_FALSE(doQmddPathSignaturesMatch({}, referenceQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsConsideredUniqueIfOnlyPartialMatchInQmddPathWithoutGapsExists) {
    const auto referenceQmddPath  = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const auto comparedToQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})});
    ASSERT_FALSE(existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsConsideredUniqueIfOnlyPartialMatchInQmddPathWithGapsExists) {
    const auto referenceQmddPath  = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const auto comparedToQmddPath = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})});
    ASSERT_FALSE(existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsConsideredUniqueIfQmddNodeQubitsDoNotMatchButPolarityDoes) {
    const auto referenceQmddPath  = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const auto comparedToQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P})});
    ASSERT_FALSE(existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsNotConsideredUniqueIfTotalMatchWithQmddPathWithoutGapsExists) {
    const auto referenceQmddPath  = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const auto comparedToQmddPath = OptimizedQmddPath({referenceQmddPath.at(0), referenceQmddPath.at(1), referenceQmddPath.at(2)});
    ASSERT_TRUE(existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsNotConsideredUniqueIfTotalMatchWithQmddPathWithGapsExists) {
    const auto referenceQmddPath  = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const auto comparedToQmddPath = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
                                                       referenceQmddPath.at(2)});
    ASSERT_TRUE(existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsNotConsideredUniqueIfComparedToQmddPathWithoutGapsLengthDoesNotMatch) {
    const auto referenceQmddPath         = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const auto shorterComparedToQmddPath = OptimizedQmddPath({referenceQmddPath.at(0),
                                                              referenceQmddPath.at(1)});
    const auto longerComparedToQmddPath  = OptimizedQmddPath({referenceQmddPath.at(0),
                                                              referenceQmddPath.at(1),
                                                              referenceQmddPath.at(2),
                                                              QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N})});
    ASSERT_TRUE(existsQmddPathWithSameSignature(referenceQmddPath, shorterComparedToQmddPath, false));
    ASSERT_TRUE(existsQmddPathWithSameSignature(referenceQmddPath, longerComparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsNotConsideredUniqueIfComparedToQmddPathWithGapsLengthDoesNotMatch) {
    const auto referenceQmddPath         = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const auto shorterComparedToQmddPath = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 2U})});
    const auto longerComparedToQmddPath  = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 4U})});
    ASSERT_TRUE(existsQmddPathWithSameSignature(referenceQmddPath, shorterComparedToQmddPath, false));
    ASSERT_TRUE(existsQmddPathWithSameSignature(referenceQmddPath, longerComparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsNotConsideredUniqueIfComparedToQmddPathWithoutGapsMatchesQmddNodeEdgesTotally) {
    const auto referenceQmddPath  = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const auto comparedToQmddPath = OptimizedQmddPath({
            referenceQmddPath.at(0),
            referenceQmddPath.at(1),
            referenceQmddPath.at(2),
    });
    ASSERT_TRUE(existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsNotConsideredUniqueIfComparedToQmddPathWithoutGapsMatchesQmddNodeEdgesOnlyByPolarity) {
    const auto referenceQmddPath  = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const auto comparedToQmddPath = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
    });
    ASSERT_TRUE(existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsNotConsideredUniqueIfComparedToQmddPathWithGapsMatchesQmddNodeEdgesTotally) {
    const auto referenceQmddPath  = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const auto comparedToQmddPath = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    ASSERT_TRUE(existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsNotConsideredUniqueIfComparedToQmddPathWithGapsMatchesQmddNodeEdgesOnlyByPolarity) {
    const auto referenceQmddPath  = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N})});
    const auto comparedToQmddPath = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
            QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U}),
    });
    ASSERT_TRUE(existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, FirstUniqueQmddPathFoundWhenPartialMatchExistsInComparedToQmddPathCollectionWithReferencePathContainingNoGaps) {
    const auto referenceQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                      QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                      QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                      QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});

    const auto comparedToQmddPaths = std::vector({
            OptimizedQmddPath({
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
            }),
            OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                               QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 3U})}),
            OptimizedQmddPath({
                    QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 3U}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
            }),
    });

    const auto                               expectedUniqueQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const std::optional<UnoptimizedQmddPath> actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(referenceQmddPath, comparedToQmddPaths, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, FirstUniqueQmddPathFoundWhenPartialMatchExistsInComparedToQmddPathCollectionWithReferencePathContainingGaps) {
    const auto referenceQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                      QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
                                                      QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});

    const auto comparedToQmddPaths = std::vector({
            OptimizedQmddPath({
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
            }),
            OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                               QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 3U})}),
            OptimizedQmddPath({
                    QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 3U}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
            }),
    });

    const auto                               expectedUniqueQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const std::optional<UnoptimizedQmddPath> actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(referenceQmddPath, comparedToQmddPaths, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, FirstUniqueQmddPathFoundWhenComparedToQmddPathCollectionIsEmpty) {
    const auto referenceQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                      QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})});

    const auto                               expectedUniqueQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N})});
    const std::optional<UnoptimizedQmddPath> actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(referenceQmddPath, {}, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, NoUniqueQmddPathFoundWhenReferencePathIsEmpty) {
    constexpr std::optional<UnoptimizedQmddPath> expectedUniqueQmddPath = std::nullopt;
    const std::optional<UnoptimizedQmddPath>     actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(OptimizedQmddPath(), std::vector<OptimizedQmddPath>(), false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, NoUniqueQmddPathFoundWhenQmddPathCollectionContainsTotalMatchWithComparedToPathContainingGapsAndReferencePathContainingNoGaps) {
    const auto referenceQmddPath = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
    });

    const auto comparedToQmddPaths = std::vector({OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                     QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})})});

    constexpr std::optional<UnoptimizedQmddPath> expectedUniqueQmddPath = std::nullopt;
    const std::optional<UnoptimizedQmddPath>     actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(OptimizedQmddPath(), comparedToQmddPaths, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, NoUniqueQmddPathFoundWhenQmddPathCollectionContainsTotalMatchWithComparedToPathContainingNoGapsAndReferencePathContainingNoGaps) {
    const auto referenceQmddPath   = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
    });
    const auto comparedToQmddPaths = std::vector({referenceQmddPath});

    constexpr std::optional<UnoptimizedQmddPath> expectedUniqueQmddPath = std::nullopt;
    const std::optional<UnoptimizedQmddPath>     actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(OptimizedQmddPath(), comparedToQmddPaths, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, NoUniqueQmddPathFoundWhenQmddPathCollectionContainsTotalMatchWithComparedToPathContainingGapsAndReferencePathContainingGaps) {
    const auto                                   referenceQmddPath      = OptimizedQmddPath({
            QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
    });
    const auto                                   comparedToQmddPaths    = std::vector({OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})}),
                                                                                       OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})})});
    constexpr std::optional<UnoptimizedQmddPath> expectedUniqueQmddPath = std::nullopt;
    const std::optional<UnoptimizedQmddPath>     actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(OptimizedQmddPath(), comparedToQmddPaths, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, NoUniqueQmddPathFoundWhenQmddPathCollectionContainsTotalMatchWithComparedToPathContainingNoGapsAndReferencePathContainingGaps) {
    const auto                                   referenceQmddPath      = OptimizedQmddPath({
            QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
    });
    const auto                                   comparedToQmddPaths    = std::vector({
            OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})}),
            OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})}),
            OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})}),
            OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})}),
    });
    constexpr std::optional<UnoptimizedQmddPath> expectedUniqueQmddPath = std::nullopt;
    const std::optional<UnoptimizedQmddPath>     actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(OptimizedQmddPath(), comparedToQmddPaths, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, NoUniqueQmddPathFoundWhenLengthOfComparedToPathInCollectionIsSmallerThanReferenceOne) {
    const auto                                   referenceQmddPath                       = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
    });
    constexpr std::optional<UnoptimizedQmddPath> expectedUniqueQmddPath                  = std::nullopt;
    const auto                                   qmddPathCollectionContainingPathWithGap = std::vector({OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 0U, .nConsecutiveQubitInGap = 1U})})});
    std::optional<UnoptimizedQmddPath>           actualUniqueQmddPath                    = findFirstQmddPathWithUniqueSignature(referenceQmddPath, qmddPathCollectionContainingPathWithGap, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);

    const auto qmddPathCollectionContainingPathWithoutGap = std::vector({OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P})})});
    actualUniqueQmddPath                                  = findFirstQmddPathWithUniqueSignature(referenceQmddPath, qmddPathCollectionContainingPathWithoutGap, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, NoUniqueQmddPathFoundWhenLengthOfComparedToPathInCollectionIsLongerThanReferenceOne) {
    const auto                                   referenceQmddPath                       = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
    });
    constexpr std::optional<UnoptimizedQmddPath> expectedUniqueQmddPath                  = std::nullopt;
    const auto                                   qmddPathCollectionContainingPathWithGap = std::vector({OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 3U})})});
    std::optional<UnoptimizedQmddPath>           actualUniqueQmddPath                    = findFirstQmddPathWithUniqueSignature(referenceQmddPath, qmddPathCollectionContainingPathWithGap, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);

    const auto qmddPathCollectionContainingPathWithoutGap = std::vector({OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})})});
    actualUniqueQmddPath                                  = findFirstQmddPathWithUniqueSignature(OptimizedQmddPath(), qmddPathCollectionContainingPathWithoutGap, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, NoUniqueQmddPathFoundInReferenceCollectionIfTotalMatchInComparedToCollectionExistsWithMatchContainingNoGaps) {
    const auto referenceQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                      QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})});

    const auto                                   comparedToQmddPaths    = std::vector({OptimizedQmddPath({
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                  }),
                                                                                       OptimizedQmddPath({
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                  }),
                                                                                       OptimizedQmddPath({
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                  }),
                                                                                       OptimizedQmddPath({
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                  })});
    constexpr std::optional<UnoptimizedQmddPath> expectedUniqueQmddPath = std::nullopt;
    const std::optional<UnoptimizedQmddPath>     actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(std::vector({referenceQmddPath}), comparedToQmddPaths, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, NoUniqueQmddPathFoundInReferenceCollectionIfTotalMatchInComparedToCollectionExistsWithMatchContainingGaps) {
    const auto referenceQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                      QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})});

    const auto                                   comparedToQmddPaths    = std::vector({OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 3U})})});
    constexpr std::optional<UnoptimizedQmddPath> expectedUniqueQmddPath = std::nullopt;
    const std::optional<UnoptimizedQmddPath>     actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(std::vector({referenceQmddPath}), comparedToQmddPaths, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, NoUniqueQmddPathFoundInReferenceCollectionIfReferenceCollectionIsEmpty) {
    constexpr std::optional<UnoptimizedQmddPath> expectedUniqueQmddPath = std::nullopt;
    const std::optional<UnoptimizedQmddPath>     actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(std::vector<OptimizedQmddPath>(), std::vector<OptimizedQmddPath>(), false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, UniqueQmddPathFoundInReferenceCollectionIfComparedToCollectionIsEmpty) {
    const auto                               referenceQmddPath      = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
    });
    const auto                               expectedUniqueQmddPath = UnoptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
    });
    const std::optional<UnoptimizedQmddPath> actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(std::vector({referenceQmddPath}), {}, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, UniquePathFoundInReferenceCollectionIfReferencePathContainedNoGaps) {
    const auto referenceQmddPath = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
    });

    const auto comparedToQmddPath = std::vector({OptimizedQmddPath({
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                 }),
                                                 OptimizedQmddPath({
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                 })});

    const auto                               expectedUniqueQmddPath = UnoptimizedQmddPath({std::get<QmddPathComponent>(referenceQmddPath.at(0)),
                                                                                           std::get<QmddPathComponent>(referenceQmddPath.at(1))});
    const std::optional<UnoptimizedQmddPath> actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(std::vector({referenceQmddPath}), comparedToQmddPath, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, UniquePathFoundInReferenceCollectionIfReferencePathContainedGaps) {
    const auto referenceQmddPath = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
    });

    const auto comparedToQmddPath = std::vector({
            OptimizedQmddPath({
                    QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 3U}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
            }),
            OptimizedQmddPath({
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
            }),
            OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                               QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
                               QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N})}),
    });

    const auto                               expectedUniqueQmddPath = UnoptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
    });
    const std::optional<UnoptimizedQmddPath> actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(std::vector({referenceQmddPath}), comparedToQmddPath, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, UniquePathFoundInReferenceCollectionIfPreviousEntriesWereNotUniqueWithReferencePathContainingNoGaps) {
    const auto nonUniqueReferenceQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                               QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                               QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N})});

    const auto uniqueReferenceQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})});

    const auto comparedToQmddPath = std::vector({OptimizedQmddPath({
                                                         QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                 }),
                                                 OptimizedQmddPath({
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                 })});

    const auto                               expectedUniqueQmddPath = UnoptimizedQmddPath({std::get<QmddPathComponent>(uniqueReferenceQmddPath.at(0)),
                                                                                           std::get<QmddPathComponent>(uniqueReferenceQmddPath.at(1)),
                                                                                           std::get<QmddPathComponent>(uniqueReferenceQmddPath.at(2))});
    const std::optional<UnoptimizedQmddPath> actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(std::vector({nonUniqueReferenceQmddPath, uniqueReferenceQmddPath}), comparedToQmddPath, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, UniquePathFoundInReferenceCollectionIfPreviousEntriesWereNotUniqueWithReferencePathContainingGaps) {
    const auto nonUniqueReferenceQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                               QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                               QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N})});

    const auto uniqueReferenceQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                            QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})});

    const auto comparedToQmddPath = std::vector({OptimizedQmddPath({
                                                         QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                 }),
                                                 OptimizedQmddPath({
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                 })});

    const auto                               expectedUniqueQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})});
    const std::optional<UnoptimizedQmddPath> actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(std::vector({nonUniqueReferenceQmddPath, uniqueReferenceQmddPath}), comparedToQmddPath, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, FindOperandsToMakeQmddPathUniqueSuccessfulWithReferenceCollectionBeingEmpty) {
    const auto                                              referenceQmddPath                    = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
    });
    const std::optional<ToUniqueQmddPathSignatureOperands>  expectedOperandsToTurnQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, {}, false);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

// TODO: Tests for already unique qmdd path being passed as input parameter to search?
// TODO: Tests for search that would require more than one bit flip to turn path unique?
// TODO: getOperandsToMakeQmddPathSignatureUnique will only consider paths that contain more than one entry
// TODO: Usages of getOperandsToMakeQmddPathSignatureUnique in qmdd transformations assumes that qmdd paths share qmdd path origin?

TEST(QmddPathOperationTests, FindOperandsToMakeQmddPathUniqueSuccessfulForQmddPathContainingNoGaps) {
    const auto referenceQmddPath = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
    });

    const auto                                              comparedToQmddPaths                  = std::vector({OptimizedQmddPath({
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                  }),
                                                                                                                OptimizedQmddPath({
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                  })});
    const auto                                              expectedOperandsToTurnQmddPathUnique = ToUniqueQmddPathSignatureOperands({.controlQubitsFromFirstNodeInPathToTargetQubit = qc::Controls({qc::Control(1U, qc::Control::Type::Neg)}),
                                                                                                                                      .targetQubit                                   = 0U});
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths, false);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, FindOperandsToMakeQmddPathUniqueSuccessfulForQmddPathContainingGaps) {
    const auto referenceQmddPath = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
    });

    const auto                                              comparedToQmddPaths                  = std::vector({OptimizedQmddPath({
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                  }),
                                                                                                                OptimizedQmddPath({
                                                          QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 2U}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                  }),
                                                                                                                OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 2U}),
                                                                                                                                   QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                                                                   QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 0U, .nConsecutiveQubitInGap = 1U})})});
    const auto                                              expectedOperandsToTurnQmddPathUnique = ToUniqueQmddPathSignatureOperands({.controlQubitsFromFirstNodeInPathToTargetQubit = qc::Controls({qc::Control(3U, qc::Control::Type::Neg),
                                                                                                                                                                                                     qc::Control(2U, qc::Control::Type::Neg)}),
                                                                                                                                      .targetQubit                                   = 1U});
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths, false);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, FindOperandsToMakeQmddPathUniqueSuccessfulForReferencePathContainingNoGapsAndHasASingleEntry) {
    const auto referenceQmddPath = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
    });

    const auto                                              comparedToQmddPaths                  = std::vector({OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})})});
    const auto                                              expectedOperandsToTurnQmddPathUnique = ToUniqueQmddPathSignatureOperands({.controlQubitsFromFirstNodeInPathToTargetQubit = qc::Controls(), .targetQubit = 0U});
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths, false);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, FindOperandsToMakeQmddPathUniqueSuccessfulForReferencePathContainingGapAndHasASingleEntry) {
    const auto referenceQmddPath = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 0U, .nConsecutiveQubitInGap = 1U})});

    const auto                                              comparedToQmddPaths                  = std::vector({OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N})})});
    const auto                                              expectedOperandsToTurnQmddPathUnique = ToUniqueQmddPathSignatureOperands({.controlQubitsFromFirstNodeInPathToTargetQubit = qc::Controls(), .targetQubit = 0U});
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths, false);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, FindOperandsToMakeQmddPathUniqueNotSuccessfulIfReferenceQmddPathIsEmpty) {
    const auto                                              comparedToQmddPaths                  = std::vector({OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})})});
    const std::optional<ToUniqueQmddPathSignatureOperands>  expectedOperandsToTurnQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique({}, comparedToQmddPaths, false);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, FindOperandsToMakeQmddPathUniqueNotSuccessfulIfQmddPathWithSameSignatureExistsInComparedToQmddPathCollectionForAllSingleChangeRefPathCombinationsWithMatchesContainingNoGaps) {
    const auto referenceQmddPath = OptimizedQmddPath({
            QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 2U}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
    });

    const auto                                              comparedToQmddPaths                  = std::vector({
            OptimizedQmddPath({
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            }),
            OptimizedQmddPath({
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            }),
            OptimizedQmddPath({
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            }),
            OptimizedQmddPath({
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                    QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            }),
    });
    const std::optional<ToUniqueQmddPathSignatureOperands>  expectedOperandsToTurnQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths, false);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, FindOperandsToMakeQmddPathUniqueNotSuccessfulIfQmddPathWithSameSignatureExistsInComparedToQmddPathCollectionForAllSingleChangeRefPathCombinationsWithMatchesContainingGaps) {
    const auto referenceQmddPath = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
    });

    const auto                                              comparedToQmddPaths                  = std::vector({OptimizedQmddPath({
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                          QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 1U}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                  }),
                                                                                                                OptimizedQmddPath({
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                          QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                  })});
    const std::optional<ToUniqueQmddPathSignatureOperands>  expectedOperandsToTurnQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths, false);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}
