#ifndef SYREC_SIGNAL_OPERAND_BUILDER_HELPERS_HPP
#define SYREC_SIGNAL_OPERAND_BUILDER_HELPERS_HPP
#pragma once

#include "syrec_signal_operand_builder.hpp"
#include <cstddef>
#include <optional>

namespace SignalAccessRestrictionParserTests {
    class SignalOperandBuilderHelpers {
    public:
        [[nodiscard]] static SignalOperandBuilder createAccessOnWholeSignal(const std::string& signalName) {
            return SignalOperandBuilder::init()
                    .forSignal(signalName);
        }

        [[nodiscard]] static SignalOperandBuilder createAccessOnValueForDimensionOfSignal(const std::string& signalName, const std::size_t valueForDimension) {
            return createAccessOnWholeSignal(signalName).accessDimension(std::make_optional(valueForDimension));
        }

        [[nodiscard]] static SignalOperandBuilder createAccessOnAnyValueForDimensionOfSignal(const std::string& signalName) {
            return createAccessOnWholeSignal(signalName).accessDimension(std::nullopt);
        }

        [[nodiscard]] static SignalOperandBuilder createAccessOnBitOfSignal(const std::string& signalName, const std::size_t bitPosition) {
            return createAccessOnWholeSignal(signalName).accessBit(std::make_optional(bitPosition));
        }

        [[nodiscard]] static SignalOperandBuilder createAccessOnAnyBitOfSignal(const std::string& signalName) {
            return createAccessOnWholeSignal(signalName).accessBit(std::nullopt);
        }

        [[nodiscard]] static SignalOperandBuilder createAccessOnBitRangeOfSignal(const std::string& signalName, const std::pair<std::size_t, std::size_t>& bitRange) {
            return createAccessOnRuntimeConstantBitRangeOfSignal(signalName, std::make_pair(std::make_optional(bitRange.first), std::make_optional(bitRange.second)));
        }

        [[nodiscard]] static SignalOperandBuilder createAccessOnRuntimeConstantBitRangeOfSignal(const std::string& signalName, const std::pair<std::optional<std::size_t>, std::optional<std::size_t>>& bitRange) {
            return createAccessOnWholeSignal(signalName).accessBitRange(bitRange);
        }

        [[nodiscard]] static SignalOperandBuilder createAccessOnBitOfDimension(const std::string& signalName, const std::optional<std::size_t> valueForDimension, const std::size_t bitPosition) {
            return (valueForDimension.has_value() ? createAccessOnValueForDimensionOfSignal(signalName, valueForDimension.value()) : createAccessOnAnyValueForDimensionOfSignal(signalName))
                    .accessBit(std::make_optional(bitPosition));
        }

        [[nodiscard]] static SignalOperandBuilder createAccessOnAnyBitOfDimension(const std::string& signalName, const std::optional<std::size_t> valueForDimension) {
            return (valueForDimension.has_value() ? createAccessOnValueForDimensionOfSignal(signalName, valueForDimension.value()) : createAccessOnAnyValueForDimensionOfSignal(signalName))
                    .accessBit(std::nullopt);
        }

        [[nodiscard]] static SignalOperandBuilder createAccessOnBitRangeOfDimension(const std::string& signalName, const std::optional<std::size_t> valueForDimension, const std::pair<std::size_t, std::size_t>& bitRange) {
            return createAccessOnRuntimeConstantBitRangeOfDimension(signalName, valueForDimension, std::make_pair(std::make_optional(bitRange.first), std::make_optional(bitRange.second)));
        }

        [[nodiscard]] static SignalOperandBuilder createAccessOnRuntimeConstantBitRangeOfDimension(const std::string& signalName, const std::optional<std::size_t> valueForDimension, const std::pair<std::optional<std::size_t>, std::optional<std::size_t>>& bitRange) {
            return (valueForDimension.has_value() ? createAccessOnValueForDimensionOfSignal(signalName, valueForDimension.value()) : createAccessOnAnyValueForDimensionOfSignal(signalName))
                    .accessBitRange(bitRange);
        }
    };
} // namespace SignalAccessRestrictionParserTests
#endif