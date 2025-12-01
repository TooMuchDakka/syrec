/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "algorithms/synthesis/dd_synthesis.hpp"

#include "algorithms/optimization/esop_minimization.hpp"
#include "algorithms/synthesis/encoding.hpp"
#include "algorithms/synthesis/qmdd_to_identity_transformer.hpp"
#include "core/truthTable/truth_table.hpp"
#include "dd/DDDefinitions.hpp"
#include "dd/Node.hpp"
#include "dd/Operations.hpp"
#include "dd/Package.hpp"
#include "ir/Definitions.hpp"
#include "ir/QuantumComputation.hpp"
#include "ir/operations/Control.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <memory>
#include <unordered_set>
#include <utility>

using namespace qc::literals;

namespace {
    struct PermutationMatrixSubmatrix {
        std::size_t idxOfFirstRowInclusive;
        std::size_t idxOfLastRowExclusive;
        std::size_t idxOfFirstColInclusive;
        std::size_t idxOfLastColExclusive;
    };

    // TODO: We are assuming that the row and column index can be combined into an std::unit64_t without overflowing which would place a restriction on the maximum supported truth table size.
    [[nodiscard]] std::uint64_t generateCombinedIndexFromRowAndColumnIndexInPermutationMatrix(const std::size_t numRowsInPermutationMatrix, const std::size_t rowIndex, const std::size_t colIndex) {
        return (rowIndex * numRowsInPermutationMatrix) + colIndex;
    }

    [[nodiscard]] dd::mEdge determineTerminalNodeForIndexInPermutationMatrix(const std::unordered_set<std::uint64_t>& combinedIndicesOfOneEntriesInPermutationMatrix, const std::uint64_t indexOfEntryInUnrolledPermutationMatrix) {
        return combinedIndicesOfOneEntriesInPermutationMatrix.contains(indexOfEntryInUnrolledPermutationMatrix) ? dd::mEdge::one() : dd::mEdge::zero();
    }

    [[nodiscard]] dd::mEdge buildDDFromTruthtable(const syrec::TruthTable& truthTable, const std::unordered_set<std::uint64_t>& combinedIndicesOfOneEntriesInPermutationMatrix, const std::size_t levelInDecisionDiagram, const PermutationMatrixSubmatrix& processedPermutationMatrixSubmatrix, dd::Package& ddPackage) {
        if (levelInDecisionDiagram == 0) {
            assert(processedPermutationMatrixSubmatrix.idxOfLastRowExclusive - processedPermutationMatrixSubmatrix.idxOfFirstRowInclusive == 2);
            assert(processedPermutationMatrixSubmatrix.idxOfLastColExclusive - processedPermutationMatrixSubmatrix.idxOfFirstColInclusive == 2);

            const std::size_t numRowsInPermutationMatrix                          = truthTable.size();
            const auto        combinedIndexForTopLeftEntryOfPermutationMatrix     = generateCombinedIndexFromRowAndColumnIndexInPermutationMatrix(numRowsInPermutationMatrix, processedPermutationMatrixSubmatrix.idxOfFirstRowInclusive, processedPermutationMatrixSubmatrix.idxOfFirstColInclusive);
            const auto        combinedIndexForTopRightEntryOfPermutationMatrix    = generateCombinedIndexFromRowAndColumnIndexInPermutationMatrix(numRowsInPermutationMatrix, processedPermutationMatrixSubmatrix.idxOfFirstRowInclusive, processedPermutationMatrixSubmatrix.idxOfFirstColInclusive + 1U);
            const auto        combinedIndexForBottomLeftEntryOfPermutationMatrix  = generateCombinedIndexFromRowAndColumnIndexInPermutationMatrix(numRowsInPermutationMatrix, processedPermutationMatrixSubmatrix.idxOfFirstRowInclusive + 1U, processedPermutationMatrixSubmatrix.idxOfFirstColInclusive);
            const auto        combinedIndexForBottomRightEntryOfPermutationMatrix = generateCombinedIndexFromRowAndColumnIndexInPermutationMatrix(numRowsInPermutationMatrix, processedPermutationMatrixSubmatrix.idxOfFirstRowInclusive + 1U, processedPermutationMatrixSubmatrix.idxOfFirstColInclusive + 1U);

            if (combinedIndicesOfOneEntriesInPermutationMatrix.contains(combinedIndexForTopLeftEntryOfPermutationMatrix) || combinedIndicesOfOneEntriesInPermutationMatrix.contains(combinedIndexForTopRightEntryOfPermutationMatrix) || combinedIndicesOfOneEntriesInPermutationMatrix.contains(combinedIndexForBottomLeftEntryOfPermutationMatrix) || combinedIndicesOfOneEntriesInPermutationMatrix.contains(combinedIndexForBottomRightEntryOfPermutationMatrix)) {
                // TODO: What if garbage qubits were added?
                // TODO: dd::Qubit type is smaller than mqt::Qubit type

                const dd::mEdge topLeftSubmatrixOfPermutationMatrix     = determineTerminalNodeForIndexInPermutationMatrix(combinedIndicesOfOneEntriesInPermutationMatrix, combinedIndexForTopLeftEntryOfPermutationMatrix);
                const dd::mEdge topRightSubmatrixOfPermutationMatrix    = determineTerminalNodeForIndexInPermutationMatrix(combinedIndicesOfOneEntriesInPermutationMatrix, combinedIndexForTopRightEntryOfPermutationMatrix);
                const dd::mEdge bottomLeftSubmatrixOfPermutationMatrix  = determineTerminalNodeForIndexInPermutationMatrix(combinedIndicesOfOneEntriesInPermutationMatrix, combinedIndexForBottomLeftEntryOfPermutationMatrix);
                const dd::mEdge bottomRightSubmatrixOfPermutationMatrix = determineTerminalNodeForIndexInPermutationMatrix(combinedIndicesOfOneEntriesInPermutationMatrix, combinedIndexForBottomRightEntryOfPermutationMatrix);
                return ddPackage.makeDDNode(static_cast<dd::Qubit>(levelInDecisionDiagram), std::array<dd::mEdge, 4U>({topLeftSubmatrixOfPermutationMatrix, topRightSubmatrixOfPermutationMatrix, bottomLeftSubmatrixOfPermutationMatrix, bottomRightSubmatrixOfPermutationMatrix}));
            }
            return dd::mEdge::zero();
        }

        const auto rowMid = (processedPermutationMatrixSubmatrix.idxOfFirstRowInclusive + processedPermutationMatrixSubmatrix.idxOfLastRowExclusive) / 2;
        const auto colMid = (processedPermutationMatrixSubmatrix.idxOfFirstColInclusive + processedPermutationMatrixSubmatrix.idxOfLastColExclusive) / 2;
        // TODO: What if garbage qubits were added?
        // TODO: dd::Qubit type is smaller than mqt::Qubit type
        const dd::mEdge& topLeftSubmatrixOfPermutationMatrix     = buildDDFromTruthtable(truthTable, combinedIndicesOfOneEntriesInPermutationMatrix, levelInDecisionDiagram - 1U, PermutationMatrixSubmatrix({.idxOfFirstRowInclusive = processedPermutationMatrixSubmatrix.idxOfFirstRowInclusive, .idxOfLastRowExclusive = rowMid, .idxOfFirstColInclusive = processedPermutationMatrixSubmatrix.idxOfFirstColInclusive, .idxOfLastColExclusive = colMid}), ddPackage);
        const dd::mEdge& topRightSubmatrixOfPermutationMatrix    = buildDDFromTruthtable(truthTable, combinedIndicesOfOneEntriesInPermutationMatrix, levelInDecisionDiagram - 1U, PermutationMatrixSubmatrix({.idxOfFirstRowInclusive = processedPermutationMatrixSubmatrix.idxOfFirstRowInclusive, .idxOfLastRowExclusive = rowMid, .idxOfFirstColInclusive = colMid, .idxOfLastColExclusive = processedPermutationMatrixSubmatrix.idxOfLastColExclusive}), ddPackage);
        const dd::mEdge& bottomLeftSubmatrixOfPermutationMatrix  = buildDDFromTruthtable(truthTable, combinedIndicesOfOneEntriesInPermutationMatrix, levelInDecisionDiagram - 1U, PermutationMatrixSubmatrix({.idxOfFirstRowInclusive = rowMid, .idxOfLastRowExclusive = processedPermutationMatrixSubmatrix.idxOfLastRowExclusive, .idxOfFirstColInclusive = processedPermutationMatrixSubmatrix.idxOfFirstColInclusive, .idxOfLastColExclusive = colMid}), ddPackage);
        const dd::mEdge& bottomRightSubmatrixOfPermutationMatrix = buildDDFromTruthtable(truthTable, combinedIndicesOfOneEntriesInPermutationMatrix, levelInDecisionDiagram - 1U, PermutationMatrixSubmatrix({.idxOfFirstRowInclusive = rowMid, .idxOfLastRowExclusive = processedPermutationMatrixSubmatrix.idxOfLastRowExclusive, .idxOfFirstColInclusive = colMid, .idxOfLastColExclusive = processedPermutationMatrixSubmatrix.idxOfLastColExclusive}), ddPackage);
        return ddPackage.makeDDNode(static_cast<dd::Qubit>(levelInDecisionDiagram), std::array<dd::mEdge, 4U>({topLeftSubmatrixOfPermutationMatrix, topRightSubmatrixOfPermutationMatrix, bottomLeftSubmatrixOfPermutationMatrix, bottomRightSubmatrixOfPermutationMatrix}));
    }

    // TODO: Most significant qubit is expected to have label n-1 in QMDD instead of the initially assumed index 0 that it would have in a truth table.
    // TODO: TODO: Do we need to apply a permutation in the generated qc::QuantumComputation that will generate this mapping from qubit 0->(n-1), ..., (n-1)->0 and use swap gates after the QMDD was synthesized?
    [[nodiscard]] dd::mEdge buildDDFromTruthtable(const syrec::TruthTable& truthTable, dd::Package& ddPackage) {
        // TODO: Validate that truth table is square, etc. see dd::Package::makeDDFromMatrix, the code below is essentially a copy of the validation performed in the latter.
        if (truthTable.empty()) {
            return dd::mEdge::one();
        }
        assert(truthTable.nInputs() == truthTable.nOutputs());

        // TODO: Handle base case in which truth table only has a single entry?
        const std::size_t                 nRowsInTruthTable = truthTable.size();
        std::unordered_set<std::uint64_t> combinedIndicesOfOneEntriesInPermutationMatrix;
        for (auto truthTableEntriesIterator = truthTable.cbegin(); truthTableEntriesIterator != truthTable.cend(); ++truthTableEntriesIterator) {
            const auto& [inputValuesOfTruthTableEntry, outputValuesOfTruthTableEntry] = *truthTableEntriesIterator;
            combinedIndicesOfOneEntriesInPermutationMatrix.emplace(generateCombinedIndexFromRowAndColumnIndexInPermutationMatrix(nRowsInTruthTable, static_cast<std::size_t>(outputValuesOfTruthTableEntry.toInteger()), static_cast<std::size_t>(inputValuesOfTruthTableEntry.toInteger())));
        }

        const auto initialPermutationMatrixDimensions = PermutationMatrixSubmatrix({.idxOfFirstRowInclusive = 0U, .idxOfLastRowExclusive = nRowsInTruthTable, .idxOfFirstColInclusive = 0U, .idxOfLastColExclusive = nRowsInTruthTable});
        // TODO: What if garbage or ancillary qubits exist?
        return buildDDFromTruthtable(truthTable, combinedIndicesOfOneEntriesInPermutationMatrix, truthTable.nInputs() - 1, initialPermutationMatrixDimensions, ddPackage);
    }
} // namespace

namespace syrec {
    // Essentially a reimplementation of mEdge Package::makeDDFromMatrix(const CMat& matrix) {
    // TODO: Interface can be simplified to use reference to dd::Package instead of std::unique_ptr&
    auto buildDD(const TruthTable& tt, dd::Package& dd) -> dd::mEdge {
        return buildDDFromTruthtable(tt, dd);
    }

    // Refer to the decoder algorithm of https://www.cda.cit.tum.de/files/eda/2018_aspdac_coding_techniques_in_synthesis.pdf.
    template<class T>
    bool DDSynthesizer::decoder(const T& codewords, SynthesizerComponents& synthesizerComponents) {
        const auto codeLength = codewords.begin()->second.size();

        // decode the r most significant bits of the original output pattern.
        if (synthesizerComponents.truthTableInformation.r != 0U) {
            for (const auto& [pattern, code]: codewords) {
                TruthTable::Cube targetCube(pattern.cbegin(), pattern.cbegin() + static_cast<int>(synthesizerComponents.truthTableInformation.r));

                qc::Controls ctrl;
                for (auto i = 0U; i < codeLength; i++) {
                    if (code[i].has_value()) {
                        const auto ctrlType = *code[i] ? qc::Control::Type::Pos : qc::Control::Type::Neg;
                        ctrl.emplace(qc::Control{static_cast<qc::Qubit>((codeLength - 1U) - i), ctrlType});
                    }
                }

                const auto targetSize = targetCube.size();

                for (std::size_t i = 0U; i < targetSize; ++i) {
                    if (targetCube[i].has_value() && *(targetCube[i])) {
                        const auto targetBit = static_cast<qc::Qubit>((synthesizerComponents.truthTableInformation.totalNoQubits - 1U) - i);
                        synthesizerComponents.qc->mcx(ctrl, targetBit);
                        // TODO:
                        //++numGates;
                    }
                }
            }
        }

        // decode the remaining (m − r) primary outputs.
        const auto correctionBits = synthesizerComponents.truthTableInformation.m - synthesizerComponents.truthTableInformation.r;

        if (correctionBits == 0U) {
            return true;
        }

        TruthTable ttCorrection{};

        for (const auto& [pattern, code]: codewords) {
            TruthTable::Cube outCube(pattern);
            outCube.resize(synthesizerComponents.truthTableInformation.totalNoQubits);

            TruthTable::Cube inCube(pattern.cbegin(), pattern.cbegin() + static_cast<int>(synthesizerComponents.truthTableInformation.r));
            for (auto i = 0U; i < codeLength; i++) {
                inCube.emplace_back(code[i]);
            }

            // Extend the dc in the inputs.
            const auto completeInputs = inCube.completeCubes();
            for (auto const& completeInput: completeInputs) {
                ttCorrection.try_emplace(completeInput, outCube);
            }
        }

        const auto ttCorrectionDD = buildDD(ttCorrection, *synthesizerComponents.qmddPackage);
        // garbageFlag               = true;
        return synthesize(ttCorrectionDD, *synthesizerComponents.qmddPackage, *synthesizerComponents.qc);
    }

    DDSynthesizer::SynthesizerComponents DDSynthesizer::initializeSynthesizerComponents(const TruthTable& tt) {
        TruthTableQubitInformation truthTableQubitInformation;
        truthTableQubitInformation.n = tt.nInputs();
        truthTableQubitInformation.m = tt.nOutputs();

        // k1 -> Minimum no. of additional lines required.
        const std::size_t k1                     = tt.minimumAdditionalLinesRequired();
        truthTableQubitInformation.totalNoQubits = std::max(truthTableQubitInformation.n, truthTableQubitInformation.m + k1);
        truthTableQubitInformation.r             = (truthTableQubitInformation.m + k1) - std::max(truthTableQubitInformation.n, truthTableQubitInformation.m);

        return SynthesizerComponents({.truthTableInformation = truthTableQubitInformation,
                                      .qmddPackage           = std::make_unique<dd::Package>(truthTableQubitInformation.totalNoQubits),
                                      .qc                    = std::make_unique<qc::QuantumComputation>(truthTableQubitInformation.totalNoQubits, truthTableQubitInformation.totalNoQubits)});
    }

    bool DDSynthesizer::buildAndSynthesize(const TruthTable& tt, dd::Package& qmddPackage, qc::QuantumComputation& qc) {
        // the garbage and constants stored in the tt must be equal to the garbage and constants stored in qc.
        assert(tt.getGarbage() == qc.getGarbage() && tt.getConstants() == qc.getAncillary());
        const auto start = std::chrono::steady_clock::now();

        const auto src = buildDD(tt, qmddPackage);
        return synthesize(src, qmddPackage, qc);

        //runtime = static_cast<double>((std::chrono::steady_clock::now() - start).count());
    }

    std::unique_ptr<qc::QuantumComputation> DDSynthesizer::synthesizeOnePassTT(TruthTable tt) {
        SynthesizerComponents synthesizerComponents = initializeSynthesizerComponents(tt);
        if (tt.empty()) {
            return std::move(synthesizerComponents.qc);
        }

        // Refer to the one-pass synthesis algorithm of https://www.cda.cit.tum.de/files/eda/2017_tcad_one_pass_synthesis_reversible_circuits.pdf.
        // based on the totalNoBits, zeros are appended to the inputs and the outputs.
        const std::size_t numBitsPrependedToInputOfTruthTableEntries = synthesizerComponents.truthTableInformation.totalNoQubits - tt.nInputs();
        const std::size_t numBitsAppendedToOutputOfTruthTableEntries = synthesizerComponents.truthTableInformation.totalNoQubits - tt.nOutputs();
        augmentWithConstants(tt, synthesizerComponents.truthTableInformation.totalNoQubits);

        if (numBitsPrependedToInputOfTruthTableEntries > 0) {
            synthesizerComponents.qc->setLogicalQubitsAncillary(0U, static_cast<qc::Qubit>(numBitsPrependedToInputOfTruthTableEntries - 1U));
        }
        if (numBitsAppendedToOutputOfTruthTableEntries > 0) {
            synthesizerComponents.qc->setLogicalQubitsGarbage(static_cast<qc::Qubit>(tt.nOutputs()), static_cast<qc::Qubit>(synthesizerComponents.truthTableInformation.totalNoQubits - 1));
        }

        // If the one-pass synthesis is selected, the appended garbage bits need not be considered during the synthesis process.
        //garbageFlag = true;
        return buildAndSynthesize(tt, *synthesizerComponents.qmddPackage, *synthesizerComponents.qc) ? std::move(synthesizerComponents.qc) : nullptr;
    }

    std::unique_ptr<qc::QuantumComputation> DDSynthesizer::synthesizeCodingTechniquesTT(TruthTable tt, bool withAdditionalLine) {
        SynthesizerComponents synthesizerComponents = initializeSynthesizerComponents(tt);

        TruthTable::CubeMultiMap codewordWithoutAdditionalLine;
        TruthTable::CubeMap      codewordWithAdditionalLine;

        // TODO: Is encoder correct? Currently that seems to be the case.
        if (withAdditionalLine) {
            // Reference algorithm uses a huffman tree to determine the code word for the output patterns. While the number of garbage bits required
            // for said code word is used as the value of the associated vertex in the tree, however the reference algorithm does not define how the one determines how
            // the to be merged nodes are added as children of the merged node (i.e. which of the merge child nodes is the left/right child node). It should not make a
            // difference which of the two merged nodes serves as the left or right child since this will only determine whether a 0 or 1 is encoded in the code word when
            // traversing the huffman tree.
            codewordWithAdditionalLine = encodeWithAdditionalLine(tt);
        } else {
            codewordWithoutAdditionalLine = encodeWithoutAdditionalLine(tt);
        }

        synthesizerComponents.truthTableInformation.r                = synthesizerComponents.truthTableInformation.totalNoQubits - tt.nOutputs();
        const std::size_t numBitsPrependedToInputOfTruthTableEntries = synthesizerComponents.truthTableInformation.totalNoQubits - tt.nInputs();
        const std::size_t numBitsAppendedToOutputOfTruthTableEntries = synthesizerComponents.truthTableInformation.totalNoQubits - tt.nOutputs();
        augmentWithConstants(tt, synthesizerComponents.truthTableInformation.totalNoQubits);

        if (numBitsPrependedToInputOfTruthTableEntries > 0) {
            synthesizerComponents.qc->setLogicalQubitsAncillary(0U, static_cast<qc::Qubit>(numBitsPrependedToInputOfTruthTableEntries - 1U));
        }
        if (numBitsAppendedToOutputOfTruthTableEntries > 0) {
            synthesizerComponents.qc->setLogicalQubitsGarbage(static_cast<qc::Qubit>(synthesizerComponents.truthTableInformation.m), static_cast<qc::Qubit>(synthesizerComponents.truthTableInformation.totalNoQubits - 1));
        }
        // TODO: If the encoder defined in (https://www.cda.cit.tum.de/files/eda/2018_aspdac_coding_techniques_in_synthesis.pdf section IV.B) is use to synthesize the truth table or its associated permutation matrix
        // then added ancillary qubits are not storing the original values of some of the primary outputs. Are now Fredkin gates required to "swap" the decoded output value to the "correct"/expected primary output qubit?

        // Builds the QMDD for the given truth table (T) and transforms it so that QMDD represents the identity permutation matrix with the generated quantum operations being equal to the reversible circuit for T^(-1).
        // The transformation algorithm to get the identity permutation matrix for a given QMDD is described in (https://agra.informatik.uni-bremen.de/doc/konf/12aspdac_qmdd_synth_rev.pdf [Section IV - Algorithm Algorithm Q])
        if (!buildAndSynthesize(tt, *synthesizerComponents.qmddPackage, *synthesizerComponents.qc)) {
            return nullptr;
        }

        const auto start = std::chrono::steady_clock::now();

        // synthesizing the corresponding decoder circuit.
        // TODO: Is deencoder correct?
        if (withAdditionalLine) {
            return decoder(codewordWithAdditionalLine, synthesizerComponents) ? std::move(synthesizerComponents.qc) : nullptr;
        }
        return decoder(codewordWithoutAdditionalLine, synthesizerComponents) ? std::move(synthesizerComponents.qc) : nullptr;
        //runtime = runtime + static_cast<double>((std::chrono::steady_clock::now() - start).count());
        //return std::move(synthesizerComponents.qc);
    }

    bool DDSynthesizer::synthesize(const dd::mEdge& src, dd::Package& qmddPackage, qc::QuantumComputation& qc) {
        const auto qmddToIdentityTransformer = std::make_unique<QmddToIdentityTransformer>(qc, qmddPackage);
        return qmddToIdentityTransformer->synthesize(src);
    }

    // explicitly instantiate the template function decoder.
    template bool DDSynthesizer::decoder(const TruthTable::CubeMap& codewords, SynthesizerComponents& synthesizerComponents);

    template bool DDSynthesizer::decoder(const TruthTable::CubeMultiMap& codewords, SynthesizerComponents& synthesizerComponents);
} // namespace syrec
