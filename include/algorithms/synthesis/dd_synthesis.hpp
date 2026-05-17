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

#include "core/truthTable/truth_table.hpp"
#include "dd/Node.hpp"
#include "dd/Package.hpp"
#include "ir/QuantumComputation.hpp"
#include "qmdd_transformer.hpp"

#include <memory>

namespace syrec {

    [[nodiscard]] dd::mEdge buildDD(const TruthTable& tt, dd::Package& dd);

    class DDSynthesizer {
    public:
        struct SynthesisStatistics {
            std::uint64_t qmddTransformationRuntimeInMilliseconds;
        };

        struct QmddSynthesisResult {
            std::unique_ptr<qc::QuantumComputation> quantumComputation;
            std::unique_ptr<dd::Package>            qmddPackage;
            SynthesisStatistics                     synthesisStatistics;
        };

        static std::unique_ptr<qc::QuantumComputation> synthesizeCodingTechniques(const TruthTable& tt, const bool withAdditionalLine = true, BaseQmddDumper* qmddDumper = nullptr) {
            return synthesizeCodingTechniquesTT(tt, withAdditionalLine, qmddDumper);
        }

        static std::unique_ptr<qc::QuantumComputation> synthesizeOnePass(const TruthTable& tt, BaseQmddDumper* qmddDumper = nullptr) {
            return synthesizeOnePassTT(tt, qmddDumper);
        }

        // TODO: One could optionally pass a dd::Package parameter?
        static std::optional<QmddSynthesisResult> synthesizeQmdd(const TruthTable& tt, BaseQmddDumper* qmddDumper = nullptr) {
            SynthesizerComponents synthesizerComponents = initializeSynthesizerComponents(tt);
            if (const dd::mEdge edgeToRootOfQmdd = buildDD(tt, *synthesizerComponents.qmddPackage); synthesize(edgeToRootOfQmdd, *synthesizerComponents.qmddPackage, *synthesizerComponents.qc, qmddDumper)) {
                return QmddSynthesisResult({.quantumComputation = std::move(synthesizerComponents.qc), .qmddPackage = std::move(synthesizerComponents.qmddPackage), .synthesisStatistics = SynthesisStatistics()});
            }
            return std::nullopt;
        }

    private:
        // n -> No. of primary inputs.
        // m -> No. of primary outputs.
        // totalNoBits -> Total no. of bits required to create the circuit
        // r -> Additional variables/bits required to decode the output patterns.
        struct TruthTableQubitInformation {
            std::size_t n             = 0;
            std::size_t m             = 0;
            std::size_t totalNoQubits = 0;
            std::size_t r             = 0;
        };

        struct SynthesizerComponents {
            TruthTableQubitInformation              truthTableInformation;
            std::unique_ptr<dd::Package>            qmddPackage;
            std::unique_ptr<qc::QuantumComputation> qc;
        };

        template<class T>
        [[nodiscard]] static bool decoder(const T& codewords, SynthesizerComponents& synthesizerComponents, BaseQmddDumper* qmddDumper);

        [[nodiscard]] static std::unique_ptr<qc::QuantumComputation> synthesizeOnePassTT(TruthTable tt, BaseQmddDumper* qmddDumper);
        [[nodiscard]] static std::unique_ptr<qc::QuantumComputation> synthesizeCodingTechniquesTT(TruthTable tt, bool withAdditionalLine, BaseQmddDumper* qmddDumper);

        [[nodiscard]] static SynthesizerComponents initializeSynthesizerComponents(const TruthTable& tt);
        [[nodiscard]] static bool                  buildAndSynthesize(const TruthTable& tt, dd::Package& qmddPackage, qc::QuantumComputation& qc, BaseQmddDumper* qmddDumper);
        [[nodiscard]] static bool                  synthesize(const dd::mEdge& src, dd::Package& qmddPackage, qc::QuantumComputation& qc, BaseQmddDumper* qmddDumper);
    };

} // namespace syrec
