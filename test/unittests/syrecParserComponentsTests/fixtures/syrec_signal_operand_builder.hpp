#ifndef SYREC_SIGNAL_OPERAND_BUILDER_HPP
#define SYREC_SIGNAL_OPERAND_BUILDER_HPP
#pragma once

#include <optional>
#include <string>
#include <vector>

namespace SignalAccessRestrictionParserTests {
    class SignalOperandBuilder {
    public:
        static SignalOperandBuilder init() {
            return SignalOperandBuilder();
        }

        SignalOperandBuilder& forSignal(const std::string& signalName) {
            this->signalName = signalName;
            return *this;
        }

        SignalOperandBuilder& accessDimension(const std::optional<std::size_t> valueForDimension) {
            if (!accessedDimensions.has_value()) {
                std::vector<std::optional<std::size_t>> buffer{};
                accessedDimensions.emplace(buffer);
            }

            accessedDimensions->emplace_back(valueForDimension);
            return *this;
        }

        SignalOperandBuilder& accessBit(const std::optional<std::size_t> bitPosition) {
            accessedBitPosition = bitPosition;
            return *this;
        }

        SignalOperandBuilder& accessBitRange(const std::pair<std::size_t, std::size_t>& bitRange) {
            accessedBitRange = std::make_pair(std::make_optional(bitRange.first), std::make_optional(bitRange.second));
            return *this;
        }

        SignalOperandBuilder& accessBitRange(std::pair<std::optional<std::size_t>, std::optional<std::size_t>> bitRange) {
            accessedBitRange = bitRange;
            return *this;
        }

        [[nodiscard]] std::string stringifiy(const std::string& loopVariableNameToUse) const {
            return signalName + stringifyAccessedDimensions(loopVariableNameToUse) + (accessedBitPosition.has_value() ? stringifiyAccessedBit(loopVariableNameToUse) : stringifiyAccessedBitRange(loopVariableNameToUse));
        }

        [[nodiscard]] bool usesLoopVariableAsIndexSomeWhere() const {
            return
            (accessedDimensions.has_value() 
                && std::all_of(
                    accessedDimensions->begin(), 
                    accessedDimensions->end(), 
                    [](const std::optional<std::size_t> optionalValueForDimension) {
                        return !optionalValueForDimension.has_value();
                    }))
            || (accessedBitRange.has_value() && (!accessedBitRange->first.has_value() || !accessedBitRange->second.has_value()))
            || (accessedBitPosition.has_value() && !accessedBitPosition->has_value());
        }

    private:
        std::string                                                                      signalName;
        std::optional<std::vector<std::optional<std::size_t>>>                           accessedDimensions;
        std::optional<std::pair<std::optional<std::size_t>, std::optional<std::size_t>>> accessedBitRange;
        std::optional<std::optional<std::size_t>>                                        accessedBitPosition;

        [[nodiscard]] std::string stringifyAccessedDimensions(const std::string& loopVariableNameToUse) const {
            if (!accessedDimensions.has_value()) {
                return "";
            }

            std::string stringifiedDimensions = "";
            for (const auto& accessedDimension: accessedDimensions.value()) {
                stringifiedDimensions += "[" + (accessedDimension.has_value() ? std::to_string(accessedDimension.value()) : loopVariableNameToUse) + "]";
            }

            return stringifiedDimensions;
        }

        [[nodiscard]] std::string stringifiyAccessedBit(const std::string& loopVariableNameToUse) const {
            if (!accessedBitPosition.has_value()) {
                return "";
            }
            return "." + (accessedBitPosition->has_value() ? std::to_string(accessedBitPosition->value()) : loopVariableNameToUse);
        }

        [[nodiscard]] std::string stringifiyAccessedBitRange(const std::string& loopVariableNameToUse) const {
            if (!accessedBitRange.has_value()) {
                return "";
            }

            return ("." + (accessedBitRange->first.has_value() ? std::to_string(accessedBitRange->first.value()) : loopVariableNameToUse)) + (":" + (accessedBitRange->second.has_value() ? std::to_string(accessedBitRange->second.value()) : loopVariableNameToUse));
        }
    };

};
#endif