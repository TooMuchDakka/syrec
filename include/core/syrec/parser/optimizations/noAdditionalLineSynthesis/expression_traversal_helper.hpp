#ifndef EXPRESSION_TRAVERSAL_HELPER_HPP
#define EXPRESSION_TRAVERSAL_HELPER_HPP
#pragma once

#include "core/syrec/expression.hpp"
#include "core/syrec/parser/operation.hpp"
#include "core/syrec/parser/symbol_table.hpp"

#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace noAdditionalLineSynthesis {
    class ExpressionTraversalHelper {
    public:
        using ptr = std::shared_ptr<ExpressionTraversalHelper>;

        struct OperandNode {
            std::size_t                id;
            std::optional<std::size_t> operationNodeId;

            [[nodiscard]] bool isLeafNode() const;
        };

        struct OperationNode {
            std::size_t                id;
            std::optional<std::size_t> parentNodeId;
            syrec_operation::operation operation;
            OperandNode                lhsOperand;
            OperandNode                rhsOperand;
            bool                       isOneTimeOperationInversionFlagEnabled;

            [[nodiscard]] std::optional<std::size_t>                         getLeafNodeOperandId() const;
            [[nodiscard]] std::optional<std::pair<std::size_t, std::size_t>> getLeafNodeOperandIds() const;
            [[nodiscard]] bool                                               hasAnyLeafOperandNodes() const;
            [[nodiscard]] bool                                               areBothOperandsLeafNodes() const;
            [[nodiscard]] std::optional<syrec_operation::operation>          getOperationConsideringWhetherMarkedAsInverted() const;
            void                                                             enabledIsOneTimeOperationInversionFlag();
            void                                                             disableIsOneTimeOperationInversionFlag();
        };
        using OperationNodeReference = std::shared_ptr<OperationNode>;

        [[nodiscard]] std::optional<syrec::VariableExpression::ptr> getOperandAsVariableExpr(std::size_t operandNodeId) const;
        [[nodiscard]] std::optional<syrec::Expression::ptr>         getOperandAsExpr(std::size_t operandNodeId) const;

        [[nodiscard]] std::optional<OperationNodeReference>     getNextOperationNode();
        [[nodiscard]] std::optional<OperationNodeReference>     peekNextOperationNode() const;
        [[nodiscard]] std::optional<syrec_operation::operation> getOperationOfOperationNode(std::size_t operationNodeId) const;
        [[nodiscard]] std::optional<std::size_t>                getOperandNodeIdOfNestedOperation(std::size_t parentOperationNodeId, std::size_t nestedOperationNodeId) const;
        [[nodiscard]] bool                                      canSignalBeUsedOnAssignmentLhs(const std::string& signalIdent) const;
        [[nodiscard]] std::optional<OperationNodeReference>     getOperationNodeById(std::size_t operationNodeId) const;
        void                                                    markSignalAsAssignable(const std::string& assignableSignalIdent);
        [[nodiscard]] std::optional<std::size_t>                getOperationNodeIdOfRightOperand(std::size_t operationNodeId) const;
        [[nodiscard]] std::optional<std::size_t>                getOperationNodeIdOfFirstEntryInQueue() const;

        /**
         * \brief Update the operand data of a given operation node
         * \param operationNodeId The operation node containing the operand to be updated
         * \param updateLhsOperandData A flag indicating whether the lhs operand shall be updated
         * \param newOperandData The new data of the operand, which must be a syrec::VariableExpression
         * \return Whether the update was successful
         */
        [[nodiscard]] bool                                      updateOperandData(std::size_t operationNodeId, bool updateLhsOperandData, const syrec::Expression::ptr& newOperandData);
        [[maybe_unused]] bool                                   updateOperation(std::size_t operationNodeId, syrec_operation::operation newOperation) const;
        void                                                    markOperationNodeAsPotentialBacktrackOption(std::size_t operationNodeId);
        void                                                    removeOperationNodeAsPotentialBacktrackOperation(std::size_t operationNodeId);
        void                                                    backtrackToNode(std::size_t operationNodeId);
        void                                                    backtrackOnePastNode(std::size_t operationNodeId);

        /**
         * \brief Build the traversal queue from the given expression. By default a pre-order traversal will be performed.
         * \param expr The expression from which the traversal queue shall be built.
         * \param symbolTableReference A reference to a symbol table to determine whether a given operand of an operation can be used on the left-hand side of an assignment (i.e. as the assigned to signal)
         */
        virtual void buildTraversalQueue(const syrec::Expression::ptr& expr, const parser::SymbolTable& symbolTableReference);
        virtual void resetInternals();

        virtual ~ExpressionTraversalHelper() = default;
    protected:
        struct IdGenerator {
            [[maybe_unused]] std::size_t generateNextId();
            void                      reset();
        private:
            std::size_t lastGeneratedId = 0;
        };

        IdGenerator operationNodeIdGenerator;
        IdGenerator operandNodeIdGenerator;

        std::unordered_map<std::size_t, syrec::Expression::ptr> operandNodeDataLookup;
        std::unordered_map<std::size_t, OperationNodeReference> operationNodeDataLookup;
        std::vector<std::size_t>                                backtrackOperationNodeIds;
        std::vector<std::size_t>                                operationNodeTraversalQueue;
        std::size_t                                             operationNodeTraversalQueueIdx = 0;
        std::unordered_set<std::string>                         identsOfAssignableSignals; 

        virtual void                                        handleOperationNodeDuringTraversalQueueInit(const OperationNodeReference& operationNode);
        void                                                addSignalIdentAsUsableInAssignmentLhsIfAssignable(const std::string& signalIdent, const parser::SymbolTable& symbolTableReference);
        void                                                determineIdentsOfSignalsUsableInAssignmentLhs(const parser::SymbolTable& symbolTableReference);
        void                                                backtrackToNode(std::size_t operationNodeId, bool removeBacktrackEntryForNode);
        [[nodiscard]] std::optional<syrec::Expression::ptr> getDataOfOperand(std::size_t operandNodeId) const;
        [[nodiscard]] OperandNode                           createOperandNode(const syrec::Expression::ptr& expr, const std::optional<std::size_t>& parentOperationNodeId);
        [[nodiscard]] OperationNodeReference                createOperationNode(const syrec::Expression::ptr& expr, const std::optional<std::size_t>& parentOperationNodeId);
        [[nodiscard]] static bool                           doesExpressionContainNestedExprAsOperand(const syrec::Expression& expr);
        [[nodiscard]] static syrec::Expression::ptr         createExpressionForNumber(const syrec::Number::ptr& number, unsigned int expectedBitwidth);
    };
}
#endif