#ifndef SIGNAL_VALUE_LOOKUP_HPP
#define SIGNAL_VALUE_LOOKUP_HPP
#pragma once

#include "core/syrec/parser/optimizations/constantPropagation/potential_value_storage.hpp"
#include "core/syrec/parser/optimizations/constantPropagation/dimension_propagation_blocker.hpp"
#include "core/syrec/parser/optimizations/constantPropagation/shared_dimension_blocking_information.hpp"

#include <algorithm>
#include <memory>
#include <vector>

namespace optimizations {
    class SignalValueLookup {
    public:
        typedef std::shared_ptr<SignalValueLookup> ptr;

        explicit SignalValueLookup(unsigned int signalBitWidth, const std::vector<unsigned int>& signalDimensions, const unsigned int defaultValue):
            signalInformation(SignalDimensionInformation(signalBitWidth, signalDimensions)), dimensionAccessRestrictions(*initializeDimensionAccessRestrictionLayer(0)), valueLookup(*initializeValueLookupLayer(0, defaultValue)) {}

        void invalidateStoredValueFor(const std::vector<std::optional<unsigned int>>& accessedDimensions) const;
        void invalidateStoredValueForBitrange(const std::vector<std::optional<unsigned int>>& accessedDimensions, const BitRangeAccessRestriction::BitRangeAccess& bitRange) const;
        void invalidateAllStoredValuesForSignal() const;

        void liftRestrictionsFromWholeSignal() const;
        void liftRestrictionsOfDimensions(const std::vector<std::optional<unsigned int>>& accessedDimension, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRange);

        /*void liftRestrictionsFromValueOfDimension(const std::vector<std::optional<unsigned int>>& accessedDimensions);
        void liftRestrictionFromBitRange(const std::vector<std::optional<unsigned int>>& accessedDimensions, const BitRangeAccessRestriction::BitRangeAccess& bitRange);
        void liftRestrictionsForWholeSignal();*/

        [[nodiscard]] std::optional<unsigned int> tryFetchValueFor(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRange) const;
        void updateStoredValueFor(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRange, unsigned int newValue) const;

    private:
        const SignalDimensionInformation signalInformation;
        
        template<typename T>
        struct LayerData {
            typedef std::shared_ptr<LayerData>        ptr;

            T                            layerData;
            std::vector<LayerData::ptr>   nextLayerLinks;

            explicit LayerData(T layerData, std::vector<LayerData::ptr> nextLayerLinks):
                layerData(std::move(layerData)), nextLayerLinks(std::move(nextLayerLinks)) {}

            template <typename Fn>
            void applyRecursively(Fn&& applyLambda) {
                applyLambda(layerData);
                for (const auto& nextLayerLink : nextLayerLinks) {
                    nextLayerLink->applyRecursively(applyLambda);
                }
            }

            template <typename Fn>
            void               applyOnLastAccessedDimension(const unsigned int currDimension, const std::vector<std::optional<unsigned int>>& accessedDimensions, Fn&& applyLambda) {
                if (currDimension + 1 == accessedDimensions.size()) {
                    applyLambda(layerData, nextLayerLinks, accessedDimensions.at(currDimension), std::nullopt);
                }
                else {
                    const auto& accessedValueOfCurrentDimension = accessedDimensions.at(currDimension);
                    if (accessedValueOfCurrentDimension.has_value()) {
                        nextLayerLinks.at(*accessedValueOfCurrentDimension)->applyOnLastAccessedDimension(currDimension + 1, accessedDimensions, applyLambda);
                    } else {
                        for (auto& nextLayerLink : nextLayerLinks) {
                            nextLayerLink->applyOnLastAccessedDimension(currDimension + 1, accessedDimensions, applyLambda);
                        }
                    }
                }
            }

            template <typename Fn>
            void applyRecursivelyStartingFrom(const unsigned int startDimension, const std::optional<unsigned int> valueOfDimension, const std::vector<std::optional<unsigned int>>& accessedDimension, Fn&& applyLambda) {
                applyRecursivelyOnLayerStartingFrom(startDimension, valueOfDimension, 0, accessedDimension, applyLambda);
            }

            // TODO: This would not work if the would try to restriction the bit range 1:5 for an existing bit range restriction from 2:3 since the predicate that checks if an restriction already exists would evaluate to true
            template <typename Fn, typename Pn>
            void applyOnLastAccessedDimensionIfPredicateDoesHold(const unsigned int currDimension, const std::vector<std::optional<unsigned int>>& accessedDimensions, Fn&& applyLambda, Pn&& predicate) {
                if (currDimension + 1 == accessedDimensions.size()) {
                    if (predicate(layerData, accessedDimensions.at(currDimension), std::nullopt)) {
                        applyLambda(layerData, nextLayerLinks, accessedDimensions.at(currDimension), std::nullopt);    
                    }
                } else {
                    const auto& accessedValueOfCurrentDimension = accessedDimensions.at(currDimension);
                    if (!predicate(layerData, accessedValueOfCurrentDimension, std::nullopt)) {
                        return;
                    }

                    if (accessedValueOfCurrentDimension.has_value()) {
                        nextLayerLinks.at(*accessedValueOfCurrentDimension)->applyOnLastAccessedDimension(currDimension + 1, accessedDimensions, applyLambda);   
                    } else {
                        for (auto& nextLayerLink: nextLayerLinks) {
                            nextLayerLink->applyOnLastAccessedDimension(currDimension + 1, accessedDimensions, applyLambda);
                        }
                    }
                }
            }

            template <typename Fn>
            void applyOnLastLayer(const unsigned int lastLayerIndex, Fn&& applyLambda) {
                applyOnLayerIfLastOrCheckNextLayer(0, lastLayerIndex, applyLambda);
            }

            template <typename Pn>
            [[nodiscard]] bool doesPredicateHoldInAllDimensionsThenCheckRecursivelyElseStop(const std::vector<std::optional<unsigned int>>& accessedDimensions, Pn&& predicate) {
                return doesPredicateHoldInDimensionThenCheckRecursivelyElseStop(0, accessedDimensions, predicate);
            }

            template<typename Pn>
            [[nodiscard]] bool doesPredicateHoldInDimensionThenCheckRecursivelyElseStop(const unsigned int currDimension, const std::vector<std::optional<unsigned int>>& accessedDimensions, Pn&& predicate) {
                if (currDimension >= accessedDimensions.size()) {
                    return true;
                }

                const auto accessedValueOfDimension = accessedDimensions.at(currDimension);
                if (!predicate(currDimension, accessedValueOfDimension, layerData)) {
                    return false;
                }

                if (accessedValueOfDimension.has_value()) {
                    return nextLayerLinks.at(*accessedValueOfDimension)->doesPredicateHoldInDimensionThenCheckRecursivelyElseStop(currDimension + 1, accessedDimensions, predicate);
                }
                return std::all_of(
                        nextLayerLinks.cbegin(),
                        nextLayerLinks.cend(),
                        [currDimension, accessedDimensions, predicate](const LayerData::ptr& nextLayerLink) {
                            return nextLayerLink->doesPredicateHoldInDimensionThenCheckRecursivelyElseStop(currDimension + 1, accessedDimensions, predicate);
                        });
            }

        private:
            template <typename Fn>
            void applyOnLayerIfLastOrCheckNextLayer(const unsigned int currDimension, const unsigned int lastDimension, Fn&& applyLambda) {
                if (currDimension == lastDimension) {
                    applyLambda(layerData);
                } else {
                    for (auto& nextLayerLink: nextLayerLinks) {
                        nextLayerLink->applyOnLayerIfLastOrCheckNextLayer(currDimension + 1, lastDimension, applyLambda);
                    }
                }
            }

            template <typename Fn>
            void applyRecursivelyOnLayerStartingFrom(const unsigned int startDimension, const std::optional<unsigned int>& valueOfStartDimension, const unsigned int currDimension, const std::vector<std::optional<unsigned int>>& accessedDimensions, Fn&& applyLambda) {
                if (startDimension > accessedDimensions.size()) {
                    return;
                }

                if (currDimension != startDimension) {
                    const auto& accessedValueOfCurrentDimension = accessedDimensions.at(currDimension);
                    if (accessedValueOfCurrentDimension.has_value()) {
                        if (currDimension < startDimension) {
                            nextLayerLinks.at(*accessedValueOfCurrentDimension)->applyRecursivelyOnLayerStartingFrom(startDimension, valueOfStartDimension, currDimension + 1, accessedDimensions, applyLambda);
                        }
                        else {
                            applyLambda(layerData, accessedDimensions.at(currDimension));
                        }
                    } else {
                        for (auto& nextLayerLink: nextLayerLinks) {
                            if (currDimension < startDimension) {
                                nextLayerLink->applyRecursivelyOnLayerStartingFrom(startDimension, valueOfStartDimension, currDimension + 1, accessedDimensions, applyLambda);
                            } else {
                                applyLambda(layerData, accessedDimensions.at(currDimension));
                            }
                        }
                    }
                }
            }
        };

        const LayerData<DimensionPropagationBlocker::ptr>::ptr dimensionAccessRestrictions;
        const LayerData<std::map<unsigned int, std::optional<unsigned int>>>::ptr           valueLookup;

        /*
         * a[2][4][3]
         *
         * For this signal we create the following empty restrictions during initialization (assuming the created dimension restriction is only a pointer to a map that contains the actual information for the dimension)
         * I.  For the first dimension we only need one dimension restriction (the latter is a lookup map with 2 entries)
         * II. For the second dimension we need 2 dimension restrictions since the subsequent dimensions of the signal are independent from each other when accessing any value for the first dimension
         *      (i.e. when accessing a[0] the resulting signal would be of dimension [4][3] and independent from the [4][3] dimensional signal of a[1]) [each created dimension restriction consists of a lookup map of 4 entries)
         * III. The third dimension requires 4 dimension restrictions
         *
         * 0: 1 restriction -> Lookup entries: 2
         * 1: 2 restrictions -> Lookup entries: 4
         * 2: 4 restrictions -> Lookup entries: 3
         */
        //std::vector<std::vector<DimensionPropagationBlocker::ptr>> dimensionAccessRestrictions;

        /*
         * a[2][4][3]
         *
         * A dimension is called a layer and can either be an intermediate one that simple links to the next one or a lookup one containing the optionally substitutable values for the signal
         * The structure of this lookup for the signal above will look like the following:
         *
         * Layer 0: INTERMEDIATE, # lookup entries: 2 -> Link to layer 1
         * Layer 1: INTERMEDIATE, # lookup entries: 4 -> Link to layer 2
         * Layer 2: LOOKUP, # lookup entries 3 -> Optional value for access on last dimension (i.e. by accessing a[1][2][0])
         */
        //std::optional<PotentialValueStorage>                               valueLookup;

        //std::optional<PotentialValueStorage> initValueLookupWithDefaultValues(unsigned int defaultValue);
        //PotentialValueStorage initValueLookupLayer(unsigned int currDimension, unsigned int valueDimension, unsigned int defaultValue);

        [[nodiscard]] std::optional<LayerData<DimensionPropagationBlocker::ptr>::ptr> initializeDimensionAccessRestrictionLayer(const unsigned int dimension) {
            if (dimension >= signalInformation.valuesPerDimension.size()) {
                return std::nullopt;
            }

            const auto numValuesOfDimension = signalInformation.valuesPerDimension.at(dimension);
            auto       nextLayerLinks       = std::vector<std::shared_ptr<LayerData<DimensionPropagationBlocker::ptr>>>(numValuesOfDimension);
            if (dimension + 1 == signalInformation.valuesPerDimension.size()) {
                nextLayerLinks.clear();
            } else {
                for (unsigned int valueOfDimension = 0; valueOfDimension < numValuesOfDimension; ++valueOfDimension) {
                    nextLayerLinks[valueOfDimension] = *initializeDimensionAccessRestrictionLayer(dimension + 1);
                }    
            }
            return std::make_optional(std::make_unique<LayerData<DimensionPropagationBlocker::ptr>>(LayerData(std::make_shared<DimensionPropagationBlocker>(DimensionPropagationBlocker(dimension, signalInformation)), nextLayerLinks)));
        }

        [[nodiscard]] std::optional<LayerData<std::map<unsigned int, std::optional<unsigned int>>>::ptr> initializeValueLookupLayer(const unsigned int dimension, const std::optional<unsigned int> defaultValue) {
            if (dimension >= signalInformation.valuesPerDimension.size()) {
                return std::nullopt;
            }

            const auto numValuesOfDimension = signalInformation.valuesPerDimension.at(dimension);
            auto       nextLayerLinks      = std::vector<LayerData<std::map<unsigned int, std::optional<unsigned int>>>::ptr>(numValuesOfDimension);
            if (dimension + 1 == signalInformation.valuesPerDimension.size()) {
                nextLayerLinks.clear();
            } else {
                for (unsigned int valueOfDimension = 0; valueOfDimension < numValuesOfDimension; ++valueOfDimension) {
                    nextLayerLinks[valueOfDimension] = *initializeValueLookupLayer(dimension + 1, defaultValue);
                }    
            }

            std::map<unsigned int, std::optional<unsigned int>> perValueOfDimensionValueLookup;
            if (dimension + 1 == signalInformation.valuesPerDimension.size()) {
                for (unsigned int valueOfDimension = 0; valueOfDimension < numValuesOfDimension; ++valueOfDimension) {
                    perValueOfDimensionValueLookup.insert(std::pair(valueOfDimension, defaultValue));
                }   
            }
            
            return std::make_optional(std::make_unique<LayerData<std::map<unsigned int, std::optional<unsigned int>>>>(perValueOfDimensionValueLookup, nextLayerLinks));
        }

        [[nodiscard]] std::optional<std::vector<unsigned int>> transformAccessOnDimensions(const std::vector<std::optional<unsigned int>>& accessedDimensions) const;
        [[nodiscard]] bool                                     isValueLookupBlockedFor(const std::vector<std::optional<unsigned int>>& accessedDimensions, const std::optional<BitRangeAccessRestriction::BitRangeAccess>& bitRange) const;

        [[nodiscard]] static bool         isValueStorableInBitrange(const BitRangeAccessRestriction::BitRangeAccess& availableStorageSpace, unsigned int value);
        [[nodiscard]] static unsigned int extractPartsOfValue(unsigned int value, const BitRangeAccessRestriction::BitRangeAccess& partToFetch);
        [[nodiscard]] static unsigned int transformExistingValueByMergingWithNewOne(unsigned int currentValue, unsigned int newValue, const BitRangeAccessRestriction::BitRangeAccess& partsToUpdate);
    };
}; // namespace optimizations

#endif