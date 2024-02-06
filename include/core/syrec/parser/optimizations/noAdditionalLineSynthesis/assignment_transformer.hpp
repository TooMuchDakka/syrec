#ifndef ASSIGNMENT_TRANSFORMER_HPP
#define ASSIGNMENT_TRANSFORMER_HPP
#pragma once

#include "expression_to_subassignment_splitter.hpp"
#include "core/syrec/statement.hpp"
#include "core/syrec/parser/operation.hpp"

#include <functional>
#include <optional>
#include <unordered_map>

namespace noAdditionalLineSynthesis {
    class AssignmentTransformer {
    public:
        using ptr = std::unique_ptr<AssignmentTransformer>;
        using InitialSignalValueLookupCallback = std::function<std::optional<unsigned int>(syrec::VariableAccess& accessedSignalParts)>;
        using SharedAssignmentReference = std::variant<std::shared_ptr<syrec::AssignStatement>, std::shared_ptr<syrec::UnaryStatement>>;

        AssignmentTransformer() {
            assignmentRhsExprSplitter = std::make_unique<ExpressionToSubAssignmentSplitter>();
        }

        virtual ~AssignmentTransformer() = default;
        [[nodiscard]] std::vector<SharedAssignmentReference> simplify(const std::vector<SharedAssignmentReference>& assignmentsToSimplify, const InitialSignalValueLookupCallback& initialSignalValueLookupCallback);
        
    protected:
        using AwaitedInversionsByBinaryAssignmentLookupEntry = std::unordered_map<syrec::expression::ptr, syrec_operation::operation>;
        using AwaitedInversionOfAssignedToSignalPartsByBinaryAssignmentLookup = std::unordered_map<syrec::VariableAccess::ptr, AwaitedInversionsByBinaryAssignmentLookupEntry>;

        struct AwaitedInversionsByUnaryAssignmentLookupEntry {
            syrec_operation::operation awaitedInvertedAssignmentOperation;
            std::size_t                numWaits;
        };
        using AwaitedInversionsOfAssignedToSignalPartsByUnaryAssignmentLookup = std::unordered_map<syrec::VariableAccess::ptr, AwaitedInversionsByUnaryAssignmentLookupEntry>;

        struct SignalValueLookupCacheEntry {
            enum ActiveValueEntry {
                Original,
                Modifiable,
                None
            };

            ActiveValueEntry            activeValueEntry;
            std::optional<unsigned int> valuePriorToAnyAssignment;
            std::optional<unsigned int> currentValue;

            explicit SignalValueLookupCacheEntry(const std::optional<unsigned>& originalSignalValue):
                activeValueEntry(originalSignalValue.has_value() ? Original : None), valuePriorToAnyAssignment(originalSignalValue), currentValue(originalSignalValue) {}
            
            [[nodiscard]] std::optional<unsigned int> getValue() const;
            [[nodiscard]] ActiveValueEntry            getTypeOfActiveValue() const;
            void                                      updateValueTo(const std::optional<unsigned int>& newValue);
            void                                      invalidateCurrentValue();
            void                                      markInitialValueAsActive();
            void                                      markValueAsModifiable();
        };
        using SignalValueLookupCacheEntryReference = std::shared_ptr<SignalValueLookupCacheEntry>;
        using SignalValueLookupCache = std::unordered_map<syrec::VariableAccess::ptr, SignalValueLookupCacheEntryReference>;

        AwaitedInversionsOfAssignedToSignalPartsByUnaryAssignmentLookup awaitedInversionsOfUnaryAssignmentsLookup;
        AwaitedInversionOfAssignedToSignalPartsByBinaryAssignmentLookup awaitedInversionsOfBinaryAssignmentsLookup;
        SignalValueLookupCache                                          signalValueLookupCache;
        ExpressionToSubAssignmentSplitter::ptr                          assignmentRhsExprSplitter;

        virtual void       resetInternals();
        void               addWatchForInversionOfAssignment(const syrec::AssignStatement& assignment);
        void               addWatchForInversionOfAssignment(const syrec::UnaryStatement& assignment);
        void               tryRemoveWatchForInversionOfAssignment(const syrec::AssignStatement& assignment, bool& wasLastExistingWatchRemoved);
        void               tryRemoveWatchForInversionOfAssignment(const syrec::UnaryStatement& assignment);
        void               updateEntryInValueLookupCacheByPerformingAssignment(const syrec::VariableAccess::ptr& assignedToSignalParts, const std::optional<syrec_operation::operation>& assignmentOperation, const syrec::expression& expr) const;
        void               invalidateSignalValueLookupEntryFor(const syrec::VariableAccess::ptr& assignedToSignalParts) const;
        void               handleAllExpectedInversionsOfAssignedToSignalPartsDefined(const syrec::VariableAccess::ptr& assignedToSignalParts) const;
        void               createSignalValueLookupCacheEntry(const syrec::VariableAccess::ptr& assignedToSignalParts, const InitialSignalValueLookupCallback& initialSignalValueLookupCallback);

        [[nodiscard]] bool                                                         areMoreInversionsRequiredToReachOriginalSignalValue(const syrec::VariableAccess::ptr& accessedSignalParts) const;
        [[nodiscard]] bool                                                         doesAssignmentMatchWatchForInvertedAssignment(const syrec::AssignStatement& assignment) const;
        [[nodiscard]] bool                                                         doesAssignmentMatchWatchForInvertedAssignment(const syrec::UnaryStatement& assignment) const;
        [[nodiscard]] std::optional<unsigned int>                                  fetchCurrentValueOfAssignedToSignal(const syrec::VariableAccess::ptr& accessedSignalParts) const;
        [[nodiscard]] syrec::AssignStatement::vec                                  trySplitAssignmentRhsExpr(const syrec::AssignStatement::ptr& assignment) const;
        [[nodiscard]] SignalValueLookupCacheEntryReference                         getSignalValueCacheEntryFor(const syrec::VariableAccess::ptr& assignedToSignalParts) const;
        [[nodiscard]] static std::optional<std::shared_ptr<syrec::UnaryStatement>> tryReplaceBinaryAssignmentWithUnaryOne(const syrec::AssignStatement& assignment);
    };
}
#endif