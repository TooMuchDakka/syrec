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

#include "dd/DDDefinitions.hpp"
#include "ir/operations/Control.hpp"

#include <cstdint>
#include <ostream>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace syrec {
    enum class QmddNodeEdge : std::uint8_t {
        N      = 1,
        PPrime = 2,
        NPrime = 4,
        P      = 8
    };

    struct QmddPathComponent {
        dd::Qubit    qubitAssociatedWithQmddNode = 0U;
        QmddNodeEdge qmddEdgeToChildNode         = QmddNodeEdge::N;
    };

    struct QmddPathGap {
        dd::Qubit   qubitAssociatedWithFirstQmddNodeOfGap = 0U;
        std::size_t nConsecutiveQubitInGap                = 0U;
    };

    using OptimizedQmddPath   = std::vector<std::variant<QmddPathComponent, QmddPathGap>>;
    using UnoptimizedQmddPath = std::vector<QmddPathComponent>;

    constexpr bool operator==(const QmddPathComponent lQmddPathComponent, const QmddPathComponent rQmddPathComponent) noexcept {
        return lQmddPathComponent.qubitAssociatedWithQmddNode == rQmddPathComponent.qubitAssociatedWithQmddNode && lQmddPathComponent.qmddEdgeToChildNode == rQmddPathComponent.qmddEdgeToChildNode;
    }

    constexpr bool operator==(const QmddPathGap lQmddPathGap, const QmddPathGap rQmddPathGap) noexcept {
        return lQmddPathGap.qubitAssociatedWithFirstQmddNodeOfGap == rQmddPathGap.qubitAssociatedWithFirstQmddNodeOfGap && lQmddPathGap.nConsecutiveQubitInGap == rQmddPathGap.nConsecutiveQubitInGap;
    }

    constexpr bool operator&(const QmddNodeEdge lQmddNodeEdge, const QmddNodeEdge rQmddNodeEdge) noexcept {
        return (static_cast<std::underlying_type_t<QmddNodeEdge>>(lQmddNodeEdge) & static_cast<std::underlying_type_t<QmddNodeEdge>>(rQmddNodeEdge)) > 0;
    }

    constexpr QmddNodeEdge operator|(const QmddNodeEdge lQmddNodeEdge, const QmddNodeEdge rQmddNodeEdge) noexcept {
        return static_cast<QmddNodeEdge>(static_cast<std::underlying_type_t<QmddNodeEdge>>(lQmddNodeEdge) | static_cast<std::underlying_type_t<QmddNodeEdge>>(rQmddNodeEdge));
    }

    constexpr void operator|=(QmddNodeEdge& lQmddNodeEdge, const QmddNodeEdge rQmddNodeEdge) noexcept {
        lQmddNodeEdge = lQmddNodeEdge | rQmddNodeEdge;
    }

    [[nodiscard]] constexpr qc::Control::Type getControlQubitTypeForQmddNodeEdge(const QmddNodeEdge qmddNodeEdge) noexcept {
        return qmddNodeEdge == QmddNodeEdge::P || qmddNodeEdge == QmddNodeEdge::PPrime ? qc::Control::Type::Pos : qc::Control::Type::Neg;
    }
    [[nodiscard]] inline qc::Control getControlQubitFromSignatureOfQmddPathComponent(const QmddPathComponent& qmddPathComponent) noexcept {
        return {qmddPathComponent.qubitAssociatedWithQmddNode, getControlQubitTypeForQmddNodeEdge(qmddPathComponent.qmddEdgeToChildNode)};
    }

    [[nodiscard]] static constexpr std::size_t convertQmddNodeEdgeEnumValueToArrayIdx(const QmddNodeEdge qmddNodeEdge) {
        switch (qmddNodeEdge) {
            case QmddNodeEdge::N:
                return 0U;
            case QmddNodeEdge::PPrime:
                return 1U;
            case QmddNodeEdge::NPrime:
                return 2U;
            default:
                return 3U;
        }
    }

    inline std::ostream& operator<<(std::ostream& os, const QmddNodeEdge qmddNodeEdge) {
        switch (qmddNodeEdge) {
            case QmddNodeEdge::N:
                os << "N";
                break;
            case QmddNodeEdge::PPrime:
                os << "N";
                break;
            case QmddNodeEdge::NPrime:
                os << "N";
                break;
            case QmddNodeEdge::P:
                os << "N";
                break;
            default:
                os << "UNKNOWN";
                break;
        }
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const QmddPathComponent& qmddPathComponent) {
        os << "Q: " << qmddPathComponent.qubitAssociatedWithQmddNode << " | E: " << qmddPathComponent.qmddEdgeToChildNode;
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const QmddPathGap& qmddPathGap) {
        os << "GAP FIRST QUBIT: " << qmddPathGap.qubitAssociatedWithFirstQmddNodeOfGap << " | # qubits covered by gap: " << qmddPathGap.nConsecutiveQubitInGap;
        return os;
    }
} // namespace syrec
