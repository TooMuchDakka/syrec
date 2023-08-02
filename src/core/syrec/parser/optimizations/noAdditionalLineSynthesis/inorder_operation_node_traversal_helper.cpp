#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/inorder_operation_node_traversal_helper.hpp"

using namespace noAdditionalLineSynthesis;

std::optional<std::shared_ptr<syrec::NumericExpression>> InorderOperationNodeTraversalHelper::OperationNodeLeaf::getAsNumber()  const {
    if (std::holds_alternative<std::shared_ptr<syrec::NumericExpression>>(nodeVariantData)) {
        return std::make_optional(std::get<std::shared_ptr<syrec::NumericExpression>>(nodeVariantData));
    }
    return std::nullopt;
}

std::optional<std::shared_ptr<syrec::VariableExpression>> InorderOperationNodeTraversalHelper::OperationNodeLeaf::getAsSignalAccess() const {
    if (std::holds_alternative<std::shared_ptr<syrec::VariableExpression>>(nodeVariantData)) {
        return std::make_optional(std::get<std::shared_ptr<syrec::VariableExpression>>(nodeVariantData));
    }
    return std::nullopt;
}

syrec::expression::ptr InorderOperationNodeTraversalHelper::OperationNodeLeaf::getData() const {
    if (const auto& dataAsSignalAccess = getAsSignalAccess(); dataAsSignalAccess.has_value()) {
        return *dataAsSignalAccess;
    }
    return *getAsNumber();
}


std::optional<InorderOperationNodeTraversalHelper::OperationNodeReference> InorderOperationNodeTraversalHelper::getNextOperationNode() {
    while (!operationNodeTraversalQueue.empty()) {
        const auto& nextOperationNode = std::make_optional(operationNodesLookup.at(operationNodeTraversalQueueIdx++));
        return nextOperationNode;
    }
    return std::nullopt;
}

std::optional<InorderOperationNodeTraversalHelper::OperationNodeReference> InorderOperationNodeTraversalHelper::peekNextOperationNode() const {
    if (operationNodeTraversalQueue.empty()) {
        return std::nullopt;
    }
    return std::make_optional(operationNodesLookup.at(operationNodeTraversalQueue.at(operationNodeTraversalQueueIdx)));
}

std::optional<std::size_t> InorderOperationNodeTraversalHelper::getNodeIdOfLastOperationIdOfCurrentSubexpression() const {
    if (operationNodeTraversalQueue.empty()) {
        return std::nullopt;
    }

    const auto& nextOperationNode = operationNodesLookup.at(operationNodeTraversalQueueIdx);
    /*
     * The only operation node without a parent is the top most one and thus the last operation node of the subtree of the current operation node
     * is, in case the right subtree has no operation nodes defined, the last entry in the operation node traversal queue. Otherwise, if
     * the left subtree defines an subexpression, the node with id = node_id_first_of_rhs - 1 is the last one. If none of these conditions match,
     * no value is returned
     *
     */
    if (!nextOperationNode->parentNodeId.has_value()) {
        const auto& firstOperationNodeOfRightSubtree = operationNodesLookup.at(0)->rhs.isOperationNode ? std::make_optional(operationNodesLookup.at(0)->rhs) : std::nullopt;

        if (firstOperationNodeOfRightSubtree.has_value()) {
            return std::make_optional(firstOperationNodeOfRightSubtree->nodeId - 1);
        }
        return std::make_optional(operationNodeTraversalQueue.back());
        std::optional<std::size_t> lastOperationNodeOfLeftSubtree = firstOperationNodeOfRightSubtree.has_value()
            ? std::make_optional(firstOperationNodeOfRightSubtree->nodeId - 1)
            : std::nullopt;

        if (lastOperationNodeOfLeftSubtree.has_value()) {
            const auto& foundLastOperationNodeOfLeftSubtree = std::find_if(
            operationNodeTraversalQueue.cbegin(),
            operationNodeTraversalQueue.cend(),
            [&firstOperationNodeOfRightSubtree, &nextOperationNode](const std::size_t nodeId) {
                if (firstOperationNodeOfRightSubtree.has_value()) {
                    return nodeId > nextOperationNode->nodeId && nodeId < firstOperationNodeOfRightSubtree->nodeId;
                }
                return nodeId > nextOperationNode->nodeId;
            });
            if (foundLastOperationNodeOfLeftSubtree != operationNodeTraversalQueue.cend()) {
                return *foundLastOperationNodeOfLeftSubtree;
            }
        }
        return lastOperationNodeOfLeftSubtree;
    }

    const auto& parentOperationNode = operationNodesLookup.at(*nextOperationNode->parentNodeId);
    if (nextOperationNode->nodeId == parentOperationNode->lhs.nodeId && parentOperationNode->rhs.isOperationNode) {
        return parentOperationNode->rhs.nodeId - 1;
    }

    /*
     * Search for the next operation node, considering the inorder traversal order of the operation nodes, after the current subexpression tree was traversed
     * (i.e. search for the next operation node in one of the parent nodes). If none was found, the last operation node in the traversal queue is also the last
     * of the subexpression.
     */
    const auto& nextOperationNodeInOneOfParents = findNextOperationNodeInOneOfParents(nextOperationNode);
    if (!nextOperationNodeInOneOfParents.has_value()) {
        return std::make_optional(operationNodeTraversalQueue.back());
    }

    // Otherwise, the first operation node with a node id smaller then the one found in the parent is the last one of the subexpression.
    const auto& foundLastOperationNodeOfSubtree = std::find_if(
    operationNodeTraversalQueue.rbegin(),
    operationNodeTraversalQueue.rend(),
[&nextOperationNodeInOneOfParents](const std::size_t nodeId) {
            return nodeId == nextOperationNodeInOneOfParents.value() - 1;
      }
    );

    // If both checks above do not return a result, the current operation node is the last one of the current subexpression
    return std::make_optional(foundLastOperationNodeOfSubtree != operationNodeTraversalQueue.rend() ? *foundLastOperationNodeOfSubtree : nextOperationNode->nodeId);
}


void InorderOperationNodeTraversalHelper::buildOperationNodesQueue(const syrec::expression::ptr& expr) {
    buildOperationNode(expr, std::nullopt);
}

InorderOperationNodeTraversalHelper::TraversalNode InorderOperationNodeTraversalHelper::buildOperationNode(const syrec::expression::ptr& expr, const std::optional<std::size_t>& parentNodeId) {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        const auto operationNodeId = operationNodeIdCounter++;
        operationNodeTraversalQueue.emplace_back(operationNodeId);
        const auto& lhsNode = buildOperationNode(exprAsBinaryExpr->lhs, std::make_optional(operationNodeId));
        const auto& rhsNode               = buildOperationNode(exprAsBinaryExpr->rhs, std::make_optional(operationNodeId));
        const auto& mappedBinaryOperation = *syrec_operation::tryMapBinaryOperationFlagToEnum(exprAsBinaryExpr->op);
        operationNodesLookup.insert(std::make_pair(operationNodeId, std::make_shared<OperationNode>(operationNodeId, parentNodeId, mappedBinaryOperation, lhsNode, rhsNode)));
        return TraversalNode(operationNodeId, true);
    }
    if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr); exprAsShiftExpr != nullptr) {
        const auto operationNodeId = operationNodeIdCounter++;
        operationNodeTraversalQueue.emplace_back(operationNodeId);
        const auto& lhsNode               = buildOperationNode(exprAsShiftExpr->lhs, std::make_optional(operationNodeId));
        const auto& rhsNode              = buildOperationNode(exprAsShiftExpr->rhs, determineBitwidthOfExpr(exprAsShiftExpr->lhs));
        const auto& mappedShiftOperation  = *syrec_operation::tryMapShiftOperationFlagToEnum(exprAsShiftExpr->op);
        operationNodesLookup.insert(std::make_pair(operationNodeId, std::make_shared<OperationNode>(operationNodeId, parentNodeId, mappedShiftOperation, lhsNode, rhsNode)));
        return TraversalNode(operationNodeId, true);
    }

    const auto  leafNodeId           = leafNodeIdCounter++;
    const auto& exprAsConstantNumber = std::dynamic_pointer_cast<syrec::NumericExpression>(expr);

    std::variant<std::shared_ptr<syrec::NumericExpression>, std::shared_ptr<syrec::VariableExpression>> leafNodeData;
    if (const auto& exprAsSignalAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsSignalAccess != nullptr) {
        leafNodeData.emplace<std::shared_ptr<syrec::VariableExpression>>(exprAsSignalAccess);
    } else {
        leafNodeData.emplace<std::shared_ptr<syrec::NumericExpression>>(exprAsConstantNumber);
    }
    leafNodesLookup.insert(std::make_pair(leafNodeId, std::make_shared<OperationNodeLeaf>(leafNodeId, leafNodeData)));
    return TraversalNode(leafNodeId, false);
}

std::optional<InorderOperationNodeTraversalHelper::OperationNodeLeafReference> InorderOperationNodeTraversalHelper::getLeafNodeOfOperationNode(const OperationNodeReference& operationNode) const {
    if (operationNode->lhs.isOperationNode && operationNode->rhs.isOperationNode) {
        return std::nullopt;
    }
    return std::make_optional(leafNodesLookup.at(!operationNode->lhs.isOperationNode ? operationNode->lhs.nodeId : operationNode->rhs.nodeId));
}

std::optional<InorderOperationNodeTraversalHelper::OperationNodeReference> InorderOperationNodeTraversalHelper::getOperationNodeForId(std::size_t operationNodeId) const {
    return operationNodeId < operationNodesLookup.size() ? std::make_optional(operationNodesLookup.at(operationNodeId)) : std::nullopt;
}

InorderOperationNodeTraversalHelper::TraversalNode InorderOperationNodeTraversalHelper::buildOperationNode(const syrec::Number::ptr& number, unsigned int expectedBitwidthOfOperation) {
    const auto  leafNodeId           = leafNodeIdCounter++;
    const auto& numericExpr = std::make_shared<syrec::NumericExpression>(number, expectedBitwidthOfOperation);
    std::variant<std::shared_ptr<syrec::NumericExpression>, std::shared_ptr<syrec::VariableExpression>> leafNodeData;
    leafNodeData.emplace<std::shared_ptr<syrec::NumericExpression>>(numericExpr);
    leafNodesLookup.insert(std::make_pair(leafNodeId, std::make_shared<OperationNodeLeaf>(leafNodeId, leafNodeData)));
    return TraversalNode(leafNodeId, false);
}

std::optional<bool> InorderOperationNodeTraversalHelper::areAllOperandsOfOperationNodeLeafNodes(const OperationNodeReference& operationNode) const {
    if (operationNodesLookup.size() >= operationNode->nodeId) {
        return std::nullopt;
    }

    return operationNode->lhs.isOperationNode && operationNode->rhs.isOperationNode;
}

std::optional<std::pair<InorderOperationNodeTraversalHelper::OperationNodeLeafReference, InorderOperationNodeTraversalHelper::OperationNodeLeafReference>> InorderOperationNodeTraversalHelper::getLeafNodesOfOperationNode(const OperationNodeReference& operationNode) const {
    if (operationNode->lhs.isOperationNode || operationNode->rhs.isOperationNode) {
        return std::nullopt;
    }
    return std::make_optional(std::make_pair(leafNodesLookup.at(operationNode->lhs.nodeId), leafNodesLookup.at(operationNode->rhs.nodeId)));
}

bool InorderOperationNodeTraversalHelper::doesExprDefineSignalAccess(const syrec::expression::ptr& expr) {
    return std::dynamic_pointer_cast<syrec::VariableExpression>(expr) != nullptr;
}

unsigned InorderOperationNodeTraversalHelper::determineBitwidthOfExpr(const syrec::expression::ptr& expr) const {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        return determineBitwidthOfExpr(exprAsBinaryExpr->lhs);
    }
    if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr); exprAsShiftExpr != nullptr) {
        return determineBitwidthOfExpr(exprAsShiftExpr->lhs);
    }
    if (const auto& exprAsSignalAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsSignalAccess != nullptr) {
        return determineBitwidthOfSignalAccess(exprAsSignalAccess->var);
    }
    if (const auto& exprAsNumericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(expr); exprAsNumericExpr != nullptr) {
        return exprAsNumericExpr->bwidth;   
    }
    return 0;
}

unsigned InorderOperationNodeTraversalHelper::determineBitwidthOfSignalAccess(const syrec::VariableAccess::ptr& signalAccess) const {
    const auto& symbolTableEntryForSignal = std::get<syrec::Variable::ptr>(*symbolTableReference->getVariable(signalAccess->var->name));
    if (signalAccess->range.has_value() && signalAccess->range->first->isConstant() && signalAccess->range->second->isConstant()) {
        return signalAccess->range->second->evaluate({}) - signalAccess->range->first->evaluate({});
    }
    return symbolTableEntryForSignal->bitwidth;
}

std::optional<std::size_t> InorderOperationNodeTraversalHelper::findNextOperationNodeInOneOfParents(const OperationNodeReference& operationNode) const {
    const auto& parentOperationNode = operationNode->parentNodeId.has_value() ? std::make_optional(operationNodesLookup.at(*operationNode->parentNodeId)) : std::nullopt;
    if (!parentOperationNode.has_value()) {
        return std::nullopt;
    }

    const auto isCurrentOperationNodeLeftChildOfParent = operationNode->nodeId == parentOperationNode.value()->lhs.nodeId && parentOperationNode.value()->lhs.isOperationNode;
    if (isCurrentOperationNodeLeftChildOfParent && parentOperationNode.value()->rhs.isOperationNode) {
        return std::make_optional(parentOperationNode.value()->rhs.nodeId);
    }
    return findNextOperationNodeInOneOfParents(parentOperationNode.value());
}
