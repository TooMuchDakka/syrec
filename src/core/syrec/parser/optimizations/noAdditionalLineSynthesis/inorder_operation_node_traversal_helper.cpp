#include "core/syrec/parser/optimizations/noAdditionalLineSynthesis/inorder_operation_node_traversal_helper.hpp"

using namespace noAdditionalLineSynthesis;

std::optional<syrec::NumericExpression::ptr> InorderOperationNodeTraversalHelper::OperationNodeLeaf::getAsNumber()  const {
    if (std::holds_alternative<std::shared_ptr<syrec::NumericExpression>>(nodeVariantData)) {
        return std::make_optional(std::get<std::shared_ptr<syrec::NumericExpression>>(nodeVariantData));
    }
    return std::nullopt;
}

std::optional<syrec::VariableExpression::ptr> InorderOperationNodeTraversalHelper::OperationNodeLeaf::getAsSignalAccess() const {
    if (std::holds_alternative<std::shared_ptr<syrec::VariableExpression>>(nodeVariantData)) {
        return std::make_optional(std::get<std::shared_ptr<syrec::VariableExpression>>(nodeVariantData));
    }
    return std::nullopt;
}

std::optional<InorderOperationNodeTraversalHelper::OperationNodeReference> InorderOperationNodeTraversalHelper::getNextOperationNode() {
    while (!operationNodeTraversalQueue.empty()) {
        const auto& nextOperationNode = std::make_optional(operationNodesLookup.at(operationNodeTraversalQueue.front()));
        operationNodeTraversalQueue.pop();
        return nextOperationNode;
    }
    return std::nullopt;
}

void InorderOperationNodeTraversalHelper::buildOperationNodesQueue(const syrec::expression::ptr& expr) {
    buildOperationNode(expr);
}

InorderOperationNodeTraversalHelper::TraversalNode InorderOperationNodeTraversalHelper::buildOperationNode(const syrec::expression::ptr& expr) {
    if (const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr); exprAsBinaryExpr != nullptr) {
        const auto operationNodeId = operationNodeIdCounter++;
        operationNodeTraversalQueue.emplace(operationNodeId);
        const auto& lhsNode = buildOperationNode(exprAsBinaryExpr->lhs);
        const auto& rhsNode = buildOperationNode(exprAsBinaryExpr->rhs);
        const auto& mappedBinaryOperation = *tryMapBinaryExprOperationFlagToEnum(exprAsBinaryExpr);
        operationNodesLookup.insert(std::make_pair(operationNodeId, std::make_shared<OperationNode>(operationNodeId, mappedBinaryOperation, lhsNode, rhsNode)));
        return TraversalNode(operationNodeId, true);
    }
    if (const auto& exprAsShiftExpr = std::dynamic_pointer_cast<syrec::ShiftExpression>(expr); exprAsShiftExpr != nullptr) {
        const auto operationNodeId = operationNodeIdCounter++;
        operationNodeTraversalQueue.emplace(operationNodeId);
        const auto& lhsNode               = buildOperationNode(exprAsShiftExpr->lhs);
        const auto& rhsNode              = buildOperationNode(exprAsShiftExpr->rhs, determineBitwidthOfExpr(exprAsShiftExpr->lhs));
        const auto& mappedShiftOperation  = *tryMapShiftOperationExprFlagToEnum(exprAsShiftExpr);
        operationNodesLookup.insert(std::make_pair(operationNodeId, std::make_shared<OperationNode>(operationNodeId, mappedShiftOperation, lhsNode, rhsNode)));
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

std::optional<InorderOperationNodeTraversalHelper::OperationNodeLeafReference> InorderOperationNodeTraversalHelper::getLeafNodeOfOperationNode(const OperationNodeReference& operationNode, bool fetchLeftOperand) const {
    if ((fetchLeftOperand && operationNode->lhs.isOperationNode) || (!fetchLeftOperand && operationNode->rhs.isOperationNode)) {
        return std::nullopt;
    }
    return std::make_optional(leafNodesLookup.at(fetchLeftOperand ? operationNode->lhs.nodeId : operationNode->rhs.nodeId));
}

std::optional<InorderOperationNodeTraversalHelper::OperationNodeReference> InorderOperationNodeTraversalHelper::getOperationNodeForId(std::size_t operationNodeId) const {
    return operationNodeId < operationNodesLookup.size() ? std::make_optional(operationNodesLookup.at(operationNodeId)) : std::nullopt;
}

std::optional<syrec_operation::operation> InorderOperationNodeTraversalHelper::tryMapShiftOperationExprFlagToEnum(const std::shared_ptr<syrec::ShiftExpression>& shiftExpr) {
    return std::make_optional(shiftExpr->op == syrec::ShiftExpression::Left ? syrec_operation::operation::shift_left : syrec_operation::operation::shift_right);
}

std::optional<syrec_operation::operation> InorderOperationNodeTraversalHelper::tryMapBinaryExprOperationFlagToEnum(const std::shared_ptr<syrec::BinaryExpression>& binaryExpr) {
    switch (binaryExpr->op) {
        case syrec::BinaryExpression::Add:
            return std::make_optional(syrec_operation::operation::addition);
        case syrec::BinaryExpression::Subtract:
            return std::make_optional(syrec_operation::operation::subtraction);
        case syrec::BinaryExpression::Multiply:
            return std::make_optional(syrec_operation::operation::multiplication);
        case syrec::BinaryExpression::Divide:
            return std::make_optional(syrec_operation::operation::division);
        case syrec::BinaryExpression::Modulo:
            return std::make_optional(syrec_operation::operation::modulo);
        case syrec::BinaryExpression::LogicalAnd:
            return std::make_optional(syrec_operation::operation::logical_and);
        case syrec::BinaryExpression::LogicalOr:
            return std::make_optional(syrec_operation::operation::logical_or);
        case syrec::BinaryExpression::BitwiseAnd:
            return std::make_optional(syrec_operation::operation::bitwise_and);
        case syrec::BinaryExpression::BitwiseOr:
            return std::make_optional(syrec_operation::operation::bitwise_or);
        case syrec::BinaryExpression::Exor:
            return std::make_optional(syrec_operation::operation::bitwise_xor);
        case syrec::BinaryExpression::FracDivide:
            return std::make_optional(syrec_operation::operation::upper_bits_multiplication);
        case syrec::BinaryExpression::LessThan:
            return std::make_optional(syrec_operation::operation::less_than);
        case syrec::BinaryExpression::GreaterThan:
            return std::make_optional(syrec_operation::operation::greater_than);
        case syrec::BinaryExpression::LessEquals:
            return std::make_optional(syrec_operation::operation::less_than);
        case syrec::BinaryExpression::GreaterEquals:
            return std::make_optional(syrec_operation::operation::greater_equals);
        case syrec::BinaryExpression::Equals:
            return std::make_optional(syrec_operation::operation::equals);
        case syrec::BinaryExpression::NotEquals:
            return std::make_optional(syrec_operation::operation::not_equals);
        default:
            return std::nullopt;
    }
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