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
#include "algorithms/synthesis/qmddTransformation/qmdd_path_generator.hpp"

#include "gmock/gmock-matchers.h"
#include <cstddef>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace syrec;
namespace {
    void assertUnoptimizedQmddPathsMatch(const UnoptimizedQmddPath& expectedQmddPath, const UnoptimizedQmddPath* actualQmddPath) {
        ASSERT_THAT(actualQmddPath, testing::NotNull());
        ASSERT_EQ(expectedQmddPath.size(), actualQmddPath->size());
        for (std::size_t i = 0; i < expectedQmddPath.size(); ++i) {
            const QmddPathComponent& expectedQmddPathEntry = expectedQmddPath.at(i);
            const QmddPathComponent& actualQmddPathEntry   = actualQmddPath->at(i);
            ASSERT_EQ(expectedQmddPathEntry.qubitAssociatedWithQmddNode, actualQmddPathEntry.qubitAssociatedWithQmddNode) << "Mismatch between qmdd paths at index " << std::to_string(i);
            ASSERT_EQ(expectedQmddPathEntry.qmddEdgeToChildNode, actualQmddPathEntry.qmddEdgeToChildNode) << "Mismatch between qmdd node edge for qmdd nodes of qubit " << std::to_string(expectedQmddPathEntry.qubitAssociatedWithQmddNode);
        }
    }

    void assertGeneratedPathsCollectionsMatchesExpectedOne(QmddPathGenerator& qmddPathGenerator, const std::vector<UnoptimizedQmddPath>& expectedGeneratedQmddPaths) {
        for (std::size_t i = 0; i < expectedGeneratedQmddPaths.size(); ++i) {
            ASSERT_NO_FATAL_FAILURE(assertUnoptimizedQmddPathsMatch(expectedGeneratedQmddPaths.at(i), qmddPathGenerator.tryGenerateNextPath())) << "Mismatch between expected and actual qmdd paths at index " << std::to_string(i) << " in qmdd paths collection";
        }
    }
} // namespace

TEST(QmddPathGeneratorTests, CheckGeneratorForEmptyQmddPath) {
    auto qmddPathGenerator = QmddPathGenerator(OptimizedQmddPath());
    ASSERT_THAT(qmddPathGenerator.tryGenerateNextPath(), testing::IsNull());
}

TEST(QmddPathGeneratorTests, CheckGeneratorForSingleEntryQmddPathWithoutGaps) {
    const auto optimizedQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N})});

    auto qmddPathGenerator = QmddPathGenerator(optimizedQmddPath);
    ASSERT_NO_FATAL_FAILURE(assertGeneratedPathsCollectionsMatchesExpectedOne(qmddPathGenerator, {UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N})})}));
    ASSERT_THAT(qmddPathGenerator.tryGenerateNextPath(), testing::IsNull());
}

TEST(QmddPathGeneratorTests, CheckGeneratorForSingleEntryQmddPathWithGap) {
    const auto optimizedQmddPath = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})});

    auto qmddPathGenerator = QmddPathGenerator(optimizedQmddPath);
    ASSERT_NO_FATAL_FAILURE(assertGeneratedPathsCollectionsMatchesExpectedOne(qmddPathGenerator, {UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N})}),
                                                                                                  UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})}),
                                                                                                  UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N})}),
                                                                                                  UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P})})}));
    ASSERT_THAT(qmddPathGenerator.tryGenerateNextPath(), testing::IsNull());
}

TEST(QmddPathGeneratorTests, CheckGeneratorForQmddPathWithoutGaps) {
    const auto optimizedQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                      QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                      QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})});

    auto qmddPathGenerator = QmddPathGenerator(optimizedQmddPath);
    ASSERT_NO_FATAL_FAILURE(assertGeneratedPathsCollectionsMatchesExpectedOne(qmddPathGenerator, {UnoptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                                       QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})})}));
    ASSERT_THAT(qmddPathGenerator.tryGenerateNextPath(), testing::IsNull());
}

TEST(QmddPathGeneratorTests, CheckGeneratorForQmddPathWithGapsAtStartOfPath) {
    const auto optimizedQmddPath = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 2U}),
                                                      QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                      QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})});

    auto qmddPathGenerator = QmddPathGenerator(optimizedQmddPath);
    ASSERT_NO_FATAL_FAILURE(assertGeneratedPathsCollectionsMatchesExpectedOne(qmddPathGenerator, {UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                                  }),
                                                                                                  UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                                  }),
                                                                                                  UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                                  }),
                                                                                                  UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                                  })}));
    ASSERT_THAT(qmddPathGenerator.tryGenerateNextPath(), testing::IsNull());
}

TEST(QmddPathGeneratorTests, CheckGeneratorForQmddPathWithSingleQubitGapAtEndOfPath) {
    const auto optimizedQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                      QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                      QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 0U, .nConsecutiveQubitInGap = 1U})});

    auto qmddPathGenerator = QmddPathGenerator(optimizedQmddPath);
    ASSERT_NO_FATAL_FAILURE(assertGeneratedPathsCollectionsMatchesExpectedOne(qmddPathGenerator, {UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                  }),
                                                                                                  UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                  })}));
    ASSERT_THAT(qmddPathGenerator.tryGenerateNextPath(), testing::IsNull());
}

TEST(QmddPathGeneratorTests, CheckGeneratorForQmddPathWithNQubitGapAtEndOfPath) {
    const auto optimizedQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                      QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                      QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 1U, .nConsecutiveQubitInGap = 2U})});

    auto qmddPathGenerator = QmddPathGenerator(optimizedQmddPath);
    ASSERT_NO_FATAL_FAILURE(assertGeneratedPathsCollectionsMatchesExpectedOne(qmddPathGenerator, {UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                  }),
                                                                                                  UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                  }),
                                                                                                  UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                  }),
                                                                                                  UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                  })}));
    ASSERT_THAT(qmddPathGenerator.tryGenerateNextPath(), testing::IsNull());
}

TEST(QmddPathGeneratorTests, CheckGeneratorForQmddPathWithOneIntermediateGap) {
    const auto optimizedQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                      QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 2U, .nConsecutiveQubitInGap = 2U}),
                                                      QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})});

    auto qmddPathGenerator = QmddPathGenerator(optimizedQmddPath);
    ASSERT_NO_FATAL_FAILURE(assertGeneratedPathsCollectionsMatchesExpectedOne(qmddPathGenerator, {UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                                  }),
                                                                                                  UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                                  }),
                                                                                                  UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                                  }),
                                                                                                  UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                                  })}));
    ASSERT_THAT(qmddPathGenerator.tryGenerateNextPath(), testing::IsNull());
}

TEST(QmddPathGeneratorTests, CheckGeneratorForQmddPathWithMultipleIntermediateGaps) {
    const auto optimizedQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                      QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 2U}),
                                                      QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                      QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 0U, .nConsecutiveQubitInGap = 1U})});

    auto qmddPathGenerator = QmddPathGenerator(optimizedQmddPath);
    ASSERT_NO_FATAL_FAILURE(assertGeneratedPathsCollectionsMatchesExpectedOne(qmddPathGenerator, {UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                  }),
                                                                                                  UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                  }),
                                                                                                  UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                  }),
                                                                                                  UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                  }),
                                                                                                  UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                  }),
                                                                                                  UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                  }),
                                                                                                  UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::N}),
                                                                                                  }),
                                                                                                  UnoptimizedQmddPath({
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 2U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                                                                          QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::P}),
                                                                                                  })}));
    ASSERT_THAT(qmddPathGenerator.tryGenerateNextPath(), testing::IsNull());
}

TEST(QmddPathGeneratorTests, CheckGeneratorWillNotGenerateEntriesForQmddPathWithGapsUncoveredQubitsBetweenPathEntries) {
    const auto optimizedQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 5U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                      QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 4U, .nConsecutiveQubitInGap = 2U}),
                                                      QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
                                                      QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 0U, .nConsecutiveQubitInGap = 1U})});
    auto       qmddPathGenerator = QmddPathGenerator(optimizedQmddPath);
    ASSERT_THAT(qmddPathGenerator.tryGenerateNextPath(), testing::IsNull());
}

TEST(QmddPathGeneratorTests, CheckGeneratorWillNotGenerateEntriesForQmddPathWithoutGapsUncoveredQubitsBetweenPathEntries) {
    const auto optimizedQmddPath = OptimizedQmddPath({
            QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
            QmddPathComponent({.qubitAssociatedWithQmddNode = 0U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime}),
    });
    auto       qmddPathGenerator = QmddPathGenerator(optimizedQmddPath);
    ASSERT_THAT(qmddPathGenerator.tryGenerateNextPath(), testing::IsNull());
}

TEST(QmddPathGeneratorTests, CheckGeneratorWillNotGenerateEntriesForQmddPathWithoutGapsThatDoesNotCoverRequiredQubitRange) {
    const auto optimizedQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                      QmddPathComponent({.qubitAssociatedWithQmddNode = 3U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})});
    auto       qmddPathGenerator = QmddPathGenerator(optimizedQmddPath);
    ASSERT_THAT(qmddPathGenerator.tryGenerateNextPath(), testing::IsNull());
}

TEST(QmddPathGeneratorTests, CheckGeneratorWillNotGenerateEntriesForQmddPathWithGapsThatDoesNotCoverRequiredQubitRange) {
    const auto optimizedQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 4U, .qmddEdgeToChildNode = QmddNodeEdge::PPrime}),
                                                      QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 2U}),
                                                      QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})});
    auto       qmddPathGenerator = QmddPathGenerator(optimizedQmddPath);
    ASSERT_THAT(qmddPathGenerator.tryGenerateNextPath(), testing::IsNull());
}

TEST(QmddPathGeneratorTests, CheckGeneratorWillNotGenerateEntriesForQmddPathWithSingleEntryWithoutGapsNotCoveringRequiredQubitRange) {
    const auto optimizedQmddPath = OptimizedQmddPath({QmddPathComponent({.qubitAssociatedWithQmddNode = 1U, .qmddEdgeToChildNode = QmddNodeEdge::NPrime})});
    auto       qmddPathGenerator = QmddPathGenerator(optimizedQmddPath);
    ASSERT_THAT(qmddPathGenerator.tryGenerateNextPath(), testing::IsNull());
}

TEST(QmddPathGeneratorTests, CheckGeneratorWillNotGenerateEntriesForQmddPathWithSingleEntryWithGapNotCoveringRequiredQubitRange) {
    const auto optimizedQmddPath = OptimizedQmddPath({QmddPathGap({.qubitAssociatedWithFirstQmddNodeOfGap = 3U, .nConsecutiveQubitInGap = 2U})});
    auto       qmddPathGenerator = QmddPathGenerator(optimizedQmddPath);
    ASSERT_THAT(qmddPathGenerator.tryGenerateNextPath(), testing::IsNull());
}
