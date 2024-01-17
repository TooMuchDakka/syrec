#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/expression_substitution_generator.hpp"

#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/utils/bit_helpers.hpp"


void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::redefineNotUsableReplacementCandidates(const std::vector<syrec::VariableAccess::ptr>& notUsableReplacementCandidates) {
    dimensionReplacementStatusLookup->clear();
    for (const syrec::VariableAccess::ptr& notUsableReplacementCandidate: notUsableReplacementCandidates) {
        activateRestriction(*notUsableReplacementCandidate, true);
    }
}

void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::activateTemporaryRestrictions(const std::vector<syrec::VariableAccess::ptr>& temporaryNotUsableReplacementCandidates) {
    for (const syrec::VariableAccess::ptr& temporaryNotUsableReplacementCandidate: temporaryNotUsableReplacementCandidates) {
        activateRestriction(*temporaryNotUsableReplacementCandidate, false);
    }
}

void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::clearAllRestrictions() const {
    dimensionReplacementStatusLookup->clear();
}

void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::resetInternals() {
    clearAllRestrictions();
    orderedReplacementCandidatesBitwidthCache.clear();
    
    const std::optional<std::size_t> lastGeneratedTemporarySignalName = loadLastGeneratedTemporarySignalNameFromSymbolTable();
    lastGeneratedReplacementCandidateCounter                          = determineNextTemporarySignalPostfix(lastGeneratedTemporarySignalName);
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::ReplacementResult> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::generateReplacementFor(unsigned requiredBitwidthOfReplacement) {
    if (const std::optional<syrec::VariableAccess::ptr> foundReplacement = findReplacementCandidateBasedOnBitwidth(requiredBitwidthOfReplacement); foundReplacement.has_value()) {
        const std::optional<unsigned int> currentValueOfFoundReplacement = symbolTable->tryFetchValueForLiteral(*foundReplacement);
        if (!currentValueOfFoundReplacement.has_value()) {
            return std::nullopt;
        }
        std::optional<syrec::AssignStatement::ptr> requiredResetPriorToUsageOfReplacement;
        if (!*currentValueOfFoundReplacement) {
            if (const std::optional<unsigned int> mappedToAssignmentFlagForEnum = syrec_operation::tryMapAssignmentOperationEnumToFlag(syrec_operation::operation::MinusAssign); mappedToAssignmentFlagForEnum.has_value()) {
                const syrec::Number::ptr containerForAssignmentRhsValue = std::make_shared<syrec::Number>(*currentValueOfFoundReplacement);
                if (const syrec::NumericExpression::ptr containerForAssignmentRhsValueExpr = containerForAssignmentRhsValue ? std::make_shared<syrec::NumericExpression>(containerForAssignmentRhsValue, BitHelpers::getRequiredBitsToStoreValue(*currentValueOfFoundReplacement)) : nullptr) {
                    requiredResetPriorToUsageOfReplacement = std::make_shared<syrec::AssignStatement>(*foundReplacement, *mappedToAssignmentFlagForEnum, containerForAssignmentRhsValueExpr);
                }
            }
        }
        return ReplacementResult({.foundReplacement = *foundReplacement, .requiredResetOfReplacement = requiredResetPriorToUsageOfReplacement, .doesGeneratedReplacementReferenceExistingSignal = false});
    }
    if (const std::optional<syrec::VariableAccess::ptr> newlyGeneratedReplacement = generateNewTemporarySignalAsReplacementCandidate(requiredBitwidthOfReplacement); newlyGeneratedReplacement.has_value()) {
        return ReplacementResult({.foundReplacement = *newlyGeneratedReplacement, .requiredResetOfReplacement = std::nullopt, .doesGeneratedReplacementReferenceExistingSignal = true});
    }
    return std::nullopt;
}

syrec::Variable::vec noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::getDefinitionsForGeneratedReplacements() const {
    return generatedReplacementsLookup;
}


/*
 * NON_PUBLIC DATASTRUCTURE INTERNALS
 */
void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LeafLayerData::clearAllTemporaryRestrictions() const {
    temporaryRestrictions->liftAllRestrictions();
}

void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LeafLayerData::activatePermanentRestriction(const optimizations::BitRangeAccessRestriction::BitRangeAccess& restriction) const {
    permanentRestrictions->restrictAccessTo(restriction);
}

void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LeafLayerData::activateTemporaryRestriction(const optimizations::BitRangeAccessRestriction::BitRangeAccess& restriction) const {
    temporaryRestrictions->restrictAccessTo(restriction);
}

bool noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LeafLayerData::isAccessRestrictedToBitrange(const optimizations::BitRangeAccessRestriction::BitRangeAccess& bitRange) const {
    return temporaryRestrictions->isAccessRestrictedTo(bitRange);
}


std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LayerDataEntry::getIntermediateLayerData() const {
    if (std::holds_alternative<DimensionReplacementStatus::ptr>(data)) {
        return std::get<DimensionReplacementStatus::ptr>(data);
    }
    return std::nullopt;
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LeafLayerData::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LayerDataEntry::getLeafLayerData() const {
    if (std::holds_alternative<LeafLayerData::ptr>(data)) {
        return std::get<LeafLayerData::ptr>(data);
    }
    return std::nullopt;
}


std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::getIntermediateLayerDataForValueOfDimension(std::size_t valueOfDimension) const {
    if (const std::optional<LayerDataEntry::ptr> dataForValueOfDimension = getDataOfValueOfDimension(valueOfDimension); dataForValueOfDimension.has_value()) {
        return dataForValueOfDimension->get()->getIntermediateLayerData();
    }
    return std::nullopt;
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LeafLayerData::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::getLeafLayerDataForValueOfDimension(std::size_t valueOfDimension) const {
    if (const std::optional<LayerDataEntry::ptr> dataForValueOfDimension = getDataOfValueOfDimension(valueOfDimension); dataForValueOfDimension.has_value()) {
        return dataForValueOfDimension->get()->getLeafLayerData();
    }
    return std::nullopt;
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LayerDataEntry::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::getDataOfValueOfDimension(std::size_t valueOfDimension) const {
    const auto& dataForValueOfDimension = std::find_if(dataPerValueOfDimension.begin(), dataPerValueOfDimension.end(), [valueOfDimension](const LayerDataEntry::ptr& dataOfValueOfDimension) {
        return dataOfValueOfDimension->referencedValueOfDimension == valueOfDimension;
    });

    if (dataForValueOfDimension == dataPerValueOfDimension.end()) {
        return std::nullopt;
    }
    return *dataForValueOfDimension;
}


void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::makeValueOfDimensionAvailable(const LayerDataEntry::ptr& valueOfDimensionData) {
    if (std::optional<LayerDataEntry::ptr> dataForValueOfDimension = getDataOfValueOfDimension(valueOfDimensionData->referencedValueOfDimension); dataForValueOfDimension.has_value()) {
        *dataForValueOfDimension = valueOfDimensionData;
    } else {
        dataPerValueOfDimension.emplace_back(valueOfDimensionData);
    }

    if (!wasValueOfDimensionAlreadyLoaded(valueOfDimensionData->referencedValueOfDimension)) {
        alreadyLoadedValuesOfDimension.insert(valueOfDimensionData->referencedValueOfDimension);
    }
}

bool noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::wasValueOfDimensionAlreadyLoaded(std::size_t valueOfDimension) const {
    return alreadyLoadedValuesOfDimension.count(valueOfDimension);
}

/*
 * START OF NON_PUBLIC FUNCTIONALITY
 */
void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::activateRestriction(const syrec::VariableAccess& restrictionToCreate, bool shouldRestrictionLiveUntilNextReset) {
    const std::optional<syrec::Variable::ptr>    symbolTableInformationOfReferencedSignal = getSignalInformationFromSymbolTable(restrictionToCreate.var->name);
    const std::optional<TransformedSignalAccess> transformedSignalAccess                  = symbolTableInformationOfReferencedSignal.has_value() ? transformSignalAccess(restrictionToCreate, **symbolTableInformationOfReferencedSignal) : std::nullopt;
    if (!transformedSignalAccess.has_value()) {
        return;
    }

    const std::optional<DimensionReplacementStatus::ptr> topMostRestrictionLayerForCandidate = getOrCreateEntryForRestriction(**symbolTableInformationOfReferencedSignal);
    if (!topMostRestrictionLayerForCandidate.has_value()) {
        return;
    }

    if (const std::optional<DimensionReplacementStatus::ptr> leafLayerForCandidate = advanceRestrictionLookupToLeafLayer(*topMostRestrictionLayerForCandidate, transformedSignalAccess->accessedValuePerDimension, restrictionToCreate.var->bitwidth); leafLayerForCandidate.has_value()) {
        if (const DimensionReplacementStatus::LeafLayerData::ptr leafLayerDataForCandidate = leafLayerForCandidate->get()->getLeafLayerDataForValueOfDimension(transformedSignalAccess->accessedValuePerDimension.back()).value_or(nullptr); leafLayerDataForCandidate) {
            if (shouldRestrictionLiveUntilNextReset) {
                leafLayerDataForCandidate->activatePermanentRestriction(transformedSignalAccess->accessedBitRange);
            } else {
                leafLayerDataForCandidate->activateTemporaryRestriction(transformedSignalAccess->accessedBitRange);
            }
        }
    }
}

void noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::cacheSignalBitwidth(const std::string& signalIdent, unsigned signalBitwidth) {
    std::optional<std::reference_wrapper<SignalBitwidthGroup>> matchingGroupForSignalBitwidth = getOrCreateCacheEntryForSignalBitwdith(signalBitwidth);
    if (!matchingGroupForSignalBitwidth.has_value()) {
        return;
    }

    std::vector<std::string> cachedSignalIdentsInGroup = matchingGroupForSignalBitwidth->get().signalIdents;
    if (std::find_if(cachedSignalIdentsInGroup.cbegin(), cachedSignalIdentsInGroup.cend(), [&signalIdent](const std::string& cachedSignalIdent) { return signalIdent == cachedSignalIdent; }) == cachedSignalIdentsInGroup.cend()) {
        cachedSignalIdentsInGroup.emplace_back(signalIdent);
    }
}

std::optional<std::reference_wrapper<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::SignalBitwidthGroup>> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::getOrCreateCacheEntryForSignalBitwdith(unsigned int bitwidth) {
    if (!bitwidth) {
        return std::nullopt;
    }

    const auto& matchingGroupForSignalBitwidth = std::find_if(
    orderedReplacementCandidatesBitwidthCache.begin(),
    orderedReplacementCandidatesBitwidthCache.end(),
    [bitwidth](const SignalBitwidthGroup& signalBitwidthGroup) {
        return signalBitwidthGroup.bitwidth >= bitwidth;
    });

    if (matchingGroupForSignalBitwidth == orderedReplacementCandidatesBitwidthCache.end()) {
        auto newCacheEntry = SignalBitwidthGroup({.bitwidth = bitwidth, .signalIdents = {}});
        orderedReplacementCandidatesBitwidthCache.emplace_back(newCacheEntry);
        return orderedReplacementCandidatesBitwidthCache.back();
    }
    return *matchingGroupForSignalBitwidth;
}

std::optional<unsigned> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::getSignalBitwidth(const std::string& signalIdent) const {
    if (const std::optional<syrec::Variable::ptr> symbolTableInformationOfReferencedSignal = getSignalInformationFromSymbolTable(signalIdent); symbolTableInformationOfReferencedSignal.has_value()) {
        return symbolTableInformationOfReferencedSignal->get()->bitwidth;
    }
    return std::nullopt;
}

std::optional<syrec::Variable::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::getSignalInformationFromSymbolTable(const std::string_view& signalIdent) const {
    std::optional<std::variant<syrec::Variable::ptr, syrec::Number::ptr>> symbolTableEntryForSignalIdent = symbolTable->getVariable(signalIdent);
    if (symbolTableEntryForSignalIdent.has_value() && std::holds_alternative<syrec::Variable::ptr>(*symbolTableEntryForSignalIdent)) {
        return std::get<syrec::Variable::ptr>(*symbolTableEntryForSignalIdent);
    }
    return std::nullopt;
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::TransformedSignalAccess> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::transformSignalAccess(const syrec::VariableAccess& signalAccess, const syrec::Variable& symbolTableEntryForReferencedSignal) const {
    std::vector<unsigned int> transformedDimensionAccess    = std::vector(symbolTableEntryForReferencedSignal.dimensions.size(), 0u);
    std::size_t               transformedDimensionAccessIdx                = 0;
    for (const syrec::expression::ptr& exprDefiningAccessedValueOfDimension : signalAccess.indexes) {
        if (const auto& exprAsNumericExpr = std::dynamic_pointer_cast<const syrec::NumericExpression>(exprDefiningAccessedValueOfDimension); exprAsNumericExpr && exprAsNumericExpr->value->isConstant()) {
            transformedDimensionAccess.at(transformedDimensionAccessIdx++) = exprAsNumericExpr->value->evaluate({});
        } else {
            return std::nullopt;
        }
    }

    auto transformedBitrangeAccess = optimizations::BitRangeAccessRestriction::BitRangeAccess(0, symbolTableEntryForReferencedSignal.bitwidth - 1);
    if (signalAccess.range.has_value()) {
        const syrec::Number::ptr accessedBitRangeStart = std::dynamic_pointer_cast<syrec::Number>(signalAccess.range->first);
        const syrec::Number::ptr accessedBitRangeEnd = accessedBitRangeStart ? std::dynamic_pointer_cast<syrec::Number>(signalAccess.range->second) : nullptr;
        if (!accessedBitRangeEnd || !accessedBitRangeStart->isConstant() || !accessedBitRangeEnd->isConstant()) {
            return std::nullopt;
        }
        transformedBitrangeAccess.first = accessedBitRangeStart->evaluate({});
        transformedBitrangeAccess.second = accessedBitRangeEnd->evaluate({});
    }
    return TransformedSignalAccess({.signalIdent = signalAccess.var->name, .accessedValuePerDimension = transformedDimensionAccess, .accessedBitRange = transformedBitrangeAccess});
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::getOrCreateEntryForRestriction(const syrec::Variable& symbolTableEntryForReferencedSignal) {
    if (const DimensionReplacementStatus::ptr existingEntryMatchingSignalIdent = dimensionReplacementStatusLookup->at(symbolTableEntryForReferencedSignal.name)) {
        return existingEntryMatchingSignalIdent;
    }

    const auto defaultAccessedValuePerDimension = std::vector(symbolTableEntryForReferencedSignal.dimensions.size(), 0u);
    if (const std::optional<DimensionReplacementStatus::ptr> generatedLookupForGivenAccess = generateDefaultDimensionReplacementStatusForIntermediateLayer(defaultAccessedValuePerDimension, 0, symbolTableEntryForReferencedSignal.bitwidth); generatedLookupForGivenAccess.has_value()) {
        dimensionReplacementStatusLookup->insert(std::make_pair(symbolTableEntryForReferencedSignal.name, *generatedLookupForGivenAccess));
        if (const std::optional<std::reference_wrapper<SignalBitwidthGroup>>& groupOfSignalsSharingSameBitwidth = getOrCreateCacheEntryForSignalBitwdith(symbolTableEntryForReferencedSignal.bitwidth); groupOfSignalsSharingSameBitwidth.has_value()) {
            groupOfSignalsSharingSameBitwidth->get().signalIdents.emplace_back(symbolTableEntryForReferencedSignal.name);
        }

        return *generatedLookupForGivenAccess;
    }
    return std::nullopt;
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::generateDefaultDimensionReplacementStatusForIntermediateLayer(const std::vector<unsigned>& declaredValuesPerDimension, std::size_t currDimensionToProcess, unsigned int bitwidthOfAccessedSignal) {
    if (currDimensionToProcess > declaredValuesPerDimension.size()) {
        return std::nullopt;
    }

    DimensionReplacementStatus::ptr generatedContainerForIntermediateLayer = std::make_shared<DimensionReplacementStatus>();
    if (!generatedContainerForIntermediateLayer) {
        return std::nullopt;
    }
    generatedContainerForIntermediateLayer->lastLoadedValueOfDimension     = 0;
    generatedContainerForIntermediateLayer->alreadyLoadedValuesOfDimension = std::unordered_set<std::size_t>();

    const bool  isIntermediateDimension       = currDimensionToProcess == declaredValuesPerDimension.size();
    bool        didProcessingCompleteNormally = false;
    if (!isIntermediateDimension) {
        if (const std::optional<DimensionReplacementStatus::LeafLayerData::ptr>& generatedDataForValueOfLeafLayer = generateDefaultDimensionReplacementStatusForValueOfDimensionInLeafLayer(bitwidthOfAccessedSignal); generatedDataForValueOfLeafLayer.has_value()) {
            if (const DimensionReplacementStatus::LayerDataEntry::ptr container = std::make_shared<DimensionReplacementStatus::LayerDataEntry>(DimensionReplacementStatus::LayerDataEntry({0, *generatedDataForValueOfLeafLayer})); container) {
                generatedContainerForIntermediateLayer->makeValueOfDimensionAvailable(container);
                didProcessingCompleteNormally = true;
            }
        }
    } else {
        if (const std::optional<DimensionReplacementStatus::ptr>& generatedDataForValueOfIntermediateLayer = generateDefaultDimensionReplacementStatusForIntermediateLayer(declaredValuesPerDimension, ++currDimensionToProcess, bitwidthOfAccessedSignal); generatedDataForValueOfIntermediateLayer.has_value()) {
            if (const DimensionReplacementStatus::LayerDataEntry::ptr container = std::make_shared<DimensionReplacementStatus::LayerDataEntry>(DimensionReplacementStatus::LayerDataEntry({0, *generatedDataForValueOfIntermediateLayer})); container) {
                generatedContainerForIntermediateLayer->makeValueOfDimensionAvailable(container);
                didProcessingCompleteNormally = true;
            }
        }
    }

    if (!didProcessingCompleteNormally) {
        return std::nullopt;
    }
    return generatedContainerForIntermediateLayer;
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::LeafLayerData::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::generateDefaultDimensionReplacementStatusForValueOfDimensionInLeafLayer(unsigned bitwidthOfAccessedSignal) {
    const optimizations::BitRangeAccessRestriction::ptr permanentPotentialReplacementStatus            = nullptr;
    const optimizations::BitRangeAccessRestriction::ptr temporaryPotentialReplacementRestrictionStatus = std::make_shared<optimizations::BitRangeAccessRestriction>(bitwidthOfAccessedSignal);
    if (!temporaryPotentialReplacementRestrictionStatus) {
        return std::nullopt;
    }
    if (const auto& generatedLeafDataForValueOfDimension = std::make_shared<DimensionReplacementStatus::LeafLayerData>(DimensionReplacementStatus::LeafLayerData({permanentPotentialReplacementStatus, temporaryPotentialReplacementRestrictionStatus})); generatedLeafDataForValueOfDimension) {
        return generatedLeafDataForValueOfDimension;
    }
    return std::nullopt;
}

std::optional<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::DimensionReplacementStatus::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::advanceRestrictionLookupToLeafLayer(const DimensionReplacementStatus::ptr& topMostRestrictionLayer, const std::vector<unsigned>& dimensionAccessDefiningPathToLeafLayer, unsigned int bitwidthOfAccessedSignal) {
    DimensionReplacementStatus::ptr currentDimensionReplacementStatus = topMostRestrictionLayer;
    for (std::size_t i = 0; i < dimensionAccessDefiningPathToLeafLayer.size(); ++i) {
        const std::size_t accessedValueOfDimension = dimensionAccessDefiningPathToLeafLayer.at(i);
        const bool        isIntermediateDimension  = i < (dimensionAccessDefiningPathToLeafLayer.size() - 1);
        if (!currentDimensionReplacementStatus->dataPerValueOfDimension.at(accessedValueOfDimension)) {
            DimensionReplacementStatus::LayerDataEntry::ptr containerForLoadedValueOfDimension;
            if (isIntermediateDimension) {
                std::vector<unsigned int> accessedValuePerDimensionOfRemainingDimensionAccess = std::vector(std::next(dimensionAccessDefiningPathToLeafLayer.cbegin(), i + 1), dimensionAccessDefiningPathToLeafLayer.cend());
                if (const std::optional<DimensionReplacementStatus::ptr> containerForIntermediateDimensions = generateDefaultDimensionReplacementStatusForIntermediateLayer(accessedValuePerDimensionOfRemainingDimensionAccess, 0, bitwidthOfAccessedSignal); containerForIntermediateDimensions.has_value()) {
                    containerForLoadedValueOfDimension = std::make_shared<DimensionReplacementStatus::LayerDataEntry>(DimensionReplacementStatus::LayerDataEntry({accessedValueOfDimension, *containerForIntermediateDimensions}));
                }
            } else {
                if (const std::optional<DimensionReplacementStatus::LeafLayerData::ptr> containerForLeafLayerDimensions = generateDefaultDimensionReplacementStatusForValueOfDimensionInLeafLayer(bitwidthOfAccessedSignal); containerForLeafLayerDimensions.has_value()) {
                    containerForLoadedValueOfDimension = std::make_shared<DimensionReplacementStatus::LayerDataEntry>(DimensionReplacementStatus::LayerDataEntry({accessedValueOfDimension, *containerForLeafLayerDimensions}));
                }
            }

            if (!containerForLoadedValueOfDimension) {
                break;
            }
            currentDimensionReplacementStatus->dataPerValueOfDimension.at(accessedValueOfDimension) = containerForLoadedValueOfDimension;
        } else if (!isIntermediateDimension) {
            return currentDimensionReplacementStatus;
        }

        if (const std::optional<DimensionReplacementStatus::ptr> dataOfNextDimension = currentDimensionReplacementStatus->getIntermediateLayerDataForValueOfDimension(accessedValueOfDimension); dataOfNextDimension.has_value()) {
            currentDimensionReplacementStatus = *dataOfNextDimension;
        } else {
            break;
        }
    }
    return std::nullopt;
}


std::optional<syrec::VariableAccess::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::findReplacementCandidateBasedOnBitwidth(unsigned requiredMinimumBitwidth) {
    auto                      currentGroupOfSignalsMatchingBitwidth                             = getFirstGroupOfSignalsWithEqualOrLargerBitwidth(requiredMinimumBitwidth);
    std::vector<unsigned int> potentialLoadableFutureReplacementCandidateGroupsFromSymbolTable  = symbolTable->fetchBitwidthsPerSharedBitwidthSignalGroupsWithAssignableSignals(requiredMinimumBitwidth);
    auto                      lastLoadedPotentialLoadableFutureReplacementCandidateGroupIterator = potentialLoadableFutureReplacementCandidateGroupsFromSymbolTable.begin();

    do {
        for (const std::string& signalIdentInGroup : currentGroupOfSignalsMatchingBitwidth->signalIdents) {
            if (const std::optional<syrec::Variable::ptr> symbolTableInformationForSignal = getSignalInformationFromSymbolTable(signalIdentInGroup); symbolTableInformationForSignal.has_value()) {
                if (const std::optional<syrec::VariableAccess::ptr> createdReplacement = createReplacementFromCandidate(*symbolTableInformationForSignal, requiredMinimumBitwidth); createdReplacement.has_value()) {
                    return *createdReplacement;
                }   
            }
        }

        if (lastLoadedPotentialLoadableFutureReplacementCandidateGroupIterator != potentialLoadableFutureReplacementCandidateGroupsFromSymbolTable.end() && *lastLoadedPotentialLoadableFutureReplacementCandidateGroupIterator == currentGroupOfSignalsMatchingBitwidth->bitwidth) {
            if (const std::optional<syrec::VariableAccess::ptr> createdReplacement = searchForReplacementInPotentialReplacementCandidatesFromSymbolTableWithMatchingBitwidth(*lastLoadedPotentialLoadableFutureReplacementCandidateGroupIterator); createdReplacement.has_value()) {
                return *createdReplacement;
            }
            ++lastLoadedPotentialLoadableFutureReplacementCandidateGroupIterator;
        }
    } while (advanceIteratorToNextGroup(currentGroupOfSignalsMatchingBitwidth));

    while (lastLoadedPotentialLoadableFutureReplacementCandidateGroupIterator != potentialLoadableFutureReplacementCandidateGroupsFromSymbolTable.end()) {
        if (const std::optional<syrec::VariableAccess::ptr> createdReplacement = searchForReplacementInPotentialReplacementCandidatesFromSymbolTableWithMatchingBitwidth(*lastLoadedPotentialLoadableFutureReplacementCandidateGroupIterator); createdReplacement.has_value()) {
            return *createdReplacement;
        }
        ++lastLoadedPotentialLoadableFutureReplacementCandidateGroupIterator;
    }
    return std::nullopt;
}

std::vector<noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::SignalBitwidthGroup>::iterator noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::getFirstGroupOfSignalsWithEqualOrLargerBitwidth(unsigned bitwidth) {
    return std::find_if(
    orderedReplacementCandidatesBitwidthCache.begin(),
    orderedReplacementCandidatesBitwidthCache.end(),
    [bitwidth](const SignalBitwidthGroup& signalBitwidthGroup) {
        return signalBitwidthGroup.bitwidth >= bitwidth;
    });
}

bool noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::advanceIteratorToNextGroup(std::vector<SignalBitwidthGroup>::iterator& iterator) const {
    return ++iterator != orderedReplacementCandidatesBitwidthCache.end();
}

std::optional<syrec::VariableAccess> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::generateSignalAccessForNotLoadedEntryFromSymbolTableForGivenBitwidth(const syrec::Variable::ptr& symbolTableEntry, unsigned requiredBitwidth) {
    syrec::VariableAccess generatedSignalAccess;
    generatedSignalAccess.var     = symbolTableEntry;
    generatedSignalAccess.indexes = std::vector<syrec::expression::ptr>(symbolTableEntry->dimensions.size(), nullptr);
    for (std::size_t i = 0; i < generatedSignalAccess.indexes.size(); ++i) {
        const syrec::Number::ptr            accessedValueOfDimension                   = std::make_shared<syrec::Number>(0);
        const syrec::NumericExpression::ptr containerForExprOfAccessedValueOfDimension = accessedValueOfDimension ? std::make_shared<syrec::NumericExpression>(accessedValueOfDimension, 1) : nullptr;
        if (!containerForExprOfAccessedValueOfDimension) {
            return std::nullopt;
        }
        generatedSignalAccess.indexes.at(i)                                            = containerForExprOfAccessedValueOfDimension;
    }

    const syrec::Number::ptr bitRangeAccessStart = std::make_shared<syrec::Number>(0);
    const syrec::Number::ptr bitRangeAccessEnd   = std::make_shared<syrec::Number>(requiredBitwidth - 1);
    if (!bitRangeAccessStart || !bitRangeAccessEnd) {
        return std::nullopt;
    }
    generatedSignalAccess.range = std::make_pair(bitRangeAccessStart, bitRangeAccessEnd);
    return generatedSignalAccess;
}


std::optional<syrec::VariableAccess::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::searchForReplacementInPotentialReplacementCandidatesFromSymbolTableWithMatchingBitwidth(unsigned bitwidth) {
    for (const syrec::Variable::ptr& potentialFutureReplacementCandidateFromSymbolTable: symbolTable->fetchedDeclaredAssignableSignalsHavingMatchingBitwidth(bitwidth)) {
        const std::optional<syrec::VariableAccess>   accessOnFutureReplacementCandidate                  = generateSignalAccessForNotLoadedEntryFromSymbolTableForGivenBitwidth(potentialFutureReplacementCandidateFromSymbolTable, bitwidth);
        const std::optional<TransformedSignalAccess> transformedSignalAccessOnFutureReplacementCandidate = accessOnFutureReplacementCandidate.has_value() ? transformSignalAccess(*accessOnFutureReplacementCandidate, *potentialFutureReplacementCandidateFromSymbolTable) : std::nullopt;

        //currentGroupOfSignalsMatchingBitwidth->signalIdents.emplace_back(potentialFutureReplacementCandidateFromSymbolTable->name);
        if (transformedSignalAccessOnFutureReplacementCandidate.has_value()) {
            getOrCreateEntryForRestriction(*potentialFutureReplacementCandidateFromSymbolTable);
            if (const std::optional<syrec::VariableAccess::ptr> createdReplacement = createReplacementFromCandidate(potentialFutureReplacementCandidateFromSymbolTable, bitwidth); createdReplacement.has_value()) {
                return *createdReplacement;
            }
        }
    }
    return std::nullopt;
}

std::optional<syrec::VariableAccess::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::createReplacementFromCandidate(const syrec::Variable::ptr& symbolTableEntryForReplacementCandidate, unsigned requiredMinimumBitwidthOfReplacement) {
    std::optional<SignalAccessReplacementSearchData> signalReplacementSearchData = SignalAccessReplacementSearchData::createFrom(symbolTableEntryForReplacementCandidate, requiredMinimumBitwidthOfReplacement);
    if (!signalReplacementSearchData.has_value()) {
        return std::nullopt;
    }

    const std::optional<DimensionReplacementStatus::ptr> topMostRestrictionLayer = getOrCreateEntryForRestriction(*symbolTableEntryForReplacementCandidate);
    if (!topMostRestrictionLayer.has_value()) {
        return std::nullopt;
    }

    std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess> foundUsableBitrangeAsReplacement;
    do {
        advanceRestrictionLookupToLeafLayer(*topMostRestrictionLayer, signalReplacementSearchData->currentValuePerAccessedDimension, symbolTableEntryForReplacementCandidate->bitwidth);
        foundUsableBitrangeAsReplacement = signalReplacementSearchData->determineBitrangeUsableForReplacement(*topMostRestrictionLayer);
    } while (!foundUsableBitrangeAsReplacement.has_value() && signalReplacementSearchData->advanceToNextProcessableValueOfDimension());

    if (!foundUsableBitrangeAsReplacement.has_value()) {
        return std::nullopt;
    }

    const syrec::VariableAccess::ptr generatedReplacementCandidate = std::make_shared<syrec::VariableAccess>();
    generatedReplacementCandidate->var                       = symbolTableEntryForReplacementCandidate;
    generatedReplacementCandidate->indexes                         = std::vector<syrec::expression::ptr>(symbolTableEntryForReplacementCandidate->dimensions.size(), nullptr);
    for (std::size_t i = 0; i < symbolTableEntryForReplacementCandidate->dimensions.size(); ++i) {
        const unsigned int                  accessedValueOfDimension                   = signalReplacementSearchData->currentValuePerAccessedDimension.at(i);
        const syrec::Number::ptr            containerForAccessedValueOfDimension       = std::make_shared<syrec::Number>(accessedValueOfDimension);
        const syrec::NumericExpression::ptr containerForExprOfAccessedValueOfDimension = accessedValueOfDimension ? std::make_shared<syrec::NumericExpression>(containerForAccessedValueOfDimension, BitHelpers::getRequiredBitsToStoreValue(accessedValueOfDimension)) : nullptr;
        if (!containerForExprOfAccessedValueOfDimension) {
            return std::nullopt;
        }
        generatedReplacementCandidate->indexes.at(i) = containerForExprOfAccessedValueOfDimension;
    }

    const syrec::Number::ptr containerForReplacementBitrangeStart = std::make_shared<syrec::Number>(foundUsableBitrangeAsReplacement->first);
    const syrec::Number::ptr containerForReplacementBitrangeEnd = std::make_shared<syrec::Number>(foundUsableBitrangeAsReplacement->second);
    if (!containerForReplacementBitrangeStart || !containerForReplacementBitrangeEnd) {
        return std::nullopt;
    }
    generatedReplacementCandidate->range = std::make_pair(containerForReplacementBitrangeStart, containerForReplacementBitrangeEnd);
    return generatedReplacementCandidate;
}

bool noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::SignalAccessReplacementSearchData::incrementProcessedValueOfDimensionAndCheckForOverflow() {
    const std::size_t maxAllowedValue = std::min(UINT_MAX, symbolTableInformationOfReplacementCandidate->bitwidth);
    if (currentValuePerAccessedDimension.at(currentlyProcessedDimension) + 1 >= maxAllowedValue) {
        currentlyProcessedDimension = currentlyProcessedDimension ? --currentlyProcessedDimension : 0;
        for (std::size_t i = currentlyProcessedDimension; i < currentValuePerAccessedDimension.size(); ++i) {
            currentValuePerAccessedDimension.at(i) = 0;
        }
        return true;
    }
    currentValuePerAccessedDimension.at(currentlyProcessedDimension)++;
    return false;
}

bool noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::SignalAccessReplacementSearchData::advanceToNextProcessableValueOfDimension() {
    if (!incrementProcessedValueOfDimensionAndCheckForOverflow()) {
        return true;
    }

    const std::size_t remainingDimensionsToCheck = currentlyProcessedDimension + 1;
    bool              couldAdvanceToNextValueOfDimension = false;
    for (std::size_t i = 0; i <= remainingDimensionsToCheck && !couldAdvanceToNextValueOfDimension; ++i) {
        couldAdvanceToNextValueOfDimension = !incrementProcessedValueOfDimensionAndCheckForOverflow();
    }
    return couldAdvanceToNextValueOfDimension;
}

std::optional<optimizations::BitRangeAccessRestriction::BitRangeAccess> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::SignalAccessReplacementSearchData::determineBitrangeUsableForReplacement(const DimensionReplacementStatus::ptr& topMostRestrictionLayer) {
    const std::optional<DimensionReplacementStatus::ptr>                leafLayerData                               = advanceRestrictionLookupToLeafLayer(topMostRestrictionLayer, currentValuePerAccessedDimension, (activeBitRangeAccess.second - activeBitRangeAccess.first) + 1);
    const std::optional<DimensionReplacementStatus::LeafLayerData::ptr> restrictionsStatusForCurrentDimensionAccess = leafLayerData.has_value() ? leafLayerData->get()->getLeafLayerDataForValueOfDimension(currentValuePerAccessedDimension.at(currentlyProcessedDimension)) : std::nullopt;
    if (!restrictionsStatusForCurrentDimensionAccess.has_value()) {
        return std::nullopt;
    }

    optimizations::BitRangeAccessRestriction::BitRangeAccess copyOfActiveBitrange = activeBitRangeAccess;
    for (std::size_t i = 0; i < numPossibleBitRangeWindowAdvances; ++i) {
        if (restrictionsStatusForCurrentDimensionAccess->get()->isAccessRestrictedToBitrange(copyOfActiveBitrange)) {
            copyOfActiveBitrange.first++;
            copyOfActiveBitrange.second++;
        } else {
            restrictionsStatusForCurrentDimensionAccess->get()->activateTemporaryRestriction(copyOfActiveBitrange);
            return copyOfActiveBitrange;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::loadLastGeneratedTemporarySignalNameFromSymbolTable() const {
    const std::vector<std::string> symbolTableEntriesMatchingPrefix = symbolTable->fetchIdentsOfSignalsStartingWith(DEFAULT_GENERATED_REPLACEMENT_CANDIDATE_PREFIX);
    std::vector<std::size_t>       extractedPostfixesOfSymbolTableEntries;
    for (const std::string& symbolTableEntryIdent : symbolTableEntriesMatchingPrefix) {
        if (const std::optional<std::size_t> splitOfPostfixFromSignalIdent = determinePostfixOfNewlyGeneratedReplacementSignalIdent(symbolTableEntryIdent); splitOfPostfixFromSignalIdent.has_value()) {
            extractedPostfixesOfSymbolTableEntries.emplace_back(*splitOfPostfixFromSignalIdent);
        }
    }

    // Sorts the container in ascending order
    std::sort(extractedPostfixesOfSymbolTableEntries.begin(), extractedPostfixesOfSymbolTableEntries.end());
    if (extractedPostfixesOfSymbolTableEntries.empty()) {
        return std::nullopt;
    }
    return extractedPostfixesOfSymbolTableEntries.back();
}

std::optional<std::string> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::generateNextTemporarySignalName() {
    lastGeneratedReplacementCandidateCounter = determineNextTemporarySignalPostfix(lastGeneratedReplacementCandidateCounter);
    if (lastGeneratedReplacementCandidateCounter.has_value()) {
        return std::string(DEFAULT_GENERATED_REPLACEMENT_CANDIDATE_PREFIX) + std::to_string(*lastGeneratedReplacementCandidateCounter);
    }
    return std::nullopt;
}

std::optional<syrec::VariableAccess::ptr> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::generateNewTemporarySignalAsReplacementCandidate(unsigned int requiredBitwidth) {
    std::optional<std::reference_wrapper<SignalBitwidthGroup>> matchingBitwidthSignalGroup = getOrCreateCacheEntryForSignalBitwdith(requiredBitwidth);
    if (!matchingBitwidthSignalGroup.has_value()) {
        return std::nullopt;
    }

    const std::optional<std::string> temporarySignalName = generateNextTemporarySignalName();
    if (!temporarySignalName.has_value()) {
        return std::nullopt;
    }

    const syrec::Variable::ptr generatedReplacementSignal         = std::make_shared<syrec::Variable>(syrec::Variable::Types::Wire, *temporarySignalName, std::vector(1, 1u), requiredBitwidth);
    syrec::VariableAccess::ptr generatedSignalAccessOnReplacement = generatedReplacementSignal ? std::make_shared<syrec::VariableAccess>() : nullptr;
    if (!generatedSignalAccessOnReplacement) {
        return std::nullopt;
    }

    generatedSignalAccessOnReplacement->var = generatedReplacementSignal;
    generatedSignalAccessOnReplacement->range = std::nullopt;

    constexpr unsigned int              accessedValueOfDimension               = 0;
    const syrec::Number::ptr            accessedValueOfDimensionOfReplacement  = std::make_shared<syrec::Number>(accessedValueOfDimension);
    const syrec::NumericExpression::ptr containerForAccessedValueOfReplacement = accessedValueOfDimensionOfReplacement ? std::make_shared<syrec::NumericExpression>(accessedValueOfDimensionOfReplacement, BitHelpers::getRequiredBitsToStoreValue(accessedValueOfDimension)) : nullptr;
    if (!containerForAccessedValueOfReplacement) {
        return std::nullopt;
    }

    generatedSignalAccessOnReplacement->indexes = std::vector(1, containerForAccessedValueOfReplacement);
    return generatedSignalAccessOnReplacement;
}

std::optional<std::size_t> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::determinePostfixOfNewlyGeneratedReplacementSignalIdent(const std::string_view& signalIdent) const {
    if (!signalIdent.rfind(DEFAULT_GENERATED_REPLACEMENT_CANDIDATE_PREFIX, 0)) {
        
    }
    return std::nullopt;
}

std::optional<std::size_t> noAdditionalLineSynthesis::ExpressionSubstitutionGenerator::determineNextTemporarySignalPostfix(const std::optional<std::size_t>& lastGeneratedTemporarySignalPostfix) {
    if (!lastGeneratedTemporarySignalPostfix.has_value() || *lastGeneratedTemporarySignalPostfix == UINT_MAX) {
        return std::nullopt;
    }
    return *lastGeneratedTemporarySignalPostfix + 1;
}