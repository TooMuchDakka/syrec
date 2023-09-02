#ifndef LAYER_DATA_HPP
#define LAYER_DATA_HPP
#pragma once

#include <algorithm>

template<typename T>
struct LayerData {
    typedef std::shared_ptr<LayerData<T>> ptr;

    T                           layerData;
    std::vector<LayerData::ptr> nextLayerLinks;

    explicit LayerData(T layerData, std::vector<LayerData::ptr> nextLayerLinks):
        layerData(std::move(layerData)), nextLayerLinks(std::move(nextLayerLinks)) {}

    template<typename Fn>
    void applyRecursively(Fn&& applyLambda) {
        applyLambda(layerData);
        for (const auto& nextLayerLink: nextLayerLinks) {
            nextLayerLink->applyRecursively(applyLambda);
        }
    }

    template<typename Fn>
    void applyOnLastAccessedDimension(const unsigned int currDimension, const std::vector<std::optional<unsigned int>>& accessedDimensions, Fn&& applyLambda) {
        if (currDimension + 1 == accessedDimensions.size()) {
            applyLambda(layerData, nextLayerLinks, accessedDimensions.at(currDimension), std::nullopt);
        } else {
            const auto& accessedValueOfCurrentDimension = accessedDimensions.at(currDimension);
            if (accessedValueOfCurrentDimension.has_value()) {
                nextLayerLinks.at(*accessedValueOfCurrentDimension)->applyOnLastAccessedDimension(currDimension + 1, accessedDimensions, applyLambda);
            } else {
                for (auto& nextLayerLink: nextLayerLinks) {
                    nextLayerLink->applyOnLastAccessedDimension(currDimension + 1, accessedDimensions, applyLambda);
                }
            }
        }
    }

    template<typename Fn>
    void applyRecursivelyStartingFrom(const unsigned int startDimension, const std::optional<unsigned int> valueOfDimension, const std::vector<std::optional<unsigned int>>& accessedDimension, Fn&& applyLambda) {
        applyRecursivelyOnLayerStartingFrom(startDimension, valueOfDimension, 0, accessedDimension, applyLambda);
    }

    template<typename Fn, typename Pn>
    void applyOnLastAccessedDimensionIfPredicateDoesHold(const unsigned int currDimension, const std::vector<std::optional<unsigned int>>& accessedDimensions, Fn&& applyLambda, Pn&& predicate) {
        if (currDimension + 1 == accessedDimensions.size()) {
            if (predicate(layerData, accessedDimensions.at(currDimension), std::nullopt)) {
                applyLambda(layerData, nextLayerLinks, accessedDimensions.at(currDimension), std::nullopt);
            }
        } else {
            const auto& accessedValueOfCurrentDimension = accessedDimensions.at(currDimension);
            const bool  predicateEvaluationResult       = predicate(layerData, accessedValueOfCurrentDimension, std::nullopt);

            if (!predicateEvaluationResult || currDimension == (accessedDimensions.size() - 1)) {
                return;
            }
            
            /*if (!predicate(layerData, accessedValueOfCurrentDimension, std::nullopt)) {
                return;
            }*/

            if (accessedValueOfCurrentDimension.has_value()) {
                nextLayerLinks.at(*accessedValueOfCurrentDimension)->applyOnLastAccessedDimension(currDimension + 1, accessedDimensions, applyLambda);
            } else {
                for (auto& nextLayerLink: nextLayerLinks) {
                    nextLayerLink->applyOnLastAccessedDimension(currDimension + 1, accessedDimensions, applyLambda);
                }
            }
        }
    }

    template<typename Fn>
    void applyOnLastLayer(const unsigned int lastLayerIndex, Fn&& applyLambda) {
        applyOnLayerIfLastOrCheckNextLayer(0, lastLayerIndex, applyLambda);
    }

    template<typename Pn>
    [[nodiscard]] bool doesPredicateHoldInAllDimensionsThenCheckRecursivelyElseStop(const std::vector<std::optional<unsigned int>>& accessedDimensions, Pn&& predicate) {
        return doesPredicateHoldInDimensionThenCheckRecursivelyElseStop(0, accessedDimensions, predicate);
    }

    template<typename Pn>
    [[nodiscard]] bool doesPredicateHoldInDimensionThenCheckRecursivelyElseStop(const unsigned int currDimension, const std::vector<std::optional<unsigned int>>& accessedDimensions, Pn&& predicate) {
        if (currDimension >= accessedDimensions.size()) {
            return true;
        }

        const auto accessedValueOfDimension = accessedDimensions.at(currDimension);
        const bool predicateEvaluationResult = predicate(currDimension, accessedValueOfDimension, layerData);
        if (currDimension == (accessedDimensions.size() - 1)) {
            return predicateEvaluationResult;
        }
        if (!predicateEvaluationResult) {
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
    template<typename Fn>
    void applyOnLayerIfLastOrCheckNextLayer(const unsigned int currDimension, const unsigned int lastDimension, Fn&& applyLambda) {
        if (currDimension == lastDimension) {
            applyLambda(layerData);
        } else {
            for (auto& nextLayerLink: nextLayerLinks) {
                nextLayerLink->applyOnLayerIfLastOrCheckNextLayer(currDimension + 1, lastDimension, applyLambda);
            }
        }
    }

    template<typename Fn>
    void applyRecursivelyOnLayerStartingFrom(const unsigned int startDimension, const std::optional<unsigned int>& valueOfStartDimension, const unsigned int currDimension, const std::vector<std::optional<unsigned int>>& accessedDimensions, Fn&& applyLambda) {
        if (startDimension >= accessedDimensions.size()) {
            return;
        }

        const auto& accessedValueOfCurrentDimension = accessedDimensions.at(currDimension);
        if (currDimension >= startDimension) {
            applyLambda(layerData, accessedValueOfCurrentDimension);
        }

        if (currDimension < accessedDimensions.size() - 1) {
            if (accessedValueOfCurrentDimension.has_value()) {
                nextLayerLinks.at(*accessedValueOfCurrentDimension)->applyRecursivelyOnLayerStartingFrom(startDimension, valueOfStartDimension, currDimension + 1, accessedDimensions, applyLambda);
            } else {
                for (auto& nextLayerLink: nextLayerLinks) {
                    nextLayerLink->applyRecursivelyOnLayerStartingFrom(startDimension, valueOfStartDimension, currDimension + 1, accessedDimensions, applyLambda);
                }
            }
        }
    }
};

#endif