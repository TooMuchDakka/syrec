#ifndef SYREC_PARSER_SIGNAL_ACCESS_RESTRICTION_ERROR_FIXTURE_HPP
#define SYREC_PARSER_SIGNAL_ACCESS_RESTRICTION_ERROR_FIXTURE_HPP
#pragma once

#include <googletest/googletest/include/gtest/gtest.h>
#include "gtest/gtest.h"

#include "core/syrec/program.hpp"
#include "core/syrec/parser/parser_utilities.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace parser {
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
            return ParserUtilities::createError(line, column, expectedErrorMessage);
        }
    };

    class AssignmentOperandBuilder {
    public:
        static AssignmentOperandBuilder init() {
            return AssignmentOperandBuilder();
        }

        AssignmentOperandBuilder& forSignal(const std::string& signalName) {
            this->signalName = signalName;
            return *this;
        }

        AssignmentOperandBuilder& accessDimension(const std::optional<std::size_t> valueForDimension) {
            if (!accessedDimensions.has_value()) {
                std::vector<std::optional<std::size_t>> buffer{};
                accessedDimensions.emplace(buffer);
            }

            accessedDimensions->emplace_back(valueForDimension);
            return *this;
        }

        AssignmentOperandBuilder& accessBit(const std::optional<std::size_t> bitPosition) {
            accessedBitPosition = bitPosition;
            return *this;
        }

        AssignmentOperandBuilder& accessBitRange(const std::pair<std::size_t, std::size_t>& bitRange) {
            accessedBitRange = std::make_pair(std::make_optional(bitRange.first), std::make_optional(bitRange.second));
            return *this;
        }

        AssignmentOperandBuilder& accessBitRange(std::pair<std::optional<std::size_t>, std::optional<std::size_t>> bitRange) {
            accessedBitRange = bitRange;
            return *this;
        }

        [[nodiscard]] std::string stringifiy() const {
            return signalName + stringifyAccessedDimensions() + (accessedBitPosition.has_value() ? stringifiyAccessedBit() : stringifiyAccessedBitRange());
        }

        [[nodiscard]] bool usesLoopVariableAsIndexSomeWhere() const {
            return (accessedDimensions.has_value() && std::all_of(accessedDimensions->begin(), accessedDimensions->end(), [](const std::optional<std::size_t> optionalValueForDimension) { return !optionalValueForDimension.has_value(); }))
                || (accessedBitRange.has_value() && (!accessedBitRange->first.has_value() || !accessedBitRange->second.has_value()))
                || (accessedBitPosition.has_value() && !accessedBitPosition->has_value());
        }

    private:
        std::string                                                                      signalName;
        std::optional<std::vector<std::optional<std::size_t>>>                           accessedDimensions;
        std::optional<std::pair<std::optional<std::size_t>, std::optional<std::size_t>>> accessedBitRange;
        std::optional<std::optional<std::size_t>>                                                       accessedBitPosition;

        [[nodiscard]] std::string stringifyAccessedDimensions() const {
            if (!accessedDimensions.has_value()) {
                return "";
            }

            std::string stringifiedDimensions = "";
            for (const auto& accessedDimension: accessedDimensions.value()) {
                stringifiedDimensions += "[" + (accessedDimension.has_value() ? std::to_string(accessedDimension.value()) : "$i") + "]";
            }

            return stringifiedDimensions;
        }

        [[nodiscard]] std::string stringifiyAccessedBit() const {
            if (!accessedBitPosition.has_value()) {
                return "";
            }
            return "." + (accessedBitPosition->has_value() ? std::to_string(accessedBitPosition->value()) : "$i");
        }

        [[nodiscard]] std::string stringifiyAccessedBitRange() const {
            if (!accessedBitRange.has_value()) {
                return "";
            }

            return ("." + (accessedBitRange->first.has_value() ? std::to_string(accessedBitRange->first.value()) : "$i")) + (":" + (accessedBitRange->second.has_value() ? std::to_string(accessedBitRange->second.value()) : "$i"));
        }
    };

    
class SyrecParserSignalAccessRestrictionErrorFixture:
        public ::testing::Test,
        public testing::WithParamInterface<std::tuple<std::string, AssignmentOperandBuilder, ExcludedCases>> {
    public:
        inline static const std::string assignedToSignalName = "b";
        inline static const std::string otherSignalName      = "a";

        inline static constexpr std::pair<std::size_t, std::size_t> defaultBitRange                     = std::make_pair<std::size_t, std::size_t>(2, 8);
        inline static constexpr std::pair<std::size_t, std::size_t> overlappingBitRangeBeforeDefaultOne = std::make_pair<std::size_t, std::size_t>(0, 2);
        inline static constexpr std::pair<std::size_t, std::size_t> overlappingBitRangeAfterDefaultOne  = std::make_pair<std::size_t, std::size_t>(8, 15);
        inline static constexpr std::size_t                         defaultValueForAccessedDimension    = 1;

        AssignmentOperandBuilder assignmentLhsOperand;
        ExcludedCases            excludedTestCases;

        SyrecParserSignalAccessRestrictionErrorFixture() :
            excludedTestCases(ExcludedCases::NoneExcluded) {}

        [[nodiscard]] static AssignmentOperandBuilder createAccessOnWholeSignal() {
            return AssignmentOperandBuilder::init()
                    .forSignal(assignedToSignalName);
        }

        // TODO: Add exclude tests enum values handling (i.e. to only test simple bit range instead of special cases
        // TODO: Add bit ranges with compile time constants
        // TODO: Bit range tests for partial ones (i.e. .0:$i, $i:0, $i:($i +1)) ?
        [[nodiscard]] static std::vector<std::pair<std::string, ExpectedSignalAccessRestrictionError>> createTestData(const AssignmentOperandBuilder& assignmentLhsOperand, const ExcludedCases excludedTestCases) {
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

            std::vector<std::string> parsingErrorsSplit = ParserUtilities::splitCombinedErrors(parsingErrorsConcatinated);
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
        inline static const std::string loopHeader   = "for $i = 0 to 2 step 1 do";
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

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createWholeSignalTestData(const AssignmentOperandBuilder& assignmentLhsOperand) {
            return createTestData(assignmentLhsOperand, createAccessOnWholeSignal());
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createBitAccessOnSignalTestData(const AssignmentOperandBuilder& assignmentLhsOperand) {
            return createTestData(assignmentLhsOperand, createAccessOnBitOfSignal(defaultBitRange.first));
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createBitRangeAccessOnSignalTestData(const AssignmentOperandBuilder& assignmentLhsOperand, const std::pair<std::size_t, std::size_t>& bitRange) {
            return createTestData(assignmentLhsOperand, createAccessOnBitRangeOfSignal(bitRange));
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createAccessOnCompleteDimensionOfSignalTestData(const AssignmentOperandBuilder& assignmentLhsOperand) {
            return createTestData(assignmentLhsOperand, createAccessOnAnyValueForDimensionOfSignal());
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createAccessOnValueForDimensionOfSignalTestData(const AssignmentOperandBuilder& assignmentLhsOperand) {
            return createTestData(assignmentLhsOperand, createAccessOnValueForDimensionOfSignal(defaultValueForAccessedDimension));
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createAccessOnBitOfCompleteDimensionOfSignalTestData(const AssignmentOperandBuilder& assignmentLhsOperand) {
            return createTestData(assignmentLhsOperand, createAccessOnBitOfDimension(std::nullopt, defaultBitRange.first));   
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createAccessOnBitOfValueForDimensionOfSignalTestData(const AssignmentOperandBuilder& assignmentLhsOperand) {
            return createTestData(assignmentLhsOperand, createAccessOnBitOfDimension(std::make_optional(defaultValueForAccessedDimension), defaultBitRange.first));   
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createAccessOnBitRangeOfCompleteDimensionOfSignalTestData(const AssignmentOperandBuilder& assignmentLhsOperand, const std::pair<std::size_t, std::size_t>& bitRange) {
            return createTestData(assignmentLhsOperand, createAccessOnBitRangeOfDimension(std::nullopt, bitRange));
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createAccessOnBitRangeOfValueForDimensionOfSignalTestData(const AssignmentOperandBuilder& assignmentLhsOperand, const std::pair<std::size_t, std::size_t>& bitRange) {
            return createTestData(assignmentLhsOperand, createAccessOnBitRangeOfDimension(std::make_optional(defaultValueForAccessedDimension), bitRange));
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createAccessOnRuntimeConstantBitRangeOfSignalTestData(const AssignmentOperandBuilder& assignmentLhsOperand, const std::pair<std::optional<std::size_t>, std::optional<std::size_t>>& bitRange) {
            return createTestData(assignmentLhsOperand, createAccessOnRuntimeConstantBitRangeOfSignal(bitRange));
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createAccessOnRuntimeConstantBitRangeOfDimensionTestData(const AssignmentOperandBuilder& assignmentLhsOperand, const std::optional<std::size_t> valueForDimension, const std::pair<std::optional<std::size_t>, std::optional<std::size_t>>& bitRange) {
            return createTestData(assignmentLhsOperand, createAccessOnRuntimeConstantBitRangeOfDimension(valueForDimension, bitRange));
        }

        [[nodiscard]] static std::pair<std::string, ExpectedSignalAccessRestrictionError> createTestData(const AssignmentOperandBuilder& assignmentLhsOperand, const AssignmentOperandBuilder& assignmentRhsCustomOperand) {
            const auto stringifiedProgram = createProgram(assignmentLhsOperand, createDefaulSecondRhsOperand(), assignmentRhsCustomOperand);
            return std::make_pair(stringifiedProgram, determineExpectedErrorPosition(stringifiedProgram));   
        }

        [[nodiscard]] static AssignmentOperandBuilder createAccessOnValueForDimensionOfSignal(const std::size_t valueForDimension) {
            return createAccessOnWholeSignal().accessDimension(std::make_optional(valueForDimension));
        }

        [[nodiscard]] static AssignmentOperandBuilder createAccessOnAnyValueForDimensionOfSignal() {
            return createAccessOnWholeSignal().accessDimension(std::nullopt);
        }

        [[nodiscard]] static AssignmentOperandBuilder createAccessOnBitOfSignal(const std::size_t bitPosition) {
            return createAccessOnWholeSignal().accessBit(std::make_optional(bitPosition));
        }

        [[nodiscard]] static AssignmentOperandBuilder createAccessOnAnyBitOfSignal() {
            return createAccessOnWholeSignal().accessBit(std::nullopt);
        }

        [[nodiscard]] static AssignmentOperandBuilder createAccessOnBitRangeOfSignal(const std::pair<std::size_t, std::size_t>& bitRange) {
            return createAccessOnRuntimeConstantBitRangeOfSignal(std::make_pair(std::make_optional(bitRange.first), std::make_optional(bitRange.second)));
        }

        [[nodiscard]] static AssignmentOperandBuilder createAccessOnRuntimeConstantBitRangeOfSignal(const std::pair<std::optional<std::size_t>, std::optional<std::size_t>>& bitRange) {
            return createAccessOnWholeSignal().accessBitRange(bitRange);
        }

        [[nodiscard]] static AssignmentOperandBuilder createAccessOnBitOfDimension(const std::optional<std::size_t> valueForDimension, const std::size_t bitPosition) {
            return (valueForDimension.has_value() ? createAccessOnValueForDimensionOfSignal(valueForDimension.value()) : createAccessOnAnyValueForDimensionOfSignal())
                    .accessBit(std::make_optional(bitPosition));
        }

        [[nodiscard]] static AssignmentOperandBuilder createAccessOnAnyBitOfDimension(const std::optional<std::size_t> valueForDimension) {
            return (valueForDimension.has_value() ? createAccessOnValueForDimensionOfSignal(valueForDimension.value()) : createAccessOnAnyValueForDimensionOfSignal())
                    .accessBit(std::nullopt);
        }

        [[nodiscard]] static AssignmentOperandBuilder createAccessOnBitRangeOfDimension(const std::optional<std::size_t> valueForDimension, const std::pair<std::size_t, std::size_t>& bitRange) {
            return createAccessOnRuntimeConstantBitRangeOfDimension(valueForDimension, std::make_pair(std::make_optional(bitRange.first), std::make_optional(bitRange.second)));
        }

        [[nodiscard]] static AssignmentOperandBuilder createAccessOnRuntimeConstantBitRangeOfDimension(const std::optional<std::size_t> valueForDimension, const std::pair<std::optional<std::size_t>, std::optional<std::size_t>>& bitRange) {
            return (valueForDimension.has_value() ? createAccessOnValueForDimensionOfSignal(valueForDimension.value()) : createAccessOnAnyValueForDimensionOfSignal())
                    .accessBitRange(bitRange);
        }

        [[nodiscard]] static AssignmentOperandBuilder createDefaulSecondRhsOperand() {
            return AssignmentOperandBuilder::init()
                .forSignal(otherSignalName);
        }

        [[nodiscard]] static std::string createProgram(const AssignmentOperandBuilder& assignedToSignal, const AssignmentOperandBuilder& rhsOperandOne, const AssignmentOperandBuilder& rhsOperandTwo) {
            const bool        needsLoopDefinition = assignedToSignal.usesLoopVariableAsIndexSomeWhere() || rhsOperandOne.usesLoopVariableAsIndexSomeWhere() || rhsOperandTwo.usesLoopVariableAsIndexSomeWhere();
            const std::string assignmentStatementStringified = assignedToSignal.stringifiy() + " += (" + rhsOperandOne.stringifiy() + " + " + rhsOperandTwo.stringifiy() + ")";
            
            if (needsLoopDefinition) {
                return moduleHeader + " " + loopHeader + " " + assignmentStatementStringified + " " + loopPostfix;
            } 
            return moduleHeader + " " + assignmentStatementStringified;
        }
    };
}
#endif