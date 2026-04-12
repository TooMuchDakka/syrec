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

    void assertQmddPathSignatureComparisonResultMatch(const std::optional<bool> expectedSignatureMatchResult, const std::optional<bool> actualSignatureMatchResult) {
        if (expectedSignatureMatchResult.has_value()) {
            ASSERT_TRUE(actualSignatureMatchResult.has_value()) << "Expected qmdd path signature comparison result to be known!";
            ASSERT_EQ(*expectedSignatureMatchResult, *actualSignatureMatchResult);
        } else {
            ASSERT_FALSE(actualSignatureMatchResult.has_value()) << "Expected qmdd path signature comparison result to be unknown!";
        }
    }

    void assertExistsQmddPathWithSameSignatureComparisonResultMatch(const std::optional<bool> expectedExistsQmddPathResult, const std::optional<bool> actualExistsQmddPathResult) {
        if (expectedExistsQmddPathResult.has_value()) {
            ASSERT_TRUE(actualExistsQmddPathResult.has_value()) << "Expected result for whether qmdd path for signature exists to be known!";
            ASSERT_EQ(*expectedExistsQmddPathResult, *actualExistsQmddPathResult);
        } else {
            ASSERT_FALSE(actualExistsQmddPathResult.has_value()) << "Expected result for whether qmdd path for signature exists to be unknown!";
        }
    }

    OptimizedQmddPath createOptimizedQmddPathWithoutGaps(const std::initializer_list<std::pair<dd::Qubit, QmddNodeEdge>>& qmddPathComponents) {
        OptimizedQmddPath generatedPath;
        generatedPath.reserve(qmddPathComponents.size());
        std::ranges::transform(qmddPathComponents, std::back_inserter(generatedPath), [](const std::pair<dd::Qubit, QmddNodeEdge>& qmddPathComponentData) {
            return QmddPathComponent({.qubitAssociatedWithQmddNode = qmddPathComponentData.first, .qmddEdgeToChildNode = qmddPathComponentData.second});
        });
        return generatedPath;
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
    const OptimizedQmddPath referenceQmddPath = createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)});
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
    const OptimizedQmddPath referenceQmddPath = createOptimizedQmddPathWithoutGaps({std::make_pair(3U, QmddNodeEdge::N),
                                                                                    std::make_pair(2U, QmddNodeEdge::N),
                                                                                    std::make_pair(1U, QmddNodeEdge::N),
                                                                                    std::make_pair(0U, QmddNodeEdge::N)});
    ASSERT_EQ(1U, getNumberOfPathsToOneTerminalForQmddPath(referenceQmddPath));
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
    auto                    qmddPathCollection  = std::vector<OptimizedQmddPath>();
    const OptimizedQmddPath qmddPathWithoutGaps = createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::N),
                                                                                      std::make_pair(1U, QmddNodeEdge::P),
                                                                                      std::make_pair(0U, QmddNodeEdge::NPrime)});
    const auto              emptyQmddPath       = OptimizedQmddPath();
    const auto              qmddPathWithGaps    = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
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

    assertQmddPathSignatureComparisonResultMatch(true, doQmddPathSignaturesMatch(referenceQmddPath, referenceQmddPath, false));
    assertQmddPathSignatureComparisonResultMatch(true, doQmddPathSignaturesMatch(referenceQmddPath, qmddPathWithQmddEdgesOfSamePolarity, false));
}

TEST(QmddPathOperationTests, CheckQmddPathsDoNotMatchIfPathLengthsDoNotMatch) {
    const auto referenceQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                        QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                        QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})});

    const auto truncatedReferenceQmddPath = UnoptimizedQmddPath({referenceQmddPath.at(0), referenceQmddPath.at(1)});

    assertQmddPathSignatureComparisonResultMatch(std::nullopt, doQmddPathSignaturesMatch(referenceQmddPath, truncatedReferenceQmddPath, false));
    assertQmddPathSignatureComparisonResultMatch(std::nullopt, doQmddPathSignaturesMatch(truncatedReferenceQmddPath, referenceQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathsDoNotMatchIfQubitsOfQmddNodeDoNotMatch) {
    const auto referenceQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                        QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})});

    const auto comparedToQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P})});
    assertQmddPathSignatureComparisonResultMatch(std::nullopt, doQmddPathSignaturesMatch(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathsDoNotMatchIfSignatureOfQmddEdgesDoNotMatch) {
    const auto referenceQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                        QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})});

    const auto comparedToQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N})});
    assertQmddPathSignatureComparisonResultMatch(false, doQmddPathSignaturesMatch(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathsMatchIfBothAreEmpty) {
    assertQmddPathSignatureComparisonResultMatch(true, doQmddPathSignaturesMatch({}, {}, false));
}

TEST(QmddPathOperationTests, CheckQmddPathDoNotMatchIfOneIfEmpty) {
    const auto referenceQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})});
    assertQmddPathSignatureComparisonResultMatch(std::nullopt, doQmddPathSignaturesMatch(referenceQmddPath, {}, false));
    assertQmddPathSignatureComparisonResultMatch(std::nullopt, doQmddPathSignaturesMatch({}, referenceQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsConsideredUniqueIfOnlyPartialMatchInQmddPathWithoutGapsExists) {
    const auto referenceQmddPath  = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const auto comparedToQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})});
    assertExistsQmddPathWithSameSignatureComparisonResultMatch(false, existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsConsideredUniqueIfOnlyPartialMatchInQmddPathWithGapsExists) {
    const auto referenceQmddPath  = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const auto comparedToQmddPath = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})});
    assertExistsQmddPathWithSameSignatureComparisonResultMatch(false, existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsConsideredUniqueIfQmddNodeQubitsDoNotMatchButPolarityDoes) {
    const auto referenceQmddPath  = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const auto comparedToQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P})});
    assertExistsQmddPathWithSameSignatureComparisonResultMatch(std::nullopt, existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsNotConsideredUniqueIfTotalMatchWithQmddPathWithoutGapsExists) {
    const auto referenceQmddPath  = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const auto comparedToQmddPath = OptimizedQmddPath({referenceQmddPath.at(0), referenceQmddPath.at(1), referenceQmddPath.at(2)});
    assertExistsQmddPathWithSameSignatureComparisonResultMatch(true, existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsNotConsideredUniqueIfTotalMatchWithQmddPathWithGapsExists) {
    const auto referenceQmddPath  = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const auto comparedToQmddPath = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
                                                       referenceQmddPath.at(2)});
    assertExistsQmddPathWithSameSignatureComparisonResultMatch(true, existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
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

    assertExistsQmddPathWithSameSignatureComparisonResultMatch(std::nullopt, existsQmddPathWithSameSignature(referenceQmddPath, shorterComparedToQmddPath, false));
    assertExistsQmddPathWithSameSignatureComparisonResultMatch(std::nullopt, existsQmddPathWithSameSignature(referenceQmddPath, longerComparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsNotConsideredUniqueIfComparedToQmddPathWithGapsLengthDoesNotMatch) {
    const auto referenceQmddPath         = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const auto shorterComparedToQmddPath = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 2U})});
    const auto longerComparedToQmddPath  = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 4U})});

    assertExistsQmddPathWithSameSignatureComparisonResultMatch(std::nullopt, existsQmddPathWithSameSignature(referenceQmddPath, shorterComparedToQmddPath, false));
    assertExistsQmddPathWithSameSignatureComparisonResultMatch(std::nullopt, existsQmddPathWithSameSignature(referenceQmddPath, longerComparedToQmddPath, false));
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

    assertExistsQmddPathWithSameSignatureComparisonResultMatch(true, existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
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
    assertExistsQmddPathWithSameSignatureComparisonResultMatch(true, existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsNotConsideredUniqueIfComparedToQmddPathWithGapsMatchesQmddNodeEdgesTotally) {
    const auto referenceQmddPath  = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    const auto comparedToQmddPath = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime})});
    assertExistsQmddPathWithSameSignatureComparisonResultMatch(true, existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, CheckQmddPathIsNotConsideredUniqueIfComparedToQmddPathWithGapsMatchesQmddNodeEdgesOnlyByPolarity) {
    const auto referenceQmddPath  = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N})});
    const auto comparedToQmddPath = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
            QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U}),
    });
    assertExistsQmddPathWithSameSignatureComparisonResultMatch(true, existsQmddPathWithSameSignature(referenceQmddPath, comparedToQmddPath, false));
}

TEST(QmddPathOperationTests, FirstUniqueQmddPathFoundWhenPartialMatchExistsInComparedToQmddPathCollectionWithReferencePathContainingNoGaps) {
    const OptimizedQmddPath referenceQmddPath   = createOptimizedQmddPathWithoutGaps({std::make_pair(3U, QmddNodeEdge::N),
                                                                                      std::make_pair(2U, QmddNodeEdge::NPrime),
                                                                                      std::make_pair(1U, QmddNodeEdge::PPrime),
                                                                                      std::make_pair(0U, QmddNodeEdge::PPrime)});
    const auto              comparedToQmddPaths = std::vector({
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
            createOptimizedQmddPathWithoutGaps({std::make_pair(3U, QmddNodeEdge::P),
                                                std::make_pair(2U, QmddNodeEdge::N),
                                                std::make_pair(1U, QmddNodeEdge::N),
                                                std::make_pair(0U, QmddNodeEdge::P)}),
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
    const OptimizedQmddPath referenceQmddPath   = createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::N),
                                                                                      std::make_pair(1U, QmddNodeEdge::N),
                                                                                      std::make_pair(0U, QmddNodeEdge::P)});
    const auto              comparedToQmddPaths = std::vector({OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                  QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})})});

    constexpr std::optional<UnoptimizedQmddPath> expectedUniqueQmddPath = std::nullopt;
    const std::optional<UnoptimizedQmddPath>     actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(OptimizedQmddPath(), comparedToQmddPaths, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, NoUniqueQmddPathFoundWhenQmddPathCollectionContainsTotalMatchWithComparedToPathContainingNoGapsAndReferencePathContainingNoGaps) {
    const OptimizedQmddPath referenceQmddPath   = createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::N),
                                                                                      std::make_pair(1U, QmddNodeEdge::N),
                                                                                      std::make_pair(0U, QmddNodeEdge::P)});
    const auto              comparedToQmddPaths = std::vector({referenceQmddPath});

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
    const auto referenceQmddPath = OptimizedQmddPath({
            QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
    });

    const auto                                   comparedToQmddPaths    = std::vector({createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::N),
                                                                                                                           std::make_pair(1U, QmddNodeEdge::N),
                                                                                                                           std::make_pair(0U, QmddNodeEdge::P)}),
                                                                                       createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::N),
                                                                                                                           std::make_pair(1U, QmddNodeEdge::P),
                                                                                                                           std::make_pair(0U, QmddNodeEdge::P)}),
                                                                                       createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P),
                                                                                                                           std::make_pair(1U, QmddNodeEdge::N),
                                                                                                                           std::make_pair(0U, QmddNodeEdge::P)}),
                                                                                       createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P),
                                                                                                                           std::make_pair(1U, QmddNodeEdge::P),
                                                                                                                           std::make_pair(0U, QmddNodeEdge::P)})});
    constexpr std::optional<UnoptimizedQmddPath> expectedUniqueQmddPath = std::nullopt;
    const std::optional<UnoptimizedQmddPath>     actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(OptimizedQmddPath(), comparedToQmddPaths, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, NoUniqueQmddPathFoundWhenLengthOfComparedToPathInCollectionIsSmallerThanReferenceOne) {
    const OptimizedQmddPath                      referenceQmddPath                       = createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::N),
                                                                                                                               std::make_pair(0U, QmddNodeEdge::PPrime)});
    constexpr std::optional<UnoptimizedQmddPath> expectedUniqueQmddPath                  = std::nullopt;
    const auto                                   qmddPathCollectionContainingPathWithGap = std::vector({OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 0U, .nConsecutiveQubitInGap = 1U})})});
    std::optional<UnoptimizedQmddPath>           actualUniqueQmddPath                    = findFirstQmddPathWithUniqueSignature(referenceQmddPath, qmddPathCollectionContainingPathWithGap, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);

    const auto qmddPathCollectionContainingPathWithoutGap = std::vector({OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P})})});
    actualUniqueQmddPath                                  = findFirstQmddPathWithUniqueSignature(referenceQmddPath, qmddPathCollectionContainingPathWithoutGap, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, NoUniqueQmddPathFoundWhenLengthOfComparedToPathInCollectionIsLongerThanReferenceOne) {
    const OptimizedQmddPath                      referenceQmddPath                       = createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::N),
                                                                                                                               std::make_pair(0U, QmddNodeEdge::PPrime)});
    constexpr std::optional<UnoptimizedQmddPath> expectedUniqueQmddPath                  = std::nullopt;
    const auto                                   qmddPathCollectionContainingPathWithGap = std::vector({OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 3U})})});
    std::optional<UnoptimizedQmddPath>           actualUniqueQmddPath                    = findFirstQmddPathWithUniqueSignature(referenceQmddPath, qmddPathCollectionContainingPathWithGap, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);

    const auto qmddPathCollectionContainingPathWithoutGap = std::vector({createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P),
                                                                                                             std::make_pair(1U, QmddNodeEdge::P),
                                                                                                             std::make_pair(0U, QmddNodeEdge::P)})});
    actualUniqueQmddPath                                  = findFirstQmddPathWithUniqueSignature(OptimizedQmddPath(), qmddPathCollectionContainingPathWithoutGap, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, NoUniqueQmddPathFoundInReferenceCollectionIfTotalMatchInComparedToCollectionExistsWithMatchContainingNoGaps) {
    const auto referenceQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                      QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})});

    const auto                                   comparedToQmddPaths    = std::vector({
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::NPrime),
                                                                                     std::make_pair(1U, QmddNodeEdge::N),
                                                                                     std::make_pair(0U, QmddNodeEdge::N)}),
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::NPrime),
                                                                                     std::make_pair(1U, QmddNodeEdge::N),
                                                                                     std::make_pair(0U, QmddNodeEdge::P)}),
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::NPrime),
                                                                                     std::make_pair(1U, QmddNodeEdge::P),
                                                                                     std::make_pair(0U, QmddNodeEdge::N)}),
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::NPrime),
                                                                                     std::make_pair(1U, QmddNodeEdge::P),
                                                                                     std::make_pair(0U, QmddNodeEdge::P)}),
    });
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
    const OptimizedQmddPath                  referenceQmddPath      = createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::N),
                                                                                                          std::make_pair(0U, QmddNodeEdge::PPrime)});
    const auto                               expectedUniqueQmddPath = UnoptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
    });
    const std::optional<UnoptimizedQmddPath> actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(std::vector({referenceQmddPath}), {}, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, UniquePathFoundInReferenceCollectionIfReferencePathContainedNoGaps) {
    const OptimizedQmddPath                  referenceQmddPath      = createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::N),
                                                                                                          std::make_pair(0U, QmddNodeEdge::PPrime)});
    const auto                               comparedToQmddPaths    = std::vector({createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::P),
                                                                                                                       std::make_pair(0U, QmddNodeEdge::PPrime)}),
                                                                                   createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::NPrime),
                                                                                                                       std::make_pair(0U, QmddNodeEdge::NPrime)})});
    const auto                               expectedUniqueQmddPath = UnoptimizedQmddPath({std::get<QmddPathComponent>(referenceQmddPath.at(0)),
                                                                                           std::get<QmddPathComponent>(referenceQmddPath.at(1))});
    const std::optional<UnoptimizedQmddPath> actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(std::vector({referenceQmddPath}), comparedToQmddPaths, false);
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
            createOptimizedQmddPathWithoutGaps({std::make_pair(3U, QmddNodeEdge::NPrime),
                                                std::make_pair(2U, QmddNodeEdge::NPrime),
                                                std::make_pair(1U, QmddNodeEdge::NPrime),
                                                std::make_pair(0U, QmddNodeEdge::NPrime)}),
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
    const OptimizedQmddPath nonUniqueReferenceQmddPath = createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::N),
                                                                                             std::make_pair(1U, QmddNodeEdge::N),
                                                                                             std::make_pair(0U, QmddNodeEdge::N)});

    const OptimizedQmddPath uniqueReferenceQmddPath = createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P),
                                                                                          std::make_pair(1U, QmddNodeEdge::P),
                                                                                          std::make_pair(0U, QmddNodeEdge::P)});
    const auto              comparedToQmddPath      = std::vector({OptimizedQmddPath({
                                                         QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                 }),
                                                                   createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::NPrime),
                                                                                                       std::make_pair(1U, QmddNodeEdge::P),
                                                                                                       std::make_pair(0U, QmddNodeEdge::NPrime)})});

    const auto                               expectedUniqueQmddPath = UnoptimizedQmddPath({std::get<QmddPathComponent>(uniqueReferenceQmddPath.at(0)),
                                                                                           std::get<QmddPathComponent>(uniqueReferenceQmddPath.at(1)),
                                                                                           std::get<QmddPathComponent>(uniqueReferenceQmddPath.at(2))});
    const std::optional<UnoptimizedQmddPath> actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(std::vector({nonUniqueReferenceQmddPath, uniqueReferenceQmddPath}), comparedToQmddPath, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, UniquePathFoundInReferenceCollectionIfPreviousEntriesWereNotUniqueWithReferencePathContainingGaps) {
    const OptimizedQmddPath nonUniqueReferenceQmddPath = createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::N),
                                                                                             std::make_pair(1U, QmddNodeEdge::N),
                                                                                             std::make_pair(0U, QmddNodeEdge::N)});

    const auto uniqueReferenceQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                            QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})});

    const auto comparedToQmddPath = std::vector({OptimizedQmddPath({
                                                         QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
                                                         QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                 }),
                                                 createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::NPrime),
                                                                                     std::make_pair(1U, QmddNodeEdge::P),
                                                                                     std::make_pair(0U, QmddNodeEdge::NPrime)})});

    const auto                               expectedUniqueQmddPath = UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                           QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})});
    const std::optional<UnoptimizedQmddPath> actualUniqueQmddPath   = findFirstQmddPathWithUniqueSignature(std::vector({nonUniqueReferenceQmddPath, uniqueReferenceQmddPath}), comparedToQmddPath, false);
    assertUnoptimizedQmddPathsMatch(expectedUniqueQmddPath, actualUniqueQmddPath);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueSuccessfulWithReferenceCollectionBeingEmpty) {
    const OptimizedQmddPath                                 referenceQmddPath                    = createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::N),
                                                                                                                                       std::make_pair(0U, QmddNodeEdge::P)});
    const std::optional<ToUniqueQmddPathSignatureOperands>  expectedOperandsToTurnQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, {});
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueSuccessfulForQmddPathContainingNoGaps) {
    const OptimizedQmddPath referenceQmddPath = createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::N), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)});

    const auto                                              comparedToQmddPaths                  = std::vector({createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::PPrime), std::make_pair(1U, QmddNodeEdge::NPrime), std::make_pair(0U, QmddNodeEdge::N)}),
                                                                                                                createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::PPrime), std::make_pair(1U, QmddNodeEdge::NPrime), std::make_pair(0U, QmddNodeEdge::P)})});
    const auto                                              expectedOperandsToTurnQmddPathUnique = ToUniqueQmddPathSignatureOperands({.controlQubitsFromFirstNodeInPathToTargetQubit = qc::Controls({qc::Control(2U, qc::Control::Type::Neg)}),
                                                                                                                                      .targetQubit                                   = 1U});
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueSuccessfulForQmddPathStartingAtNEdge) {
    const OptimizedQmddPath referenceQmddPath = createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::N), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)});

    const auto                                              comparedToQmddPaths                  = std::vector({
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::PPrime), std::make_pair(1U, QmddNodeEdge::NPrime), std::make_pair(0U, QmddNodeEdge::N)}),
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::PPrime), std::make_pair(1U, QmddNodeEdge::NPrime), std::make_pair(0U, QmddNodeEdge::P)}),
    });
    const auto                                              expectedOperandsToTurnQmddPathUnique = ToUniqueQmddPathSignatureOperands({.controlQubitsFromFirstNodeInPathToTargetQubit = qc::Controls({qc::Control(2U, qc::Control::Type::Neg)}),
                                                                                                                                      .targetQubit                                   = 1U});
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueSuccessfulForQmddPathStartingAtNPrimeEdge) {
    const OptimizedQmddPath referenceQmddPath = createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::NPrime), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)});

    const auto                                              comparedToQmddPaths                  = std::vector({
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::PPrime), std::make_pair(1U, QmddNodeEdge::NPrime), std::make_pair(0U, QmddNodeEdge::N)}),
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::PPrime), std::make_pair(1U, QmddNodeEdge::NPrime), std::make_pair(0U, QmddNodeEdge::P)}),
    });
    const auto                                              expectedOperandsToTurnQmddPathUnique = ToUniqueQmddPathSignatureOperands({.controlQubitsFromFirstNodeInPathToTargetQubit = qc::Controls({qc::Control(2U, qc::Control::Type::Neg)}),
                                                                                                                                      .targetQubit                                   = 1U});
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueSuccessfulForQmddPathStartingAtPEdge) {
    const OptimizedQmddPath referenceQmddPath = createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)});

    const auto                                              comparedToQmddPaths                  = std::vector({
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::NPrime), std::make_pair(1U, QmddNodeEdge::NPrime), std::make_pair(0U, QmddNodeEdge::N)}),
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::NPrime), std::make_pair(1U, QmddNodeEdge::NPrime), std::make_pair(0U, QmddNodeEdge::P)}),
    });
    const auto                                              expectedOperandsToTurnQmddPathUnique = ToUniqueQmddPathSignatureOperands({.controlQubitsFromFirstNodeInPathToTargetQubit = qc::Controls({qc::Control(2U, qc::Control::Type::Pos)}),
                                                                                                                                      .targetQubit                                   = 1U});
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueSuccessfulForQmddPathStartingAtPPrimeEdge) {
    const OptimizedQmddPath referenceQmddPath = createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::PPrime), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)});

    const auto                                              comparedToQmddPaths                  = std::vector({
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::N), std::make_pair(1U, QmddNodeEdge::NPrime), std::make_pair(0U, QmddNodeEdge::N)}),
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::N), std::make_pair(1U, QmddNodeEdge::NPrime), std::make_pair(0U, QmddNodeEdge::P)}),
    });
    const auto                                              expectedOperandsToTurnQmddPathUnique = ToUniqueQmddPathSignatureOperands({.controlQubitsFromFirstNodeInPathToTargetQubit = qc::Controls({qc::Control(2U, qc::Control::Type::Pos)}),
                                                                                                                                      .targetQubit                                   = 1U});
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueSuccessfulForQmddPathRequiringMultipleControlQubitsToReachTargetQubit) {
    const OptimizedQmddPath referenceQmddPath = createOptimizedQmddPathWithoutGaps({std::make_pair(4U, QmddNodeEdge::N), std::make_pair(3U, QmddNodeEdge::P), std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)});

    const auto                                              comparedToQmddPaths                  = std::vector({createOptimizedQmddPathWithoutGaps({std::make_pair(4U, QmddNodeEdge::P), std::make_pair(3U, QmddNodeEdge::P), std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)}),
                                                                                                                createOptimizedQmddPathWithoutGaps({std::make_pair(4U, QmddNodeEdge::P), std::make_pair(3U, QmddNodeEdge::N), std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)}),
                                                                                                                createOptimizedQmddPathWithoutGaps({std::make_pair(4U, QmddNodeEdge::P), std::make_pair(3U, QmddNodeEdge::P), std::make_pair(2U, QmddNodeEdge::N), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)}),
                                                                                                                createOptimizedQmddPathWithoutGaps({std::make_pair(4U, QmddNodeEdge::P), std::make_pair(3U, QmddNodeEdge::P), std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::P), std::make_pair(0U, QmddNodeEdge::P)})});
    const auto                                              expectedOperandsToTurnQmddPathUnique = ToUniqueQmddPathSignatureOperands({.controlQubitsFromFirstNodeInPathToTargetQubit = qc::Controls({qc::Control(4U, qc::Control::Type::Neg),
                                                                                                                                                                                                     qc::Control(3U, qc::Control::Type::Pos),
                                                                                                                                                                                                     qc::Control(2U, qc::Control::Type::Pos),
                                                                                                                                                                                                     qc::Control(1U, qc::Control::Type::Neg)}),
                                                                                                                                      .targetQubit                                   = 0U});
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueSuccessfulForQmddPathContainingGaps) {
    const auto referenceQmddPath = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
    });

    const auto                                              comparedToQmddPaths                  = std::vector({createOptimizedQmddPathWithoutGaps({std::make_pair(3U, QmddNodeEdge::NPrime),
                                                                                                                                                    std::make_pair(2U, QmddNodeEdge::NPrime),
                                                                                                                                                    std::make_pair(1U, QmddNodeEdge::NPrime),
                                                                                                                                                    std::make_pair(0U, QmddNodeEdge::PPrime)}),
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
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueNotSuccessfulIfReferenceQmddPathIsEmpty) {
    const auto                                              comparedToQmddPaths                  = std::vector({OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})})});
    const std::optional<ToUniqueQmddPathSignatureOperands>  expectedOperandsToTurnQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique({}, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueNotSuccessfulIfQmddPathWithSameSignatureExistsInComparedToQmddPathCollectionForAllSingleChangeRefPathCombinationsWithMatchesContainingNoGaps) {
    const auto referenceQmddPath = OptimizedQmddPath({
            QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 2U}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
    });

    const auto                                              comparedToQmddPaths                  = std::vector({createOptimizedQmddPathWithoutGaps({std::make_pair(3U, QmddNodeEdge::P),
                                                                                                                                                    std::make_pair(2U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(1U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(0U, QmddNodeEdge::P)}),

                                                                                                                createOptimizedQmddPathWithoutGaps({std::make_pair(3U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(2U, QmddNodeEdge::P),
                                                                                                                                                    std::make_pair(1U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(0U, QmddNodeEdge::P)}),
                                                                                                                createOptimizedQmddPathWithoutGaps({std::make_pair(3U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(2U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(1U, QmddNodeEdge::P),
                                                                                                                                                    std::make_pair(0U, QmddNodeEdge::P)}),
                                                                                                                createOptimizedQmddPathWithoutGaps({std::make_pair(3U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(2U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(1U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(0U, QmddNodeEdge::N)}),
                                                                                                                createOptimizedQmddPathWithoutGaps({std::make_pair(3U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(2U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(1U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(0U, QmddNodeEdge::P)}),
                                                                                                                createOptimizedQmddPathWithoutGaps({std::make_pair(3U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(2U, QmddNodeEdge::P),
                                                                                                                                                    std::make_pair(1U, QmddNodeEdge::P),
                                                                                                                                                    std::make_pair(0U, QmddNodeEdge::P)}),
                                                                                                                createOptimizedQmddPathWithoutGaps({std::make_pair(3U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(2U, QmddNodeEdge::P),
                                                                                                                                                    std::make_pair(1U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(0U, QmddNodeEdge::N)}),
                                                                                                                createOptimizedQmddPathWithoutGaps({std::make_pair(3U, QmddNodeEdge::P),
                                                                                                                                                    std::make_pair(2U, QmddNodeEdge::P),
                                                                                                                                                    std::make_pair(1U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(0U, QmddNodeEdge::P)}),
                                                                                                                createOptimizedQmddPathWithoutGaps({std::make_pair(3U, QmddNodeEdge::P),
                                                                                                                                                    std::make_pair(2U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(1U, QmddNodeEdge::P),
                                                                                                                                                    std::make_pair(0U, QmddNodeEdge::P)}),
                                                                                                                createOptimizedQmddPathWithoutGaps({std::make_pair(3U, QmddNodeEdge::P),
                                                                                                                                                    std::make_pair(2U, QmddNodeEdge::P),
                                                                                                                                                    std::make_pair(1U, QmddNodeEdge::P),
                                                                                                                                                    std::make_pair(0U, QmddNodeEdge::P)}),
                                                                                                                createOptimizedQmddPathWithoutGaps({std::make_pair(3U, QmddNodeEdge::P),
                                                                                                                                                    std::make_pair(2U, QmddNodeEdge::P),
                                                                                                                                                    std::make_pair(1U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(0U, QmddNodeEdge::N)}),
                                                                                                                createOptimizedQmddPathWithoutGaps({std::make_pair(3U, QmddNodeEdge::P),
                                                                                                                                                    std::make_pair(2U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(1U, QmddNodeEdge::N),
                                                                                                                                                    std::make_pair(0U, QmddNodeEdge::N)})});
    const std::optional<ToUniqueQmddPathSignatureOperands>  expectedOperandsToTurnQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueNotSuccessfulIfQmddPathWithSameSignatureExistsInComparedToQmddPathCollectionForAllSingleChangeRefPathCombinationsWithMatchesContainingGaps) {
    const auto referenceQmddPath = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
            QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
    });

    const auto comparedToQmddPaths = std::vector({OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                     QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                     QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})}),
                                                  OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                     QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                     QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})})});

    const std::optional<ToUniqueQmddPathSignatureOperands>  expectedOperandsToTurnQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueWithReferencePathOfLengthOneWillNotSucceed) {
    const OptimizedQmddPath referenceQmddPath   = createOptimizedQmddPathWithoutGaps({std::make_pair(0U, QmddNodeEdge::N)});
    const auto              comparedToQmddPaths = std::vector({createOptimizedQmddPathWithoutGaps({std::make_pair(0U, QmddNodeEdge::P)})});

    const std::optional<ToUniqueQmddPathSignatureOperands>  expectedOperandsToTurnQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueWithReferenceAndComparedToPathNotStartingAtSameQubitWillNotSucceedWithReferencePathContainingNoGapsAndComparedToPathContainingNoGaps) {
    const OptimizedQmddPath referenceQmddPath   = createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::N),
                                                                                      std::make_pair(0U, QmddNodeEdge::N)});
    const auto              comparedToQmddPaths = std::vector({OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                  QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P})})});

    const std::optional<ToUniqueQmddPathSignatureOperands>  expectedOperandsToTurnQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueWithReferenceAndComparedToPathNotStartingAtSameQubitWillNotSucceedWithReferencePathContainingNoGapsAndComparedToPathContainingGaps) {
    const OptimizedQmddPath referenceQmddPath   = createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::N),
                                                                                      std::make_pair(0U, QmddNodeEdge::N)});
    const auto              comparedToQmddPaths = std::vector({OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U})})});

    const std::optional<ToUniqueQmddPathSignatureOperands>  expectedOperandsToTurnQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueWithReferenceAndComparedToPathNotStartingAtSameQubitWillNotSucceedWithReferencePathContainingGapsAndComparedToPathContainingNoGaps) {
    const auto referenceQmddPath   = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U})});
    const auto comparedToQmddPaths = std::vector({createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::N),
                                                                                      std::make_pair(0U, QmddNodeEdge::N)})});

    const std::optional<ToUniqueQmddPathSignatureOperands>  expectedOperandsToTurnQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueWithReferenceAndComparedToPathNotStartingAtSameQubitWillNotSucceedWithReferencePathContainingGapsAndComparedToPathContainingGaps) {
    const auto referenceQmddPath   = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 0U, .nConsecutiveQubitInGap = 1U})});
    const auto comparedToQmddPaths = std::vector({OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})})});

    const std::optional<ToUniqueQmddPathSignatureOperands>  expectedOperandsToTurnQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueWillNotSucceedIfReferenceQmddPathIsAlreadyUniqueWithReferencePathContainingNoGaps) {
    const OptimizedQmddPath referenceQmddPath   = createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)});
    const auto              comparedToQmddPaths = std::vector({createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::P), std::make_pair(0U, QmddNodeEdge::N)})});

    const std::optional<ToUniqueQmddPathSignatureOperands>  expectedOperandsToTurnQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueWillNotSucceedIfReferenceQmddPathIsAlreadyUniqueWithReferencePathContainingGaps) {
    const auto referenceQmddPath   = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 1U}),
                                                        QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})});
    const auto comparedToQmddPaths = std::vector({createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::P), std::make_pair(0U, QmddNodeEdge::N)})});

    const std::optional<ToUniqueQmddPathSignatureOperands>  expectedOperandsToTurnQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeQmddPathUniqueWillNotSucceedIfReferenceQmddPathRequiresMoreThanOneQubitPolarityFlip) {
    const OptimizedQmddPath referenceQmddPath   = createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::N), std::make_pair(1U, QmddNodeEdge::P), std::make_pair(0U, QmddNodeEdge::P)});
    const auto              comparedToQmddPaths = std::vector({
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::P), std::make_pair(0U, QmddNodeEdge::N)}),
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)}),
    });

    const std::optional<ToUniqueQmddPathSignatureOperands>  expectedOperandsToTurnQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands>& actualOperandsToTurnQmddPathUnique   = getOperandsToMakeQmddPathSignatureUnique(referenceQmddPath, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnQmddPathUnique, actualOperandsToTurnQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeOneQmddPathUniqueWillNotSucceedIfComparedToQmddPathCollectionIsEmpty) {
    const auto                           referenceQmddPaths = std::vector({createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)}),
                                                                           createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)})});
    const std::vector<OptimizedQmddPath> comparedToQmddPaths{};

    const std::optional<ToUniqueQmddPathSignatureOperands> expectedOperandsToTurnOneQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands> actualOperandsToTurnOneQmddPathUnique   = getOperandsToMakeOneOfQmddPathSignaturesUnique(referenceQmddPaths, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnOneQmddPathUnique, actualOperandsToTurnOneQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeOneQmddPathUniqueWillNotSucceedIfReferenceQmddPathCollectionIsEmpty) {
    const std::vector<OptimizedQmddPath> referenceQmddPaths{};
    const auto                           comparedToQmddPaths = std::vector({createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)}),
                                                                            createOptimizedQmddPathWithoutGaps({std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)})});

    const std::optional<ToUniqueQmddPathSignatureOperands> expectedOperandsToTurnOneQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands> actualOperandsToTurnOneQmddPathUnique   = getOperandsToMakeOneOfQmddPathSignaturesUnique(referenceQmddPaths, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnOneQmddPathUnique, actualOperandsToTurnOneQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeOneQmddPathUniqueWillNotSucceedIfNoReferenceQmddPathContainingNoGapsCanBeMadeUnique) {
    const auto                                             referenceQmddPaths                      = std::vector({
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::N), std::make_pair(1U, QmddNodeEdge::P), std::make_pair(0U, QmddNodeEdge::N)}),
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::N), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)}),
    });
    const auto                                             comparedToQmddPaths                     = std::vector({createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::N)}),
                                                                                                                  createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)}),
                                                                                                                  createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::P), std::make_pair(0U, QmddNodeEdge::N)}),
                                                                                                                  createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::P), std::make_pair(0U, QmddNodeEdge::P)})});
    const std::optional<ToUniqueQmddPathSignatureOperands> expectedOperandsToTurnOneQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands> actualOperandsToTurnOneQmddPathUnique   = getOperandsToMakeOneOfQmddPathSignaturesUnique(referenceQmddPaths, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnOneQmddPathUnique, actualOperandsToTurnOneQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeOneQmddPathUniqueWillNotSucceedIfNoReferenceQmddPathContainingGapsCanBeMadeUnique) {
    const auto                                             referenceQmddPaths                      = std::vector({
            OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})}),
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::N), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)}),
    });
    const auto                                             comparedToQmddPaths                     = std::vector({createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::N)}),
                                                                                                                  createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)}),
                                                                                                                  createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::P), std::make_pair(0U, QmddNodeEdge::N)}),
                                                                                                                  createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::P), std::make_pair(0U, QmddNodeEdge::P)})});
    const std::optional<ToUniqueQmddPathSignatureOperands> expectedOperandsToTurnOneQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands> actualOperandsToTurnOneQmddPathUnique   = getOperandsToMakeOneOfQmddPathSignaturesUnique(referenceQmddPaths, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnOneQmddPathUnique, actualOperandsToTurnOneQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeOneQmddPathUniqueWillNotSucceedIfReferenceQmddPathHasLengthOne) {
    const auto                                             referenceQmddPaths                      = std::vector({
            OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 0U, .nConsecutiveQubitInGap = 1U})}),
            createOptimizedQmddPathWithoutGaps({std::make_pair(0U, QmddNodeEdge::N)}),
    });
    const auto                                             comparedToQmddPaths                     = std::vector({OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 0U, .nConsecutiveQubitInGap = 1U})})});
    const std::optional<ToUniqueQmddPathSignatureOperands> expectedOperandsToTurnOneQmddPathUnique = std::nullopt;
    const std::optional<ToUniqueQmddPathSignatureOperands> actualOperandsToTurnOneQmddPathUnique   = getOperandsToMakeOneOfQmddPathSignaturesUnique(referenceQmddPaths, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnOneQmddPathUnique, actualOperandsToTurnOneQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeOneQmddPathUniqueSucceedsForReferenceQmddPathContainingNoGaps) {
    const auto referenceQmddPaths  = std::vector({
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::N), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::N)}),
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::N), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)}),
    });
    const auto comparedToQmddPaths = std::vector({createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)}),
                                                  createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::P), std::make_pair(0U, QmddNodeEdge::N)})});

    const auto                                             expectedOperandsToTurnOneQmddPathUnique = ToUniqueQmddPathSignatureOperands({.controlQubitsFromFirstNodeInPathToTargetQubit = qc::Controls({qc::Control(2U, qc::Control::Type::Neg)}), .targetQubit = 1U});
    const std::optional<ToUniqueQmddPathSignatureOperands> actualOperandsToTurnOneQmddPathUnique   = getOperandsToMakeOneOfQmddPathSignaturesUnique(referenceQmddPaths, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnOneQmddPathUnique, actualOperandsToTurnOneQmddPathUnique);
}

TEST(QmddPathOperationTests, GetOperandsToMakeOneQmddPathUniqueSucceedsForReferenceQmddPathContainingGaps) {
    const auto referenceQmddPaths  = std::vector({
            OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}), QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})}),
            createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::N)}),
    });
    const auto comparedToQmddPaths = std::vector({createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::N)}),
                                                  createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::N), std::make_pair(0U, QmddNodeEdge::P)}),
                                                  createOptimizedQmddPathWithoutGaps({std::make_pair(2U, QmddNodeEdge::P), std::make_pair(1U, QmddNodeEdge::P), std::make_pair(0U, QmddNodeEdge::N)})});

    const auto                                             expectedOperandsToTurnOneQmddPathUnique = ToUniqueQmddPathSignatureOperands({.controlQubitsFromFirstNodeInPathToTargetQubit = qc::Controls({qc::Control(2U, qc::Control::Type::Neg)}), .targetQubit = 1U});
    const std::optional<ToUniqueQmddPathSignatureOperands> actualOperandsToTurnOneQmddPathUnique   = getOperandsToMakeOneOfQmddPathSignaturesUnique(referenceQmddPaths, comparedToQmddPaths);
    assertOperandsToTurnQmddPathUniqueMatch(expectedOperandsToTurnOneQmddPathUnique, actualOperandsToTurnOneQmddPathUnique);
}
