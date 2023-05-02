#include "test_signal_value_lookup.hpp"

using namespace valueLookup;

using UserDefinedDimensionAccess = std::vector<std::optional<unsigned int>>;
using OptionalBitRangeAccess     = std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess>;

// TODO: Test cases for lifting unrelated restrictions
INSTANTIATE_TEST_SUITE_P(
        SignalValueLookupTests,
        SignalValueLookupTest,
        testing::Values(
            std::make_tuple(
                "DISABLED_LiftRestrictionFromCompletelyBlockedSignalWithAssignmentOfEmptySignal",
                [](const SignalValueLookup::ptr& signalValueLookup) {
                    BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                    const auto& other = SignalValueLookupTest::SignalValueLookupTest::createDefaultZeroInitializedSignalValueLookup();
                },
                [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
                [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
            std::make_tuple(
                "DISABLED_LiftRestrictionFromCompletelyBlockedSignalWithAssignmentOfGlobalSignal",
                [](const SignalValueLookup::ptr& signalValueLookup) {
                    BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                    const auto& other = SignalValueLookupTest::SignalValueLookupTest::createDefaultZeroInitializedSignalValueLookup();
                },
                [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
                [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
            std::make_tuple(
                "DISABLED_LiftRestrictionFromCompletelyBlockedSignalWithAssignmentOfBitRangeOfSignal",
                [](const SignalValueLookup::ptr& signalValueLookup) {
                    BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                    const auto& other = SignalValueLookupTest::SignalValueLookupTest::createDefaultZeroInitializedSignalValueLookup();
                },
                [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
                [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
            std::make_tuple(
                "DISABLED_LiftRestrictionFromCompletelyBlockedSignalWithAssignmentOfValueOfDimensionOfFullySpecifiedDimensionAccessOfSignal",
                [](const SignalValueLookup::ptr& signalValueLookup) {
                    BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                    const auto& other = SignalValueLookupTest::SignalValueLookupTest::createDefaultZeroInitializedSignalValueLookup();
                },
                [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
                [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
            std::make_tuple(
                "DISABLED_LiftRestrictionFromCompletelyBlockedSignalWithAssignmentOfValueOfDimensionOfIncompleteDimensionAccessOfSignal",
                [](const SignalValueLookup::ptr& signalValueLookup) {
                    BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                    const auto& other = SignalValueLookupTest::SignalValueLookupTest::createDefaultZeroInitializedSignalValueLookup();
                },
                [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
                [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
            std::make_tuple(
                "DISABLED_LiftRestrictionFromCompletelyBlockedSignalWithAssignmentOfIncompleteDimensionAccessOfBitOfSignal",
                [](const SignalValueLookup::ptr& signalValueLookup) {
                    BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                    const auto& other = SignalValueLookupTest::SignalValueLookupTest::createDefaultZeroInitializedSignalValueLookup();
                },
                [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
                [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
            std::make_tuple(
                "DISABLED_LiftRestrictionFromCompletelyBlockedSignalWithAssignmentOfIncompleteDimensionAccessOfBitRangeOfSignal",
                [](const SignalValueLookup::ptr& signalValueLookup) {
                    BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                    const auto& other = SignalValueLookupTest::SignalValueLookupTest::createDefaultZeroInitializedSignalValueLookup();
                },
                [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
                [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; })

                /*
                 * TODO: Tests for:
                 * - global bit
                 * - global bit range
                 * - value of dimension
                 * - bit of value of dimension
                 * - bit range of value of dimension
                 */
          ),
        [](const testing::TestParamInfo<SignalValueLookupTest::ParamType>& info) {
            auto testNameToTransform = std::get<0>(info.param);
            std::replace(testNameToTransform.begin(), testNameToTransform.end(), '-', '_');
            return testNameToTransform;
        });
