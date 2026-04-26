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
#include "ir/QuantumComputation.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

namespace syrec {
    class QmddTransformer {
    public:
        struct QmddTransformationStatistic {
            std::uint64_t transformationRuntimeInMilliseconds;
        };

        struct QmddDumpConfig {
            std::string pathToDumpFile;
        };

        explicit QmddTransformer(const std::reference_wrapper<qc::QuantumComputation> quantumComputation, const std::reference_wrapper<dd::Package> qmddPkg):
            qc(quantumComputation), qmddPkg(qmddPkg) {}

        [[nodiscard]] bool             synthesizeQmdd(dd::mEdge edgeToQmddRoot, QmddTransformationStatistic* optionalTransformationStatistics = nullptr, const std::optional<QmddDumpConfig>& optionalQmddDumpConfig = std::nullopt);
        [[nodiscard]] static dd::mEdge constructQmddFromGatesOfQuantumComputation(const qc::QuantumComputation& quantumComputation, dd::Package& qmddPackage, const std::optional<QmddDumpConfig>& optionalQmddDumpConfig = std::nullopt);

    protected:
        enum class QmddExportOutputStreamOperation : std::uint8_t {
            OverwriteExisting,
            Append
        };
        static void exportQmddToFile(const dd::mEdge* edgeToRootNodeOfQmdd, const std::optional<QmddDumpConfig>& optionalQmddDumpConfig = std::nullopt, QmddExportOutputStreamOperation qmddExportOutputStreamOperation = QmddExportOutputStreamOperation::Append);

        std::reference_wrapper<qc::QuantumComputation> qc;
        std::reference_wrapper<dd::Package>            qmddPkg;
    };
} // namespace syrec
