#ifndef SYREC_PARSER_SIGNAL_ACCESS_RESTRICTION_ERROR_FIXTURE_HPP
#define SYREC_PARSER_SIGNAL_ACCESS_RESTRICTION_ERROR_FIXTURE_HPP
#pragma once

#include "gtest/gtest.h"

#include "syrec_signal_operand_builder.hpp"
#include "syrec_signal_operand_builder_utilities.hpp"

#include "core/syrec/program.hpp"
#include "core/syrec/parser/utils/message_utils.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace SignalAccessRestrictionParserTests {
    enum ExcludedCases {
        NoneExcluded,
        OverlappingBitRanges,
        RuntimeConstantBitRanges
    };

    struct ExpectedSignalAccessRestrictionError {
        std::size_t line;
        std::size_t column;
        std::string expectedErrorMessage;

        explicit ExpectedSignalAccessRestrictionError(const std::size_t line, const std::size_t column, const std::string expectedErrorMessage):
            line(line), column(column), expectedErrorMessage(expectedErrorMessage) {}

        [[nodiscard]] std::string stringifiy() const {
            return *messageUtils::tryStringifyMessage(messageUtils::Message({messageUtils::Message::ErrorCategory::Semantic, messageUtils::Message::Position(line, column), messageUtils::Message::Severity::Error, expectedErrorMessage}));
        }
    };

class SyrecParserSignalAccessRestrictionErrorFixture:
        public ::testing::Test,
        public testing::WithParamInterface<std::tuple<std::string, SignalOperandBuilder, ExcludedCases>> {
    public:
        inline static const std::string assignedToSignalName = "b";
        inline static const std::string otherSignalName      = "a";

        inline static constexpr std::pair<std::size_t, std::size_t> defaultBitRange                     = std::make_pair<std::size_t, std::size_t>(2, 8);
        inline static constexpr std::pair<std::size_t, std::size_t> overlappingBitRangeBeforeDefaultOne = std::make_pair<std::size_t, std::size_t>(0, 2);
        inline static constexpr std::pair<std::size_t, std::size_t> overlappingBitRangeAfterDefaultOne  = std::make_pair<std::size_t, std::size_t>(8, 15);
        inline static constexpr std::size_t                         defaultValueForAccessedDimension    = 1;

        SignalOperandBuilder assignmentLhsOperand;
        ExcludedCases            excludedTestCases;

        SyrecParserSignalAccessRestrictionErrorFixture() :
            excludedTestCases(ExcludedCases::NoneExcluded) {}

        // TODO: Add exclude tests enum values handling (i.e. to only test simple bit range instead of special cases
        // TODO: Add bit ranges with compile time constants
        // TODO: Bit range tests for partial ones (i.e. .0:$i, $i:0, $i:($i +1)) ?
        [[nodiscard]] static std::vector<std::pair<std::string, ExpectedSignalAccessRestrictionError>> createTestData(const SignalOperandBuilder& assignmentLhsOperand, const ExcludedCases excludedTestCases) {
            /*std::vector testCaseData = {
                    createWholeSignalTestData(assignmentLhsOperand),
                    createBitAccessOnSignalTestData(assignmentLhsOperand),
                    createBitRangeAccessOnSignalTestData(assignmentLhsOperand, defaultBitRange),
                    createAccessOnCompleteDimensionOfSignalTestData(assignmentLhsOperand),
                    createAccessOnValueForDimensionOfSignalTestData(assignmentLhsOperand),
                    createAccessOnBitOfCompleteDimensionOfSignalTestData(assignmentLhsOperand),
                    createAccessOnBitOfValueForDimensionOfSignalTestData(assignmentLhsOperand),
                    createAccessOnBitRangeOfCompleteDimensionOfSignalTestData(assignmentLhsOperand, defaultBitRange),
                    createAccessOnBitRangeOfValueForDimensionOfSignalTestData(assignmentLhsOperand, defaultBitRange),
            };

            if (!(excludedTestCases & ExcludedCases::OverlappingBitRanges)) {
                testCaseData.emplace_back(createBitRangeAccessOnSignalTestData(assignmentLhsOperand, overlappingBitRangeBeforeDefaultOne));
                testCaseData.emplace_back(createBitRangeAccessOnSignalTestData(assignmentLhsOperand, overlappingBitRangeAfterDefaultOne));

                testCaseData.emplace_back(createAccessOnBitRangeOfCompleteDimensionOfSignalTestData(assignmentLhsOperand, overlappingBitRangeBeforeDefaultOne));
                testCaseData.emplace_back(createAccessOnBitRangeOfCompleteDimensionOfSignalTestData(assignmentLhsOperand, overlappingBitRangeAfterDefaultOne));

                testCaseData.emplace_back(createAccessOnBitRangeOfValueForDimensionOfSignalTestData(assignmentLhsOperand, overlappingBitRangeBeforeDefaultOne));
                testCaseData.emplace_back(createAccessOnBitRangeOfValueForDimensionOfSignalTestData(assignmentLhsOperand, overlappingBitRangeAfterDefaultOne));
            }*/

            std::vector<std::pair<std::string, ExpectedSignalAccessRestrictionError>> testCaseData;
            if (!(excludedTestCases & ExcludedCases::RuntimeConstantBitRanges)) {
                testCaseData.emplace_back(createAccessOnRuntimeConstantBitRangeOfSignalTestData(assignmentLhsOperand, std::make_pair(std::make_optional(defaultBitRange.first), std::nullopt)));
                testCaseData.emplace_back(createAccessOnRuntimeConstantBitRangeOfSignalTestData(assignmentLhsOperand, std::make_pair(std::nullopt, std::make_optional(defaultBitRange.second))));
                testCaseData.emplace_back(createAccessOnRuntimeConstantBitRangeOfSignalTestData(assignmentLhsOperand, std::make_pair(std::nullopt, std::nullopt)));

                testCaseData.emplace_back(createAccessOnRuntimeConstantBitRangeOfDimensionTestData(assignmentLhsOperand, defaultValueForAccessedDimension, std::make_pair(std::make_optional(defaultBitRange.first), std::nullopt)));
                testCaseData.emplace_back(createAccessOnRuntimeConstantBitRangeOfDimensionTestData(assignmentLhsOperand, defaultValueForAccessedDimension, std::make_pair(std::nullopt, std::make_optional(defaultBitRange.second))));
                testCaseData.emplace_back(createAccessOnRuntimeConstantBitRangeOfDimensionTestData(assignmentLhsOperand, defaultValueForAccessedDimension, std::make_pair(std::nullopt, std::nullopt)));

                testCaseData.emplace_back(createAccessOnRuntimeConstantBitRangeOfDimensionTestData(assignmentLhsOperand, std::nullopt, std::make_pair(std::make_optional(defaultBitRange.first), std::nullopt)));
                testCaseData.emplace_back(createAccessOnRuntimeConstantBitRangeOfDimensionTestData(assignmentLhsOperand, std::nullopt, std::make_pair(std::nullopt, std::make_optional(defaultBitRange.second))));
                testCaseData.emplace_back(createAccessOnRuntimeConstantBitRangeOfDimensionTestData(assignmentLhsOperand, std::nullopt, std::make_pair(std::nullopt, std::nullopt)));
            }
            return testCaseData;
        }

        [[nodiscard]] static std::vector<std::string> filterNotRelevantErrors(const std::string& parsingErrorsConcatinated) {
            if (parsingErrorsConcatinated.empty()) {
                return {};
            }

            std::vector<std::string> parsingErrorsSplit = messageUtils::tryDeserializeStringifiedMessagesFromString(parsingErrorsConcatinated);
            parsingErrorsSplit.erase(
                    std::remove_if(
                            parsingErrorsSplit.begin(),
                            parsingErrorsSplit.end(),
                            [](const std::string& parsingError) { return parsingError.find("Accessing restricted part of signal '" + assignedToSignalName + "'") == std::string::npos; }),
                    parsingErrorsSplit.end());

            return parsingErrorsSplit;
        }

    private:
        inline static const std::string moduleHeader = "module main(in " + otherSignalName + "[2][4](16), out " + assignedToSignalName + "[2][4](16))";
        inline static const std::string loopVariableName = "$i";
        inline static const std::string loopHeader   = "for " + loopVariableName + " = 0 to 2 step 1 do";
        inline static const std::string loopPostfix  = "rof";
        
        void SetUp() override {
            const auto& testParameters               = GetParam();
            assignmentLhsOperand            = std::get<1>(testParameters);
            excludedTestCases               = std::get<2>(testParameters);
            
        }

        [[nodiscard]] static ExpectedSignalAccessRestrictionError determineExpectedErrorPosition(const std::string& stringifiedProgram) {
            const std::size_t assignedToSignalPositionInAssignmentRhs = stringifiedProgram.find_last_of(assignedToSignalName);
            return ExpectedSignalAccessRestrictionError(1, assignedToSignalPositionInAssignmentRhs, "Accessing restricted part of signal '" + assignedToSignalName + "'");
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createWholeSignalTestData(const SignalOperandBuilder& assignmentLhsOperand) {
            return createTestData(assignmentLhsOperand, SignalOperandBuilderHelpers::createAccessOnWholeSignal(assignedToSignalName));
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createBitAccessOnSignalTestData(const SignalOperandBuilder& assignmentLhsOperand) {
            return createTestData(assignmentLhsOperand, SignalOperandBuilderHelpers::createAccessOnBitOfSignal(assignedToSignalName, defaultBitRange.first));
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createBitRangeAccessOnSignalTestData(const SignalOperandBuilder& assignmentLhsOperand, const std::pair<std::size_t, std::size_t>& bitRange) {
            return createTestData(assignmentLhsOperand, SignalOperandBuilderHelpers::createAccessOnBitRangeOfSignal(assignedToSignalName, bitRange));
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createAccessOnCompleteDimensionOfSignalTestData(const SignalOperandBuilder& assignmentLhsOperand) {
            return createTestData(assignmentLhsOperand, SignalOperandBuilderHelpers::createAccessOnAnyValueForDimensionOfSignal(assignedToSignalName));
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createAccessOnValueForDimensionOfSignalTestData(const SignalOperandBuilder& assignmentLhsOperand) {
            return createTestData(assignmentLhsOperand, SignalOperandBuilderHelpers::createAccessOnValueForDimensionOfSignal(assignedToSignalName, defaultValueForAccessedDimension));
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createAccessOnBitOfCompleteDimensionOfSignalTestData(const SignalOperandBuilder& assignmentLhsOperand) {
            return createTestData(assignmentLhsOperand, SignalOperandBuilderHelpers::createAccessOnBitOfDimension(assignedToSignalName, std::nullopt, defaultBitRange.first));   
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createAccessOnBitOfValueForDimensionOfSignalTestData(const SignalOperandBuilder& assignmentLhsOperand) {
            return createTestData(assignmentLhsOperand, SignalOperandBuilderHelpers::createAccessOnBitOfDimension(assignedToSignalName, std::make_optional(defaultValueForAccessedDimension), defaultBitRange.first));   
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createAccessOnBitRangeOfCompleteDimensionOfSignalTestData(const SignalOperandBuilder& assignmentLhsOperand, const std::pair<std::size_t, std::size_t>& bitRange) {
            return createTestData(assignmentLhsOperand, SignalOperandBuilderHelpers::createAccessOnBitRangeOfDimension(assignedToSignalName, std::nullopt, bitRange));
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createAccessOnBitRangeOfValueForDimensionOfSignalTestData(const SignalOperandBuilder& assignmentLhsOperand, const std::pair<std::size_t, std::size_t>& bitRange) {
            return createTestData(assignmentLhsOperand, SignalOperandBuilderHelpers::createAccessOnBitRangeOfDimension(assignedToSignalName, std::make_optional(defaultValueForAccessedDimension), bitRange));
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createAccessOnRuntimeConstantBitRangeOfSignalTestData(const SignalOperandBuilder& assignmentLhsOperand, const std::pair<std::optional<std::size_t>, std::optional<std::size_t>>& bitRange) {
            return createTestData(assignmentLhsOperand, SignalOperandBuilderHelpers::createAccessOnRuntimeConstantBitRangeOfSignal(assignedToSignalName, bitRange));
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createAccessOnRuntimeConstantBitRangeOfDimensionTestData(const SignalOperandBuilder& assignmentLhsOperand, const std::optional<std::size_t> valueForDimension, const std::pair<std::optional<std::size_t>, std::optional<std::size_t>>& bitRange) {
            return createTestData(assignmentLhsOperand, SignalOperandBuilderHelpers::createAccessOnRuntimeConstantBitRangeOfDimension(assignedToSignalName, valueForDimension, bitRange));
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createTestData(const SignalOperandBuilder& assignmentLhsOperand, const SignalOperandBuilder& assignmentRhsCustomOperand) {
            const auto stringifiedProgram = createProgram(assignmentLhsOperand, createDefaulSecondRhsOperand(), assignmentRhsCustomOperand);
            return std::make_pair(stringifiedProgram, determineExpectedErrorPosition(stringifiedProgram));   
        }

        [[nodiscard]] static SignalOperandBuilder createDefaulSecondRhsOperand() {
            return SignalOperandBuilderHelpers::createAccessOnWholeSignal(otherSignalName);
        }

        [[nodiscard]] static std::string createProgram(const SignalOperandBuilder& assignedToSignal, const SignalOperandBuilder& rhsOperandOne, const SignalOperandBuilder& rhsOperandTwo) {
            const bool        needsLoopDefinition = assignedToSignal.usesLoopVariableAsIndexSomeWhere() || rhsOperandOne.usesLoopVariableAsIndexSomeWhere() || rhsOperandTwo.usesLoopVariableAsIndexSomeWhere();
            const std::string assignmentStatementStringified = assignedToSignal.stringifiy(loopVariableName) + " += (" + rhsOperandOne.stringifiy(loopVariableName) + " + " + rhsOperandTwo.stringifiy(loopVariableName) + ")";
            
            if (needsLoopDefinition) {
                return moduleHeader + " " + loopHeader + " " + assignmentStatementStringified + " " + loopPostfix;
            } 
            return moduleHeader + " " + assignmentStatementStringified;
        }
    };
}
#endif