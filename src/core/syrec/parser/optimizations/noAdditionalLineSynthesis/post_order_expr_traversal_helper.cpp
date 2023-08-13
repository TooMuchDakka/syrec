#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/post_order_expr_traversal_helper.hpp"

using namespace noAdditionalLineSynthesis;

std::optional<PostOrderExprTraversalHelper::PostOrderTraversalNode::ptr> PostOrderExprTraversalHelper::getNextElement() {
    // TODO: Remove nodes if no longer needed during dequeue with manual reference counting
    /*if (lastDequeueLeafNode.has_value()) {
        decrementReferencesOnOperationNodeAndRemoveIfNoneRemain(leafNodes.at(*lastDequeueLeafNode)->parentNodeIdx);
    }*/

    if (!leafNodesInPostOrderQueue.empty()) {
        const auto nextLeafNodeIdxInQueue = leafNodesInPostOrderQueue.front();
        leafNodesInPostOrderQueue.pop();
        lastDequeueLeafNode.emplace(nextLeafNodeIdxInQueue);
        return std::make_optional(leafNodes.at(nextLeafNodeIdxInQueue));
    }
    return std::nullopt;
}

bool PostOrderExprTraversalHelper::doesOperationNodeOnlyHaveOneLeaf(const PostOrderTraversalNode::ptr& node) const {
    if (const auto& matchingOperationNode = getOperationNodeForIdx(node->parentNodeIdx); matchingOperationNode != nullptr) {
        return matchingOperationNode->leftOperand.isOperationNode ^ matchingOperationNode->rightOperand.isOperationNode && !(matchingOperationNode->leftOperand.isOperationNode && matchingOperationNode->rightOperand.isOperationNode);
    }
    return false;
}

bool PostOrderExprTraversalHelper::areBothOperandsOfOperationNodeLeaves(const PostOrderTraversalNode::ptr& node) const {
    if (const auto& matchingOperationNode = getOperationNodeForIdx(node->parentNodeIdx); matchingOperationNode != nullptr) {
        return !matchingOperationNode->leftOperand.isOperationNode && !matchingOperationNode->rightOperand.isOperationNode;
    }
    return false;
}

syrec_operation::operation PostOrderExprTraversalHelper::getOperationOfParent(const PostOrderTraversalNode::ptr& node) const {
    return getOperationNodeForIdx(node->parentNodeIdx)->operation;
}

std::optional<syrec_operation::operation> PostOrderExprTraversalHelper::getOperationOfGrandParent(const PostOrderTraversalNode::ptr& node) const {
    if (const auto& grandParentNodeIdx = getOperationNodeForIdx(node->parentNodeIdx)->parentNodeIdx; grandParentNodeIdx.has_value()) {
        return std::make_optional(getOperationNodeForIdx(*grandParentNodeIdx)->operation);
    }
    return std::nullopt;
}

std::optional<syrec_operation::operation> PostOrderExprTraversalHelper::tryGetRequiredAssignmentOperation(const PostOrderTraversalNode::ptr& node) const {
    if (initialSubExpressionAssignmentOperation.count(node->nodeIdx) != 0) {
        return std::make_optional(initialSubExpressionAssignmentOperation.at(node->nodeIdx));
    }
    return std::nullopt;
}

std::optional<syrec::VariableAccess::ptr> PostOrderExprTraversalHelper::PostOrderTraversalNode::tryGetNodeDataAsSignalAccess() const {
    if (std::holds_alternative<syrec::VariableAccess::ptr>(nodeData)) {
        return std::make_optional(std::get<syrec::VariableAccess::ptr>(nodeData));
    }
    return std::nullopt;
}

std::optional<syrec::NumericExpression::ptr> PostOrderExprTraversalHelper::PostOrderTraversalNode::tryGetNodeDataAsNonSignalAccess() const {
    if (std::holds_alternative<syrec::NumericExpression::ptr>(nodeData)) {
        return std::make_optional(std::get<syrec::NumericExpression::ptr>(nodeData));
    }
    return std::nullopt;
}

syrec::expression::ptr PostOrderExprTraversalHelper::PostOrderTraversalNode::getNodeData() const {
    if (const auto& nodeDataAsSignalAccess = tryGetNodeDataAsSignalAccess(); nodeDataAsSignalAccess.has_value()) {
        return std::make_shared<syrec::VariableExpression>(*nodeDataAsSignalAccess);
    } 
    return *tryGetNodeDataAsNonSignalAccess();
}

const PostOrderExprTraversalHelper::OperationNode* PostOrderExprTraversalHelper::getOperationNodeForIdx(std::size_t operationNodeIdx) const {
    if (operationNodes.size() > operationNodeIdx) {
        return &operationNodes.at(operationNodeIdx);
    }
    return nullptr;
}

bool PostOrderExprTraversalHelper::doesExprDefineNestedExpr(const syrec::expression::ptr& expr) {
    return std::dynamic_pointer_cast<syrec::BinaryExpression>(expr) != nullptr;
}

bool PostOrderExprTraversalHelper::doesExprDefineSignalAccess(const syrec::expression::ptr& expr) {
    return std::dynamic_pointer_cast<syrec::VariableExpression>(expr) != nullptr;
}

syrec_operation::operation PostOrderExprTraversalHelper::mapBinaryExprOperationToEnum(const syrec::BinaryExpression::ptr& binaryExpr) {
    const auto& binaryExprCasted = std::dynamic_pointer_cast<syrec::BinaryExpression>(binaryExpr);
    switch (binaryExprCasted->op) {
        case syrec::BinaryExpression::Add:
            return syrec_operation::operation::Addition;
        case syrec::BinaryExpression::Subtract:
            return syrec_operation::operation::Subtraction;
        default:
            return syrec_operation::operation::BitwiseXor;
    }
}

PostOrderExprTraversalHelper::BaseNodeIdx PostOrderExprTraversalHelper::buildNodeForExpr(const syrec::expression::ptr& expr, const std::size_t parentNodeIdx, bool isNodeLeftChildOfParent) {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); doesExprDefineNestedExpr(expr)) {
        const auto& operationNodeIdx = fetchAndIncrementOperationNodeIdx();

        auto& blankOperationNode         = operationNodes[operationNodeIdx];
        blankOperationNode.operation     = mapBinaryExprOperationToEnum(exprAsBinaryExpr);
        blankOperationNode.parentNodeIdx = parentNodeIdx;

        if (doesExprDefineSignalAccess(exprAsBinaryExpr->lhs) && !doesExprDefineSignalAccess(exprAsBinaryExpr->rhs)) {
            if (nextSubexpressionInitialAssignmentOperation.has_value()) {
                initialSubExpressionAssignmentOperation.insert(std::make_pair(leafNodes.size(), *nextSubexpressionInitialAssignmentOperation));
                nextSubexpressionInitialAssignmentOperation = syrec_operation::getMatchingAssignmentOperationForOperation(blankOperationNode.operation);       
            } else {
                nextSubexpressionInitialAssignmentOperation = syrec_operation::getMatchingAssignmentOperationForOperation(blankOperationNode.operation);       
            }
        } else if (doesExprDefineSignalAccess(exprAsBinaryExpr->lhs) && doesExprDefineSignalAccess(exprAsBinaryExpr->rhs)) {
            if (isNodeLeftChildOfParent) {
                initialSubExpressionAssignmentOperation.insert(std::make_pair(leafNodes.size(), *nextSubexpressionInitialAssignmentOperation));
                nextSubexpressionInitialAssignmentOperation = syrec_operation::getMatchingAssignmentOperationForOperation(operationNodes.at(parentNodeIdx).operation);       
            }
            else {
                nextSubexpressionInitialAssignmentOperation.reset();    
            }
        }

        //blankOperationNode.leftOperand   = buildNodeForExpr(exprAsBinaryExpr->lhs, operationNodeIdx);
        //blankOperationNode.rightOperand  = buildNodeForExpr(exprAsBinaryExpr->rhs, operationNodeIdx);
        operationNodes[operationNodeIdx].leftOperand  = buildNodeForExpr(exprAsBinaryExpr->lhs, operationNodeIdx, true);
        operationNodes[operationNodeIdx].rightOperand = buildNodeForExpr(exprAsBinaryExpr->rhs, operationNodeIdx, false);

        return BaseNodeIdx(operationNodeIdx, true);
    }
    return buildNodeForLeafNode(expr, parentNodeIdx);
}

PostOrderExprTraversalHelper::BaseNodeIdx PostOrderExprTraversalHelper::buildNodeForLeafNode(const syrec::expression::ptr& expr, std::size_t parentNodeIdx) {
    PostOrderTraversalNode::ptr node;
    if (const auto& exprAsSignalAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsSignalAccess != nullptr) {
        node = std::make_shared<PostOrderTraversalNode>(PostOrderTraversalNode(leafNodes.size(), parentNodeIdx, exprAsSignalAccess->var));
    } else {
        node = std::make_shared<PostOrderTraversalNode>(PostOrderTraversalNode(leafNodes.size(), parentNodeIdx, expr));
    }
    const auto  nodeIdx            = leafNodes.size();
    leafNodes.emplace_back(node);
    leafNodesInPostOrderQueue.emplace(nodeIdx);
    return BaseNodeIdx(nodeIdx, false);
}

void PostOrderExprTraversalHelper::buildPostOrderTraversalQueue(const syrec::BinaryExpression::ptr& binaryExpr, const std::optional<std::size_t>& parentNodeIdx) {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(binaryExpr); doesExprDefineNestedExpr(binaryExpr)) {
        const auto& operationNodeIdx = fetchAndIncrementOperationNodeIdx();

        auto& blankOperationNode         = operationNodes[operationNodeIdx];
        blankOperationNode.operation = mapBinaryExprOperationToEnum(exprAsBinaryExpr);
        blankOperationNode.parentNodeIdx = parentNodeIdx;
        /*blankOperationNode.leftOperand   = buildNodeForExpr(exprAsBinaryExpr->lhs, operationNodeIdx);
        blankOperationNode.rightOperand  = buildNodeForExpr(exprAsBinaryExpr->rhs, operationNodeIdx);*/
        operationNodes[operationNodeIdx].leftOperand = buildNodeForExpr(exprAsBinaryExpr->lhs, operationNodeIdx, true);
        nextSubexpressionInitialAssignmentOperation   = *syrec_operation::getMatchingAssignmentOperationForOperation(mapBinaryExprOperationToEnum(exprAsBinaryExpr));
        operationNodes[operationNodeIdx].rightOperand = buildNodeForExpr(exprAsBinaryExpr->rhs, operationNodeIdx, false);
    }
}

void PostOrderExprTraversalHelper::decrementReferencesOnOperationNodeAndRemoveIfNoneRemain(std::size_t parentNodeIdx) {
    if (operationNodes.size() < parentNodeIdx) {
        return;
    }

    auto& referencedParentNode = operationNodes.at(parentNodeIdx);
    referencedParentNode.activeReferencesOnNode--;
    if (referencedParentNode.activeReferencesOnNode == 0) {
        operationNodes.erase(std::next(operationNodes.begin(), parentNodeIdx + 1));
    }
}

std::size_t PostOrderExprTraversalHelper::fetchAndIncrementOperationNodeIdx() {
    createBlankOperationNode();
    return operationNodeIdxCounter++;
}

void PostOrderExprTraversalHelper::createBlankOperationNode() {
    operationNodes.emplace_back(OperationNode(syrec_operation::operation::Addition, std::nullopt, BaseNodeIdx(0, false), BaseNodeIdx(0, false)));
}