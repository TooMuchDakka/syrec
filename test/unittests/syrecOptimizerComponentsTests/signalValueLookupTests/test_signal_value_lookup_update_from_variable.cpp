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
            "LockEmptyWholeSignalByAssigningUnknownValueOfSameSize",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateAllStoredValuesForSignal();
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return true; },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "LockEmptySignalByAssigningUnknownValueToValueOfDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension});
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other
                );
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) {
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (* accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension
                        && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension
                        && *accessedDimensions.at(2) == SignalValueLookupTest::lockedValueOfThirdDimension);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LockEmptySignalByAssigningUnknownValueToGlobalBit",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value() 
                    || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
        std::make_tuple(
            "LockEmptySignalByAssigningUnknownValueToGlobalBitRange",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt, 
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
        std::make_tuple(
            "LockEmptySignalByAssigningWithIncompleteDimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt});
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) {
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension;
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LockEmptySignalByAssigningWithIncompleteBitDimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension 
                        && (!accessedBitRange.has_value() 
                            || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))));
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "LockEmptySignalByAssigningWithIncompleteBitRangeDimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension, std::nullopt}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                        {}, std::nullopt,
                        {}, std::nullopt,
                        *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension 
                        && (!accessedBitRange.has_value() 
                            || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)));
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (accessedBitRange.has_value()) {
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),

        std::make_tuple(
            "ExtendLockOfGlobalBitByLockingWholeSignal",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateAllStoredValuesForSignal();
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return true; },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "ExtendLockOfGlobalBitByLockingAnotherGlobalBitOfSignal",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0); 
                signalValueLookup->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                other->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other
                );
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) 
                    || !accessedBitRange.has_value() 
                    || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))
                    || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange));
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
         std::make_tuple(
            "ExtendLockOfGlobalBitByLockingGlobalBitRangeOfSignal",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                other->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other
                );
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions) 
                    || !accessedBitRange.has_value()
                    || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)
                    || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
             }),
         std::make_tuple(
            "ExtendLockOfGlobalBitByLockingValueOfDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                other->invalidateStoredValueFor({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension });
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other
                );
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension
                        && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension
                        && *accessedDimensions.at(2) == SignalValueLookupTest::lockedValueOfThirdDimension);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
        std::make_tuple(
            "ExtendLockOfGlobalBitByLockingBitOfSubdimension",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                signalValueLookup->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                BaseSignalValueLookupTest::resetAllValuesTo(other, 0);
                other->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                other->invalidateStoredValueForBitrange({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension }, std::nullopt,
                { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension }, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension 
                        && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension
                        && BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange)))
                    || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange)); 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
        std::make_tuple(
            "ExtendLockOfGlobalBitByLockingBitRangeOfSubdimension",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                signalValueLookup->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                BaseSignalValueLookupTest::resetAllValuesTo(other, 0);
                other->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                other->invalidateStoredValueForBitrange({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension }, SignalValueLookupTest::defaultNonOverlappingBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension }, std::nullopt,
                { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension }, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension 
                        && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension
                        && BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange))
                    || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange)); 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
        std::make_tuple(
            "DISABLED_ExtendLockOfGlobalBitByLockingValueOfIncompleteSubdimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),

        std::make_tuple(
            "ExtendLockOfGlobalBitRangeByLockingWholeSignal",
            [](const SignalValueLookup::ptr& signalValueLookup) { BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                /*
                 * TODO: On could also test assigning restrictions from one dimension to another or copying parts of a completely locked signal, etc.
                 */
                signalValueLookup->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);
                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateAllStoredValuesForSignal();
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "ExtendLockOfGlobalBitRangeByLockingAnotherGlobalBitOfSignal",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);
                other->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultNonOverlappingBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)
                    || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange); 
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
        std::make_tuple(
            "ExtendLockOfGlobalBitRangeByLockingOverlappingGlobalBitOfSignal",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);
                other->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultOverlappingBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange)
                    || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultOverlappingBitRange); 
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
        std::make_tuple(
            "ExtendLockOfGlobalBitRangeByLockingValueOfDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension
                    || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange); 
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
        std::make_tuple(
            "DISABLED_ExtendLockOfGlobalBitRangeByLockingBitOfSubdimension",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "DISABLED_ExtendLockOfGlobalBitRangeByLockingNonOverlappingBitRangeOfSubdimension",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "DISABLED_ExtendLockOfGlobalBitRangeByLockingOverlappingBitRangeOfSubdimension",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "DISABLED_ExtendLockOfGlobalBitRangeByLockingBitOfIncompleteSubdimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "DISABLED_ExtendLockOfGlobalBitRangeByLockingNonOverlappingBitRangeOfIncompleteSubdimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "DISABLED_ExtendLockOfGlobalBitRangeByLockingOverlappingBitRangeOfIncompleteSubdimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "DISABLED_ExtendLockOfGlobalBitRangeByLockingValueOfIncompleteSubdimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { return std::nullopt; }),

        std::make_tuple(
            "DISABLED_ExtendLockOfValueOfDimensionByLockingWholeSignal",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateAllStoredValuesForSignal();
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return false; },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) -> std::optional<unsigned int> { return std::nullopt; }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByBitGlobally",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange)); 
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByBitRangeGlobally",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});
                other->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension 
                    || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange); 
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
            }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByAnotherValueOfSameDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedOtherValueOfFirstDimension});
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedOtherValueOfFirstDimension; 
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByValueOfOtherDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});
                other->invalidateStoredValueFor({std::nullopt, SignalValueLookupTest::lockedValueOfSecondDimension});
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension
                    || *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension; 
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByIncompleteValueOfDimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfSecondDimension});
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension); 
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByIncompleteBitOfDimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedOtherValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension) {
                    return true;
                }

                if (*accessedDimensions.front() == SignalValueLookupTest::lockedOtherValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension){
                    return accessedBitRange.has_value() 
                        ? BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange)) 
                        : false;
                }
                return false; 
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByIncompleteBitRangeOfDimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension});
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension)){
                    return true;
                }

                if (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfSecondDimension){
                    return accessedBitRange.has_value() ? BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange) : false;
                }
                return false; 
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByLockingValueOfParentDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedOtherValueOfFirstDimension});
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess&) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || *accessedDimensions.front() == SignalValueLookupTest::lockedOtherValueOfFirstDimension
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension); 
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),   
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByLockingBitOfParentDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedOtherValueOfFirstDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                 return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedOtherValueOfFirstDimension && (accessedBitRange.has_value() ? BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange)) : true))
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension); 
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),
        std::make_tuple(
            "ExtendLockOfValueOfDimensionByLockingBitRangeOfParentDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedOtherValueOfFirstDimension}, SignalValueLookupTest::defaultBlockedBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedOtherValueOfFirstDimension && (accessedBitRange.has_value() ? BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange) : true))
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension); 
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),

        std::make_tuple(
            "ExtendLockOfBitOfValueOfDimensionByLockingWholeSignal",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueFor({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension});

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateAllStoredValuesForSignal();
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) { return true; },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess&) -> std::optional<unsigned int> { return std::nullopt; }),

        std::make_tuple(
            "ExtendLockOfBitOfValueOfDimensionByLockingAnotherGlobalBit",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                other->invalidateStoredValueForBitrange({}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange))
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange)));
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),

        std::make_tuple(
            "ExtendLockOfBitOfValueOfDimensionByLockingGlobalBitRange",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                 BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                other->invalidateStoredValueForBitrange({}, SignalValueLookupTest::defaultNonOverlappingBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || !accessedBitRange.has_value()
                    || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange)
                    || (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange)));
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),

        std::make_tuple(
            "ExtendLockOfBitOfValueOfDimensionByLockingBitRangesInBothTheSameAndOtherDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                 BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension}, SignalValueLookupTest::defaultBlockedBitRange);
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedOtherValueOfFirstDimension, SignalValueLookupTest::lockedValueOfFirstDimension}, SignalValueLookupTest::defaultNonOverlappingBitRange);
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)){
                    return true;
                }

                if (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension){
                    return !accessedBitRange.has_value()
                        || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange))
                        || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
                }
                if (*accessedDimensions.front() == SignalValueLookupTest::lockedOtherValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfFirstDimension){
                    return !accessedBitRange.has_value() || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultNonOverlappingBitRange);
                }
                return false;
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> { 
                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),

        std::make_tuple(
            "ExtendLockOfBitOfValueOfDimensionByLockingBitAndBitRangesInParentDimension",
            [](const SignalValueLookup::ptr& signalValueLookup) { 
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                other->invalidateStoredValueForBitrange({0}, SignalValueLookupTest::defaultBlockedBitRange);
                other->invalidateStoredValueForBitrange({SignalValueLookupTest::lockedOtherValueOfFirstDimension}, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange));
                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    {}, std::nullopt,
                    {}, std::nullopt,
                *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) { 
                if (!BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)){
                    return true;
                }

                if (*accessedDimensions.front() == SignalValueLookupTest::lockedValueOfFirstDimension && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension){
                    return !accessedBitRange.has_value()
                        || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                }
                if (*accessedDimensions.front() == 0) {
                    return !accessedBitRange.has_value()
                        || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::defaultBlockedBitRange);
                }
                if (accessedDimensions.front() == SignalValueLookupTest::lockedOtherValueOfFirstDimension){
                    return !accessedBitRange.has_value()
                        || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange));
                }
                return false;
            },
            [](const UserDefinedDimensionAccess&, const OptionalBitRangeAccess& accessedBitRange) -> std::optional<unsigned int> {
                if (accessedBitRange.has_value()){
                    return std::make_optional(SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::defaultValue, *accessedBitRange));
                }
                return std::make_optional(SignalValueLookupTest::defaultValue);
            }),

        /*
         * I.       CopyingBitsFromSameValueOfDimensionsInBothSignalsOK
         * II.      CopyingBitsFromDifferentValueOfDimensionsOK
         * III.     CopyingBitsFromDifferentPositionsOK
         *
         * IV.      CopyingBitRangeFromSameValueOfDimensionsInBothSignalsOK
         * V.       CopyingBitRangeFromDifferentValueOfDimensionsOK
         * VI.      CopyingBitRangeFromDifferentPositionsOK
         *
         * VII.     CopyingFromSameValuesOfDimensionWithFullDimensionAccess
         * VIII.    CopyingBetweenDifferentValuesOfDimensionsWithFullDimensionAccess

         defaultRemainingBlockedBitRangeAfterSignalWasUnblocked
        */
        std::make_tuple(
            "CopyingBitsFromSameValueOfDimensionsInBothSignalsOK",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->updateStoredValueFor({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, std::nullopt, SignalValueLookupTest::stripedDefaultValue);

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                BaseSignalValueLookupTest::resetAllValuesTo(other, 0);

                other->updateStoredValueFor({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, std::nullopt, SignalValueLookupTest::stripedDefaultValue);
                other->invalidateStoredValueForBitrange({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                other->invalidateStoredValueForBitrange({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange));

                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterSignalWasUnblocked.first, SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterSignalWasUnblocked.first),
                    { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange),
                    *other);

                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterSignalWasUnblocked.second, SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterSignalWasUnblocked.second),
                    { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange),
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.at(0) == SignalValueLookupTest::lockedValueOfFirstDimension 
                        && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension
                        && *accessedDimensions.at(2) == SignalValueLookupTest::lockedValueOfThirdDimension
                        && (!accessedBitRange.has_value() 
                            || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterSignalWasUnblocked.first, SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterSignalWasUnblocked.first))
                            || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterSignalWasUnblocked.second, SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterSignalWasUnblocked.second))));

            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                const bool expectDefaultZeroValue = *accessedDimensions.at(0) != SignalValueLookupTest::lockedOtherValueOfFirstDimension 
                        || *accessedDimensions.at(1) != SignalValueLookupTest::lockedOtherValueOfSecondDimension
                        || *accessedDimensions.at(2) != SignalValueLookupTest::lockedOtherValueOfThirdDimension;

                if (accessedBitRange.has_value()){
                    return std::make_optional(expectDefaultZeroValue ? 0 : SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::stripedDefaultValue, *accessedBitRange));
                }
                else {
                    return std::make_optional(expectDefaultZeroValue ? 0 : SignalValueLookupTest::stripedDefaultValue);
                }
            }
        ),

        std::make_tuple(
            "CopyingBitsFromDifferentValueOfDimensionsOK",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);
                signalValueLookup->updateStoredValueFor({ SignalValueLookupTest::lockedOtherValueOfFirstDimension, SignalValueLookupTest::lockedOtherValueOfSecondDimension, SignalValueLookupTest::lockedOtherValueOfThirdDimension }, std::nullopt, SignalValueLookupTest::stripedDefaultValue);

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                BaseSignalValueLookupTest::resetAllValuesTo(other, 0);

                other->updateStoredValueFor({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, std::nullopt, SignalValueLookupTest::stripedDefaultValue);
                other->invalidateStoredValueForBitrange({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange));
                other->invalidateStoredValueForBitrange({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange));

                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    { SignalValueLookupTest::lockedOtherValueOfFirstDimension, SignalValueLookupTest::lockedOtherValueOfSecondDimension, SignalValueLookupTest::lockedOtherValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterSignalWasUnblocked.first, SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterSignalWasUnblocked.first),
                    { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::firstBlockedBitOfDefaultBitRange, SignalValueLookupTest::firstBlockedBitOfDefaultBitRange),
                    *other);

                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    { SignalValueLookupTest::lockedOtherValueOfFirstDimension, SignalValueLookupTest::lockedOtherValueOfSecondDimension, SignalValueLookupTest::lockedOtherValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterSignalWasUnblocked.second, SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterSignalWasUnblocked.second),
                    { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::lastBlockedBitOfDefaultBitRange, SignalValueLookupTest::lastBlockedBitOfDefaultBitRange),
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.at(0) == SignalValueLookupTest::lockedOtherValueOfFirstDimension 
                        && *accessedDimensions.at(1) == SignalValueLookupTest::lockedOtherValueOfSecondDimension
                        && *accessedDimensions.at(2) == SignalValueLookupTest::lockedOtherValueOfThirdDimension
                        && (!accessedBitRange.has_value() 
                            || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterSignalWasUnblocked.first, SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterSignalWasUnblocked.first))
                            || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterSignalWasUnblocked.second, SignalValueLookupTest::defaultRemainingBlockedBitRangeAfterSignalWasUnblocked.second))));

            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                const bool expectDefaultZeroValue = *accessedDimensions.at(0) != SignalValueLookupTest::lockedOtherValueOfFirstDimension 
                        || *accessedDimensions.at(1) != SignalValueLookupTest::lockedOtherValueOfSecondDimension
                        || *accessedDimensions.at(2) != SignalValueLookupTest::lockedOtherValueOfThirdDimension;

                if (accessedBitRange.has_value()){
                    return std::make_optional(expectDefaultZeroValue ? 0 : SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::stripedDefaultValue, *accessedBitRange));
                }
                else {
                    return std::make_optional(expectDefaultZeroValue ? 0 : SignalValueLookupTest::stripedDefaultValue);
                }
            }
        ),

        std::make_tuple(
            "CopyingBitRangeFromSameValueOfDimensionsInBothSignalsOK",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                BaseSignalValueLookupTest::resetAllValuesTo(other, 0);

                other->updateStoredValueFor({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, std::nullopt, SignalValueLookupTest::stripedDefaultValue);
                other->invalidateStoredValueForBitrange({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::bitRangeToCopyFrom.first, SignalValueLookupTest::bitRangeToCopyFrom.first));
                other->invalidateStoredValueForBitrange({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::bitRangeToCopyFrom.second, SignalValueLookupTest::bitRangeToCopyFrom.second));

                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, SignalValueLookupTest::bitRangeToCopyTo,
                    { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, SignalValueLookupTest::bitRangeToCopyFrom,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.at(0) == SignalValueLookupTest::lockedValueOfFirstDimension 
                        && *accessedDimensions.at(1) == SignalValueLookupTest::lockedValueOfSecondDimension
                        && *accessedDimensions.at(2) == SignalValueLookupTest::lockedValueOfThirdDimension
                        && (!accessedBitRange.has_value() 
                            || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::bitRangeToCopyTo)));

            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                const bool expectDefaultZeroValue = *accessedDimensions.at(0) != SignalValueLookupTest::lockedOtherValueOfFirstDimension 
                        || *accessedDimensions.at(1) != SignalValueLookupTest::lockedOtherValueOfSecondDimension
                        || *accessedDimensions.at(2) != SignalValueLookupTest::lockedOtherValueOfThirdDimension;

                if (accessedBitRange.has_value()){
                    return std::make_optional(expectDefaultZeroValue ? 0 : SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::stripedDefaultValue, *accessedBitRange));
                }
                else {
                    return std::make_optional(expectDefaultZeroValue ? 0 : SignalValueLookupTest::stripedDefaultValue);
                }
            }
        ),

        std::make_tuple(
            "CopyingBitRangeFromDifferentValueOfDimensionsOK",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                BaseSignalValueLookupTest::resetAllValuesTo(other, 0);

                other->updateStoredValueFor({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, std::nullopt, SignalValueLookupTest::stripedDefaultValue);
                other->invalidateStoredValueForBitrange({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::bitRangeToCopyFrom.first, SignalValueLookupTest::bitRangeToCopyFrom.first));
                other->invalidateStoredValueForBitrange({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::bitRangeToCopyFrom.second, SignalValueLookupTest::bitRangeToCopyFrom.second));

                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    { SignalValueLookupTest::lockedOtherValueOfFirstDimension, SignalValueLookupTest::lockedOtherValueOfSecondDimension, SignalValueLookupTest::lockedOtherValueOfThirdDimension }, SignalValueLookupTest::bitRangeToCopyTo,
                    { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, SignalValueLookupTest::bitRangeToCopyFrom,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.at(0) == SignalValueLookupTest::lockedOtherValueOfFirstDimension 
                        && *accessedDimensions.at(1) == SignalValueLookupTest::lockedOtherValueOfSecondDimension
                        && *accessedDimensions.at(2) == SignalValueLookupTest::lockedOtherValueOfThirdDimension
                        && (!accessedBitRange.has_value() 
                            || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::bitRangeToCopyTo)));

            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                const bool expectDefaultZeroValue = *accessedDimensions.at(0) != SignalValueLookupTest::lockedOtherValueOfFirstDimension 
                        || *accessedDimensions.at(1) != SignalValueLookupTest::lockedOtherValueOfSecondDimension
                        || *accessedDimensions.at(2) != SignalValueLookupTest::lockedOtherValueOfThirdDimension;

                if (accessedBitRange.has_value()){
                    return std::make_optional(expectDefaultZeroValue ? 0 : SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::stripedDefaultValueForBitRangeToCopyTo, *accessedBitRange));
                }
                else {
                    return std::make_optional(expectDefaultZeroValue ? 0 : SignalValueLookupTest::stripedDefaultValueForBitRangeToCopyTo);
                }
            }
        ),

        std::make_tuple(
            "CopyingFromSameValuesOfDimensionWithFullDimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                BaseSignalValueLookupTest::resetAllValuesTo(other, 0);

                other->updateStoredValueFor({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, std::nullopt, SignalValueLookupTest::stripedDefaultValue);
                other->invalidateStoredValueForBitrange({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::bitRangeToCopyFrom.first, SignalValueLookupTest::bitRangeToCopyFrom.first));
                other->invalidateStoredValueForBitrange({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::bitRangeToCopyFrom.second, SignalValueLookupTest::bitRangeToCopyFrom.second));

                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, std::nullopt,
                    { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.at(0) == SignalValueLookupTest::lockedOtherValueOfFirstDimension 
                        && *accessedDimensions.at(1) == SignalValueLookupTest::lockedOtherValueOfSecondDimension
                        && *accessedDimensions.at(2) == SignalValueLookupTest::lockedOtherValueOfThirdDimension
                        && (!accessedBitRange.has_value() 
                            || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::bitRangeToCopyTo)));

            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                const bool expectDefaultZeroValue = *accessedDimensions.at(0) != SignalValueLookupTest::lockedOtherValueOfFirstDimension 
                        || *accessedDimensions.at(1) != SignalValueLookupTest::lockedOtherValueOfSecondDimension
                        || *accessedDimensions.at(2) != SignalValueLookupTest::lockedOtherValueOfThirdDimension;

                if (accessedBitRange.has_value()){
                    return std::make_optional(expectDefaultZeroValue ? 0 : SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::stripedDefaultValueForBitRangeToCopyTo, *accessedBitRange));
                }
                else {
                    return std::make_optional(expectDefaultZeroValue ? 0 : SignalValueLookupTest::stripedDefaultValueForBitRangeToCopyTo);
                }
            }
        ),

        std::make_tuple(
            "CopyingBetweenDifferentValuesOfDimensionsWithFullDimensionAccess",
            [](const SignalValueLookup::ptr& signalValueLookup) {
                BaseSignalValueLookupTest::resetAllValuesTo(signalValueLookup, 0);

                const auto& other = SignalValueLookupTest::createSignalValueLookupInitializedWithDefaultValue();
                BaseSignalValueLookupTest::resetAllValuesTo(other, 0);

                other->updateStoredValueFor({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, std::nullopt, SignalValueLookupTest::stripedDefaultValue);
                other->invalidateStoredValueForBitrange({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::bitRangeToCopyFrom.first, SignalValueLookupTest::bitRangeToCopyFrom.first));
                other->invalidateStoredValueForBitrange({ SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, ::optimizations::BitRangeAccessRestriction::BitRangeAccess(SignalValueLookupTest::bitRangeToCopyFrom.second, SignalValueLookupTest::bitRangeToCopyFrom.second));

                signalValueLookup->copyRestrictionsAndUnrestrictedValuesFrom(
                    { SignalValueLookupTest::lockedOtherValueOfFirstDimension, SignalValueLookupTest::lockedOtherValueOfSecondDimension, SignalValueLookupTest::lockedOtherValueOfThirdDimension }, std::nullopt,
                    { SignalValueLookupTest::lockedValueOfFirstDimension, SignalValueLookupTest::lockedValueOfSecondDimension, SignalValueLookupTest::lockedValueOfThirdDimension }, std::nullopt,
                    *other);
            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                return !BaseSignalValueLookupTest::isFullySpecifiedDimensionAccess(accessedDimensions)
                    || (*accessedDimensions.at(0) == SignalValueLookupTest::lockedOtherValueOfFirstDimension 
                        && *accessedDimensions.at(1) == SignalValueLookupTest::lockedOtherValueOfSecondDimension
                        && *accessedDimensions.at(2) == SignalValueLookupTest::lockedOtherValueOfThirdDimension
                        && (!accessedBitRange.has_value() 
                            || BaseSignalValueLookupTest::doesBitrangeOverlapOther(*accessedBitRange, SignalValueLookupTest::bitRangeToCopyTo)));

            },
            [](const UserDefinedDimensionAccess& accessedDimensions, const OptionalBitRangeAccess& accessedBitRange) {
                const bool expectDefaultZeroValue = *accessedDimensions.at(0) != SignalValueLookupTest::lockedOtherValueOfFirstDimension 
                        || *accessedDimensions.at(1) != SignalValueLookupTest::lockedOtherValueOfSecondDimension
                        || *accessedDimensions.at(2) != SignalValueLookupTest::lockedOtherValueOfThirdDimension;

                if (accessedBitRange.has_value()){
                    return std::make_optional(expectDefaultZeroValue ? 0 : SignalValueLookupTest::determineValueForBitRangeFromValue(SignalValueLookupTest::stripedDefaultValue, *accessedBitRange));
                }
                else {
                    return std::make_optional(expectDefaultZeroValue ? 0 : SignalValueLookupTest::stripedDefaultValue);
                }
            }
        )
    ),

    /*
    * Test to copy from a signal that is blocked completely, either a bit / bit range / value of dimension and check that it is correctly recreated in the other signal
    */

    [](const testing::TestParamInfo<SignalValueLookupTest::ParamType>& info) {
        auto testNameToTransform = std::get<0>(info.param);
        std::replace(testNameToTransform.begin(), testNameToTransform.end(), '-', '_');
        return testNameToTransform;
    });