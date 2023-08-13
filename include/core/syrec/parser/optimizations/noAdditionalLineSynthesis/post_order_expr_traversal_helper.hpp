#ifndef POST_ORDER_EXPR_TRAVERSAL_HELPER_HPP
#define POST_ORDER_EXPR_TRAVERSAL_HELPER_HPP
#pragma once

#include "core/syrec/expression.hpp"
#include "core/syrec/parser/operation.hpp"

#include <queue>

namespace noAdditionalLineSynthesis {
    class PostOrderExprTraversalHelper {
    public:
        explicit PostOrderExprTraversalHelper(syrec_operation::operation assignmentOperation, const syrec::expression::ptr& expr):
            operationNodeIdxCounter(0), nextSubexpressionInitialAssignmentOperation(std::make_optional(assignmentOperation)) {
            buildPostOrderTraversalQueue(expr, std::nullopt);
        }

        class PostOrderTraversalNode {
        public:
            // Could also use a const & instead of a smart pointer
            using ptr = std::shared_ptr<PostOrderTraversalNode>;
            std::size_t                nodeIdx;
            std::size_t                parentNodeIdx;

            std::variant<syrec::VariableAccess::ptr, syrec::NumericExpression::ptr> nodeData;

            explicit PostOrderTraversalNode(std::size_t nodeIdx, std::size_t parentNodeIdx, syrec::NumericExpression::ptr nonSignalAccessData):
                nodeIdx(nodeIdx), parentNodeIdx(parentNodeIdx), nodeData(std::move(nonSignalAccessData)){ }

            explicit PostOrderTraversalNode(std::size_t nodeIdx, std::size_t parentNodeIdx, syrec::VariableAccess::ptr signalAccess):
                nodeIdx(nodeIdx), parentNodeIdx(parentNodeIdx), nodeData(std::move(signalAccess)) { }

            [[nodiscard]] std::optional<syrec::VariableAccess::ptr>    tryGetNodeDataAsSignalAccess() const;
            [[nodiscard]] std::optional<syrec::NumericExpression::ptr> tryGetNodeDataAsNonSignalAccess() const;
            [[nodiscard]] syrec::expression::ptr                       getNodeData() const;
        };

        [[nodiscard]] std::optional<PostOrderTraversalNode::ptr>          getNextElement();
        [[nodiscard]] bool                                                doesOperationNodeOnlyHaveOneLeaf(const PostOrderTraversalNode::ptr& node) const;
        [[nodiscard]] bool                                                areBothOperandsOfOperationNodeLeaves(const PostOrderTraversalNode::ptr& node) const;
        [[nodiscard]] syrec_operation::operation                          getOperationOfParent(const PostOrderTraversalNode::ptr& node) const;
        [[nodiscard]] std::optional<syrec_operation::operation>           getOperationOfGrandParent(const PostOrderTraversalNode::ptr& node) const;
        [[nodiscard]] std::optional<syrec_operation::operation>           tryGetRequiredAssignmentOperation(const PostOrderTraversalNode::ptr& node) const;
    private:
        struct BaseNodeIdx {
            std::size_t idx;
            bool        isOperationNode;

            explicit BaseNodeIdx(std::size_t idx, bool isOperationNode):
                idx(idx), isOperationNode(isOperationNode) {}

            BaseNodeIdx& operator=(BaseNodeIdx other) {
                std::swap(idx, other.idx);
                std::swap(isOperationNode, other.isOperationNode);
                return *this;
            }
        };

        struct OperationNode {
            syrec_operation::operation operation;
            std::optional<std::size_t> parentNodeIdx;
            BaseNodeIdx                leftOperand;
            BaseNodeIdx                rightOperand;
            // TODO: Remove nodes if no longer needed during dequeue with manual reference counting
            std::size_t                activeReferencesOnNode;

            explicit OperationNode(syrec_operation::operation operation, const std::optional<std::size_t>& parentNodeIdx, BaseNodeIdx&& leftOperand, BaseNodeIdx&& rightOperand):
                operation(operation), parentNodeIdx(parentNodeIdx), leftOperand(leftOperand), rightOperand(rightOperand), activeReferencesOnNode(2) {}
        };

        std::vector<OperationNode>                        operationNodes;
        std::vector<PostOrderTraversalNode::ptr>          leafNodes;
        std::queue<std::size_t>                           leafNodesInPostOrderQueue;
        std::optional<std::size_t>                        lastDequeueLeafNode;
        std::size_t                                       operationNodeIdxCounter;
        std::map<std::size_t, syrec_operation::operation> initialSubExpressionAssignmentOperation;
        std::optional<syrec_operation::operation>         nextSubexpressionInitialAssignmentOperation;


        [[nodiscard]] const OperationNode*                      getOperationNodeForIdx(std::size_t operationNodeIdx) const;
        [[nodiscard]] static bool                               doesExprDefineNestedExpr(const syrec::expression::ptr& expr);
        [[nodiscard]] static bool                               doesExprDefineSignalAccess(const syrec::expression::ptr& expr);
        [[nodiscard]] static syrec_operation::operation         mapBinaryExprOperationToEnum(const syrec::BinaryExpression::ptr& binaryExpr);
        [[nodiscard]] BaseNodeIdx                               buildNodeForExpr(const syrec::expression::ptr& expr, std::size_t parentNodeIdx, bool isNodeLeftChildOfParent);
        [[nodiscard]] BaseNodeIdx                               buildNodeForLeafNode(const syrec::expression::ptr& expr, std::size_t parentNodeIdx);
        void                                                    buildPostOrderTraversalQueue(const syrec::BinaryExpression::ptr& binaryExpr, const std::optional<std::size_t>& parentNodeIdx);
        void                                                    decrementReferencesOnOperationNodeAndRemoveIfNoneRemain(std::size_t parentNodeIdx);
        [[nodiscard]] std::size_t                               fetchAndIncrementOperationNodeIdx();
        void                                                    createBlankOperationNode();
    };
}
#endif