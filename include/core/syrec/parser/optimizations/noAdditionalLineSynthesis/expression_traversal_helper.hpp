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

            [[nodiscard]] std::optional<std::size_t>                         getLeafNodeOperandId() const;
            [[nodiscard]] std::optional<std::pair<std::size_t, std::size_t>> getLeafNodeOperandIds() const;
            [[nodiscard]] bool                                               hasAnyLeafOperandNodes() const;
            [[nodiscard]] bool                                               areBothOperandsLeafNodes() const;
        };
        using OperationNodeReference = std::shared_ptr<OperationNode>;

        [[nodiscard]] std::optional<syrec::VariableExpression::ptr> getOperandAsVariableExpr(std::size_t operandNodeId) const;
        [[nodiscard]] std::optional<syrec::expression::ptr>         getOperandAsExpr(std::size_t operandNodeId) const;

        [[nodiscard]] std::optional<OperationNodeReference>     getNextOperationNode();
        [[nodiscard]] std::optional<OperationNodeReference>     peekNextOperationNode() const;
        [[nodiscard]] std::optional<syrec_operation::operation> getOperationOfOperationNode(std::size_t operationNodeId) const;
        [[nodiscard]] std::optional<std::size_t>                getOperandNodeIdOfNestedOperation(std::size_t parentOperationNodeId, std::size_t nestedOperationNodeId) const;
        [[nodiscard]] bool                                      canSignalBeUsedOnAssignmentLhs(const std::string& signalIdent) const;
        [[nodiscard]] std::optional<OperationNodeReference>     getOperationNodeById(std::size_t operationNodeId) const;
        void                                                    markOperationNodeAsPotentialBacktrackOption(std::size_t operationNodeId);
        void                                                    removeOperationNodeAsPotentialBacktrackOperation(std::size_t operationNodeId);
        void                                                    backtrack();
        /**
         * \brief Build the traversal queue from the given expression. By default a pre-order traversal will be performed.
         * \param expr The expression from which the traversal queue shall be built.
         * \param symbolTableReference A reference to a symbol table to determine whether a given operand of an operation can be used on the left-hand side of an assignment (i.e. as the assigned to signal)
         */
        virtual void buildTraversalQueue(const syrec::expression::ptr& expr, const parser::SymbolTable& symbolTableReference);
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

        std::unordered_map<std::size_t, syrec::expression::ptr> operandNodeDataLookup;
        std::unordered_map<std::size_t, OperationNodeReference> operationNodeDataLookup;
        std::vector<std::size_t>                                backtrackOperationNodeIds;
        std::vector<std::size_t>                                operationNodeTraversalQueue;
        std::size_t                                             operationNodeTraversalQueueIdx = 0;
        std::unordered_set<std::string>                         identsOfAssignableSignals; 

        virtual void                                        handleOperationNodeDuringTraversalQueueInit(const OperationNodeReference& operationNode);
        void                                                addSignalIdentAsUsableInAssignmentLhsIfAssignable(const std::string& signalIdent, const parser::SymbolTable& symbolTableReference);
        void                                                determineIdentsOfSignalsUsableInAssignmentLhs(const parser::SymbolTable& symbolTableReference);
        [[nodiscard]] std::optional<syrec::expression::ptr> getDataOfOperand(std::size_t operandNodeId) const;
        [[nodiscard]] OperandNode                           createOperandNode(const syrec::expression::ptr& expr, const std::optional<std::size_t>& parentOperationNodeId);
        [[nodiscard]] OperationNodeReference                createOperationNode(const syrec::expression::ptr& expr, const std::optional<std::size_t>& parentOperationNodeId);
        [[nodiscard]] static bool                           doesExpressionContainNestedExprAsOperand(const syrec::expression& expr);
        [[nodiscard]] static syrec::expression::ptr         createExpressionForNumber(const syrec::Number::ptr& number, unsigned int expectedBitwidth);
    };
}


#endif

//#ifndef EXPRESSION_TRAVERSAL_HELPER_HPP
//#define EXPRESSION_TRAVERSAL_HELPER_HPP
//#pragma once
//
//#include "core/syrec/expression.hpp"
//#include "core/syrec/parser/operation.hpp"
//
//#include <unordered_map>
//
//namespace noAdditionalLineSynthesis {
//	class ExpressionTraversalHelper {
//	public:
//        using ptr = std::shared_ptr<ExpressionTraversalHelper>;
//
//        struct OperandNode {
//            std::size_t nodeId;
//            bool        isLeafNode;
//        };
//
//        struct OperationNode {
//            std::size_t id;
//            std::optional<std::size_t> parentId;
//            syrec_operation::operation operation;
//            OperandNode                lhs;
//            OperandNode                rhs;
//        };
//        
//        using OperationNodeReference = std::shared_ptr<OperationNode>;
//        using ExpressionTraversalHelperReference = std::shared_ptr<ExpressionTraversalHelper>;
//
//        void                                                                               switchOperandsOfOperation(std::size_t operationNodeId) const;
//        [[nodiscard]] std::optional<OperationNodeReference>                                tryGetNextOperationNode();
//        [[nodiscard]] std::optional<OperationNodeReference>                                peekNextOperationNode() const;
//        [[nodiscard]] std::optional<std::pair<OperandNode, OperandNode>>                   tryGetNonNestedOperationOperands(std::size_t operationNodeId) const;
//        [[nodiscard]] std::optional<OperandNode>                                           tryGetNonNestedOperationOperand(std::size_t operationNodeId) const;
//        [[nodiscard]] std::optional<syrec::expression::ptr>                                getDataOfOperandNode(std::size_t operandNodeId) const;
//        [[maybe_unused]] bool                                                              createCheckPointAtOperationNode(std::size_t operationNodeId);
//        [[maybe_unused]] bool                                                              tryBacktrackToLastCheckpoint();
//
//        virtual ~ExpressionTraversalHelper() = 0;
//	protected:
//        struct IdGenerator {
//            [[nodiscard]] std::size_t generateNextId();
//            void                      reset();
//        private:
//            std::size_t lastGeneratedId = 0;
//        };
//
//        IdGenerator operationNodeIdGenerator;
//        IdGenerator operandNodeIdGenerator;
//
//        std::unordered_map<std::size_t, syrec::expression::ptr> operandNodeDataLookup;
//        std::unordered_map<std::size_t, OperationNodeReference> operationNodeDataLookup;
//        std::vector<std::size_t>                                operationNodeTraversalQueue;
//        std::vector<std::size_t>                                checkpoints;
//        std::size_t                                             operationNodeTraversalQueueIndex = 0;
//
//        virtual void                                        buildTraversalQueue() = 0;
//        virtual void                                        resetInternals();
//        [[nodiscard]] OperandNode                           createNodeFor(const syrec::expression::ptr& expr, const std::optional<std::size_t>& parentOperationNodeId);
//        [[nodiscard]] OperationNodeReference                createOperationNodeFor(const syrec::expression::ptr& expr, const std::optional<std::size_t>& parentOperationNodeId);
//        [[nodiscard]] std::optional<OperationNodeReference> getOperationNodeById(std::size_t operationNodeId) const;
//        [[nodiscard]] static bool                           doesExpressionDefineExprOperand(const syrec::expression& expr);
//        [[nodiscard]] static syrec::expression::ptr         createExprForNumberOperand(const syrec::Number::ptr& number, unsigned int expectedBitwidth);
//	};
//}
//#endif