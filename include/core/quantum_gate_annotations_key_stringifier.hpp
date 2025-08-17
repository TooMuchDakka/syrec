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

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace syrec {
    class QuantumGateAnnotationsKeyStringifier {
    public:
        using AnnotationKey = std::uint32_t;

        [[maybe_unused]] bool registerLabelForKey(const AnnotationKey key, const std::string& label) {
            const bool existsEntryForKey               = existsLabelForKey(key);
            annotationKeyToStringifiedLabelLookup[key] = label;
            return !existsEntryForKey;
        }

        [[nodiscard]] std::optional<std::string> getLabelForKey(const AnnotationKey key) const {
            if (const auto& matchingEntryForKey = annotationKeyToStringifiedLabelLookup.find(key); matchingEntryForKey != annotationKeyToStringifiedLabelLookup.cend()) {
                return matchingEntryForKey->second;
            }
            return std::nullopt;
        }

        [[nodiscard]] bool existsLabelForKey(const AnnotationKey key) const {
            return annotationKeyToStringifiedLabelLookup.find(key) != annotationKeyToStringifiedLabelLookup.cend();
        }

    protected:
        std::unordered_map<std::uint32_t, std::string> annotationKeyToStringifiedLabelLookup;
    };
} // namespace syrec
