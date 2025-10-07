/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

#include "algorithms/synthesis/encoding.hpp"

#include "core/truthTable/truth_table.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <map>
#include <memory>
#include <queue>
#include <stack>
#include <utility>
#include <vector>

namespace {
    void appendNCopiesOfBooleanToCollection(std::vector<bool>& vectorToModify, const bool booleanValue, const std::size_t nCopiesToAppend) {
        vectorToModify.resize(vectorToModify.size() + nCopiesToAppend, booleanValue);
    }

    void appendNCopiesOfBooleanToCollection(syrec::TruthTable::Cube& cubeMapEntry, const bool booleanValue, const std::size_t nCopiesToAppend) {
        cubeMapEntry.resize(cubeMapEntry.size() + nCopiesToAppend, booleanValue);
    }

    void prependNCopiesOfBooleanToCollection(std::vector<bool>& vectorToModify, const bool booleanValue, const std::size_t nCopiesToPrepend) {
        vectorToModify.insert(vectorToModify.begin(), nCopiesToPrepend, booleanValue);
    }

    void prependNCopiesOfBooleanToCollection(syrec::TruthTable::Cube& cubeMapEntry, const bool booleanValue, const std::size_t nCopiesToPrepend) {
        cubeMapEntry.resize(cubeMapEntry.size() + nCopiesToPrepend, booleanValue);
        std::ranges::rotate(cubeMapEntry, std::prev(cubeMapEntry.end(), static_cast<std::ptrdiff_t>(nCopiesToPrepend)));
    }
} // namespace

namespace syrec {

    template<class T>
    auto computeOutputFreq(TruthTable const& tt, T& outputFreq) -> void {
        for (auto cubeMapIterator = tt.cbegin(); cubeMapIterator != tt.cend(); ++cubeMapIterator) {
            const auto& matchingElementInOutputFreq = outputFreq.find(cubeMapIterator->second);
            if (matchingElementInOutputFreq == outputFreq.end()) {
                outputFreq.emplace(cubeMapIterator->second, 1U);
            } else {
                ++matchingElementInOutputFreq->second;
            }
        }
    }

    template<class T>
    auto topNodeOfHuffmanTree(T const& outputFreq) -> std::shared_ptr<MinHeapNode> {
        // create a priority queue for building the Huffman tree
        auto comp = [](const std::shared_ptr<MinHeapNode>& a, const std::shared_ptr<MinHeapNode>& b) {
            return *a > *b;
        };

        std::priority_queue<std::shared_ptr<MinHeapNode>, std::vector<std::shared_ptr<MinHeapNode>>, decltype(comp)>
                minHeap(comp);

        // initialize the leaves of the Huffman tree from the output frequencies
        for (const auto& [output, freq]: outputFreq) {
            const auto requiredGarbage = static_cast<std::size_t>(std::ceil(std::log2(freq)));
            minHeap.emplace(std::make_shared<MinHeapNode>(output, requiredGarbage));
        }

        // combine the nodes with the smallest weights until there is only one node left
        while (minHeap.size() > 1U) {
            // pop the two nodes with the smallest weights
            const auto left = minHeap.top();
            minHeap.pop();
            const auto right = minHeap.top();
            minHeap.pop();
            // compute appropriate frequency to cover both nodes
            const auto freq = std::max(left->freq, right->freq) + 1U;
            // create new parent node
            auto top   = std::make_shared<MinHeapNode>(TruthTable::Cube{}, freq);
            top->left  = left;
            top->right = right;
            // add node to queue
            minHeap.emplace(std::move(top));
        }

        return minHeap.top();
    }

    template<class T>
    auto alterTTAndCodewords(TruthTable& tt, T& encoding, std::size_t const& requiredGarbage) -> void {
        // Minimum no. of additional lines required.
        const auto additionalLines = tt.minimumAdditionalLinesRequired();
        const auto nBits           = std::max(tt.nInputs(), tt.nOutputs() + additionalLines);
        const auto r               = nBits - requiredGarbage;
        const auto nPrimaryOutputs = tt.nPrimaryOutputs();

        // resize garbage to the correct size.
        tt.getGarbage().resize(nBits);

        // the bits excluding the primary outputs.
        const auto garbageBits = nBits - nPrimaryOutputs;

        // the bits excluding the primary outputs must be lesser than or equal to the codeword length.
        assert(garbageBits <= requiredGarbage);

        // the bits excluding the primary outputs are set to garbage.
        for (auto i = nPrimaryOutputs; i < nBits; ++i) {
            tt.setGarbage(i);
        }

        // modify the codewords by filling in the redundant dc positions.
        for (auto& [pattern, code]: encoding) {
            TruthTable::Cube outCube(pattern);
            outCube.resize(nBits);

            TruthTable::Cube newCode{};
            newCode.reserve(requiredGarbage);
            for (auto i = 0U; i < requiredGarbage; i++) {
                if (code[i].has_value()) {
                    newCode.emplace_back(code[i]);
                } else {
                    newCode.emplace_back(outCube[r + i]);
                }
            }

            code = newCode;
        }
    }

    auto encodeWithAdditionalLine(TruthTable& tt) -> TruthTable::CubeMap {
        std::map<TruthTable::Cube, std::size_t> outputFreq;

        computeOutputFreq(tt, outputFreq);

        // if the truth table function is already reversible, no encoding is necessary
        if (outputFreq.size() == tt.size()) {
            return {};
        }

        auto const& topNode = topNodeOfHuffmanTree(outputFreq);

        // determine encoding from Huffman tree
        TruthTable::CubeMap encoding{};
        topNode->traverse({}, encoding);

        const auto requiredGarbage = topNode->freq;

        // resize all outputs to the correct size (by adding don't care values)
        for (auto& [input, output]: encoding) {
            output.resize(requiredGarbage);
        }

        // TODO: With this call the don't care values of the code words for the output patterns of the truth table are set (why do they need to be set when the encoding algorithm described in (This statement is not correct and only left here temporarily for completeness)
        // With the following call only the output patterns are replaced by their associated code word from the encoder (while also resizing the code word to the number of bits required in the decoder a step that should not be necessary).
        // One should be able to then transform the permutation matrix that contains don't care values to the identity matrix (see algorithm Q in https://agra.informatik.uni-bremen.de/doc/konf/12aspdac_qmdd_synth_rev.pdf [can this algorithm be applied with existing don't care values])?
        // TODO: The bug should then be somewhere in the implementation of the Q algorithm.
        // https://www.cda.cit.tum.de/files/eda/2018_aspdac_coding_techniques_in_synthesis.pdf [Section IV.B] synthesizes the permutation matrix [we still dont now when the dont care values are then finally set or how the permutation matrix is then synthesized]).
        // TODO: The crucial missing link currently is how are the values of the don't care garbage outputs determined (various algorithms for the diagonalization of the permutation matrix are defined in literature:
        // - https://agra.informatik.uni-bremen.de/doc/konf/12aspdac_qmdd_synth_rev.pdf
        // - https://iic.jku.at/files/eda/2017_date_efficient_embedding_of_non_reversible_functions.pdf
        alterTTAndCodewords(tt, encoding, requiredGarbage);

        // encode all the outputs
        for (auto& [input, output]: tt) {
            output = encoding[output];
        }

        // TODO: The encoding
        return encoding;
    }

    auto encodeWithoutAdditionalLine(TruthTable& tt) -> TruthTable::CubeMultiMap {
        std::multimap<TruthTable::Cube, std::size_t> outputFreq;
        computeOutputFreq(tt, outputFreq);

        // ensure that all output frequencies are a power of two
        for (auto it = outputFreq.begin(); it != outputFreq.end();) {
            auto freq = it->second;

            // continue if the output frequency is a power of two
            if ((freq & (freq - 1U)) == 0U) {
                ++it;
                continue;
            }

            // split frequency into powers of two
            std::size_t bitPos = 0U;
            while (freq > 0U) {
                if ((freq & 1U) == 1U) {
                    outputFreq.emplace(it->first, 1U << bitPos);
                }
                freq >>= 1U;
                ++bitPos;
            }
            // need to update iterator after erasing to avoid invalidating it
            it = outputFreq.erase(it);
        }

        // if the truth table function is already reversible, no encoding is necessary
        if (outputFreq.size() == tt.size()) {
            return {};
        }

        auto const& topNode = topNodeOfHuffmanTree(outputFreq);

        // determine encoding from Huffman tree
        TruthTable::CubeMultiMap encoding{};
        topNode->traverse({}, encoding);

        const auto requiredGarbage = topNode->freq;

        std::map<TruthTable::Cube, std::stack<TruthTable::Cube>> encFreq;
        // resize all outputs to the correct size (by adding don't care values)
        for (auto& [input, output]: encoding) {
            auto freq = 1U << (requiredGarbage - output.size());
            output.resize(requiredGarbage);

            while (freq != 0U) {
                encFreq[input].emplace(output);
                freq--;
            }
        }

        alterTTAndCodewords(tt, encoding, requiredGarbage);

        // encode all the outputs
        for (auto& [input, output]: tt) {
            const auto out = output;
            output         = encFreq[out].top();
            encFreq[out].pop();
        }

        return encoding;
    }

    // TODO: Helpful links: https://agra.informatik.uni-bremen.de/doc/konf/12aspdac_qmdd_synth_rev.pdf
    // TODO: https://www.cda.cit.tum.de/files/eda/2017_tcad_one_pass_synthesis_reversible_circuits.pdf (regarding addition of ancillary variables)
    auto augmentWithConstants(TruthTable& tt, const std::size_t expectedTotalNumberOfBitsPerTruthTableEntry) -> void {
        const std::size_t nConstantFlagsToAdd = expectedTotalNumberOfBitsPerTruthTableEntry - tt.getConstants().size();
        const std::size_t nGarbageFlagsToAdd  = expectedTotalNumberOfBitsPerTruthTableEntry - tt.getGarbage().size();

        prependNCopiesOfBooleanToCollection(tt.getConstants(), true, nConstantFlagsToAdd);
        appendNCopiesOfBooleanToCollection(tt.getGarbage(), true, nGarbageFlagsToAdd);

        std::vector<std::uint64_t> cubeMapInputKeys;
        cubeMapInputKeys.reserve(tt.size());
        for (const auto& cubeInput: std::views::keys(tt)) {
            cubeMapInputKeys.emplace_back(cubeInput.toInteger());
        }

        const std::size_t numInputsBitsOfCubeMapEntry = tt.nInputs();
        const std::size_t requiredOutConstants        = expectedTotalNumberOfBitsPerTruthTableEntry - tt.nOutputs();
        const std::size_t requiredInConstants         = expectedTotalNumberOfBitsPerTruthTableEntry - tt.nInputs();

        if (requiredInConstants == 0 && requiredOutConstants == 0) {
            return;
        }

        for (const auto cubeMapInputKey: cubeMapInputKeys) {
            const auto& readonlyMatchingCubeMapEntry = tt.find(cubeMapInputKey, numInputsBitsOfCubeMapEntry);
            assert(readonlyMatchingCubeMapEntry != tt.end());

            const auto distanceToMatchingCubeMapEntry = std::distance(tt.cbegin(), readonlyMatchingCubeMapEntry);
            const auto modifiableMatchingCubeMapEntry = std::next(tt.begin(), distanceToMatchingCubeMapEntry);

            // We need to prepend a number of zero bits to the input pattern of the truth table entry, an operation that cannot be applied directly to the input pattern
            // due to restrictions by the data structure used to store the truthtable (when iterating over the entries of a std::map<...> the keys are const) so we need to create
            // a copy of the input pattern, prepend the required bits and then modify the key of the associated entry via the std::map<...> and not the iterator. Since this "update"
            // operation of the key in the std::map<...> can cause a emplace/rebalance operation, iterators have to be assumed to be invalidated after the operation is completed thus
            // explaining our prior collection of the integer values of the entries of the truth table to traverse the latter.
            auto copyOfExistingCubeMapEntryKey = modifiableMatchingCubeMapEntry->first;
            prependNCopiesOfBooleanToCollection(copyOfExistingCubeMapEntryKey, false, requiredInConstants);
            appendNCopiesOfBooleanToCollection(modifiableMatchingCubeMapEntry->second, false, requiredOutConstants);

            if (requiredInConstants > 0) {
                auto backingNodeTypeOfCubeMapEntryInLookup  = tt.extract(readonlyMatchingCubeMapEntry->first);
                backingNodeTypeOfCubeMapEntryInLookup.key() = copyOfExistingCubeMapEntryKey;
                tt.insert(std::move(backingNodeTypeOfCubeMapEntryInLookup));
            }
        }
    }

    // explicitly instantiate function templates.
    template void                         computeOutputFreq(TruthTable const& tt, std::map<TruthTable::Cube, std::size_t>& outputFreq);
    template void                         computeOutputFreq(TruthTable const& tt, std::multimap<TruthTable::Cube, std::size_t>& outputFreq);
    template std::shared_ptr<MinHeapNode> topNodeOfHuffmanTree(std::map<TruthTable::Cube, std::size_t> const& outputFreq);
    template std::shared_ptr<MinHeapNode> topNodeOfHuffmanTree(std::multimap<TruthTable::Cube, std::size_t> const& outputFreq);
    template void                         alterTTAndCodewords(TruthTable& tt, TruthTable::CubeMap& encoding, std::size_t const& requiredGarbage);
    template void                         alterTTAndCodewords(TruthTable& tt, TruthTable::CubeMultiMap& encoding, std::size_t const& requiredGarbage);

} // namespace syrec
