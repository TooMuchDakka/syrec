#include "core/syrec/parser/optimizations/no_additional_line_assignment.hpp"
#include "core/syrec/parser/operation.hpp"
// TODO: Refactor mapping of operation into own header file to not drag requirement for antlr dependencies into this class
#include "core/syrec/parser/parser_utilities.hpp"

#include <algorithm>
#include <set>
#include <stack>

using namespace optimizations;

std::vector<std::size_t> LineAwareOptimization::PostOrderTreeTraversal::getTraversalIndexOfAllLeafNodes() const {
    return getEitherInternalOrLeafNodes(true);
}

std::vector<std::size_t> LineAwareOptimization::PostOrderTreeTraversal::getTraversalIndexOfAllOperationNodes() const {
    return getEitherInternalOrLeafNodes(false);
}

std::optional<const LineAwareOptimization::TreeTraversalNode> LineAwareOptimization::PostOrderTreeTraversal::getParentOfNode(std::size_t postOrderTraversalIdx) const {
    return postOrderTraversalIdx < parentPerNodeLookup.size()
    ? std::make_optional(treeNodesInPostOrder[*parentPerNodeLookup[postOrderTraversalIdx]])
    : std::nullopt;
}


std::optional<const LineAwareOptimization::TreeTraversalNode> LineAwareOptimization::PostOrderTreeTraversal::getNode(std::size_t postOrderTraversalIdx) const {
    return postOrderTraversalIdx < treeNodesInPostOrder.size()
        ? std::make_optional(treeNodesInPostOrder[postOrderTraversalIdx])
        : std::nullopt;
}

bool LineAwareOptimization::PostOrderTreeTraversal::areOperandsOnlyAdditionSubtractionOrXor() const {
    return std::all_of(
    treeNodesInPostOrder.cbegin(),
    treeNodesInPostOrder.cend(),
    [](const TreeTraversalNode& treeNode) {
        if (!treeNode.isInternalNode) {
            return true;
        }
        return isOperationAdditionSubtractionOrXor(std::get<syrec_operation::operation>(treeNode.nodeData));
    });
}

bool LineAwareOptimization::PostOrderTreeTraversal::doOperandsOperateOnLeaveNodesOnly(const std::vector<syrec_operation::operation>& operands) const {
    const auto&    leafNodeTraversalIndizes = getTraversalIndexOfAllLeafNodes();
    const std::set operandsOfInterest(operands.cbegin(), operands.cend());

    bool doOperandsOperateOnLeavesOnly = true;
    for (std::size_t i = 0; i < leafNodeTraversalIndizes.size() && doOperandsOperateOnLeavesOnly; ++i) {
        const auto& parentNodeOfLeaf    = *getParentOfNode(leafNodeTraversalIndizes[i]);
        const auto  operandOfParentNode = std::get<syrec_operation::operation>(parentNodeOfLeaf.nodeData);
        doOperandsOperateOnLeavesOnly &= operandsOfInterest.count(operandOfParentNode) != 0;
    }
    return doOperandsOperateOnLeavesOnly;
}

bool LineAwareOptimization::PostOrderTreeTraversal::doTwoConsecutiveOperationNodesHaveNotAdditionSubtractionOrXorAsOperand() const {
    const auto& operationNodesIdxs = getTraversalIndexOfAllOperationNodes();
    return std::any_of(
        operationNodesIdxs.cbegin(),
        operationNodesIdxs.cend(),
        [this](const std::size_t operationNodeTraversalIdx) {
            const auto& parentNode = getParentOfNode(operationNodeTraversalIdx);
            if (parentNode.has_value()) {
                const auto operationOfCurrentNode = std::get<syrec_operation::operation>(getNode(operationNodeTraversalIdx)->nodeData);
                const auto operationOfParentNode  = std::get<syrec_operation::operation>(parentNode->nodeData);
                return !isOperationAdditionSubtractionOrXor(operationOfCurrentNode) && !isOperationAdditionSubtractionOrXor(operationOfParentNode);
            }
            return false;
        }
    );
}

std::vector<std::optional<std::size_t>> LineAwareOptimization::PostOrderTreeTraversal::determineParentPerNode(const std::vector<TreeTraversalNode>& nodesInPostOrder) {
    std::stack<std::size_t>                 processedNodeIdxStack;
    std::vector<std::optional<std::size_t>> parentLookup(nodesInPostOrder.size(), std::nullopt);

    std::size_t currentTraversalIdx = 0;
    for (const auto& node: nodesInPostOrder) {
        if (node.isInternalNode) {
            const auto rightChildIdx = processedNodeIdxStack.top();
            processedNodeIdxStack.pop();
            const auto& leftChildIdx = processedNodeIdxStack.top();
            processedNodeIdxStack.pop();

            parentLookup[rightChildIdx] = currentTraversalIdx;
            parentLookup[leftChildIdx]  = currentTraversalIdx;
        }
        processedNodeIdxStack.push(currentTraversalIdx++);
    }
    return parentLookup;
}

std::vector<std::size_t> LineAwareOptimization::PostOrderTreeTraversal::getEitherInternalOrLeafNodes(bool getLeafNodes) const {
    std::vector<std::size_t> matchingNodes;
    for (std::size_t i = 0; i < treeNodesInPostOrder.size(); ++i) {
        if (!(getLeafNodes ^ treeNodesInPostOrder.at(i).isInternalNode)) {
            matchingNodes.emplace_back(i);
        }
    }
    return matchingNodes;
}


std::optional<syrec::AssignStatement::vec> LineAwareOptimization::optimize(const std::shared_ptr<syrec::AssignStatement>& assignmentStatement) {
    bool canTryAndOptimize;
    switch (assignmentStatement->op) {
        case syrec::AssignStatement::Add:
        case syrec::AssignStatement::Subtract:
        case syrec::AssignStatement::Exor:
            canTryAndOptimize = true;
            break;
        default:
            canTryAndOptimize = false;
            break;
    }

    const auto& rhsAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(assignmentStatement->rhs);
    canTryAndOptimize &= rhsAsBinaryExpr != nullptr;
    if (!canTryAndOptimize) {
        return std::nullopt;   
    }

    const auto x = createPostOrderRepresentation(assignmentStatement->rhs);

    // TODO:
    return std::nullopt;
}

std::optional<syrec_operation::operation> LineAwareOptimization::tryInvertAssignmentOperation(syrec_operation::operation operation) {
    std::optional<syrec_operation::operation> mappedToInverseOperation;
    switch (operation) {
        case syrec_operation::operation::add_assign:
            mappedToInverseOperation.emplace(syrec_operation::operation::minus_assign);
            break;
        case syrec_operation::operation::minus_assign:
            mappedToInverseOperation.emplace(syrec_operation::operation::add_assign);
            break;
        case syrec_operation::operation::xor_assign:
            mappedToInverseOperation.emplace(syrec_operation::operation::xor_assign);
            break;
        default:
            break;
    }
    return mappedToInverseOperation;
}




LineAwareOptimization::PostOrderTreeTraversal LineAwareOptimization::createPostOrderRepresentation(const syrec::BinaryExpression::ptr& topLevelBinaryExpr) {
    std::vector<TreeTraversalNode> postOrderNodeTraversalStorage;
    traverseExpressionOperand(topLevelBinaryExpr, postOrderNodeTraversalStorage);
    return PostOrderTreeTraversal(postOrderNodeTraversalStorage);
}

// TODO:
void LineAwareOptimization::traverseExpressionOperand(const syrec::expression::ptr& expr, std::vector<TreeTraversalNode>& postOrderTraversalContainer) {
    if (isExpressionOperandLeafNode(expr)) {
        if (const auto& exprAsVariableAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsVariableAccess != nullptr) {
            postOrderTraversalContainer.emplace_back(TreeTraversalNode::CreateLeafNode(exprAsVariableAccess));
        } else if (const auto& numericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(expr); numericExpr != nullptr) {
            postOrderTraversalContainer.emplace_back(TreeTraversalNode::CreateLeafNode(numericExpr));
        }
    }
    else {
        const auto& exprAsBinaryExpr = std::dynamic_pointer_cast<syrec::BinaryExpression>(expr);
        traverseExpressionOperand(exprAsBinaryExpr->lhs, postOrderTraversalContainer);
        traverseExpressionOperand(exprAsBinaryExpr->rhs, postOrderTraversalContainer);
        postOrderTraversalContainer.emplace_back(TreeTraversalNode::CreateInternalNode(*parser::ParserUtilities::mapInternalBinaryOperationFlagToEnum(exprAsBinaryExpr->op)));   
    }
}

bool LineAwareOptimization::isExpressionOperandLeafNode(const syrec::expression::ptr& expr) {
    if (const auto& exprAsVariableAccess = std::dynamic_pointer_cast<syrec::VariableExpression>(expr); exprAsVariableAccess != nullptr) {
        return true;
    }
    if (const auto& numericExpr = std::dynamic_pointer_cast<syrec::NumericExpression>(expr); numericExpr != nullptr) {
        return true;
    }
    return false;
}

bool LineAwareOptimization::isOperationAdditionSubtractionOrXor(syrec_operation::operation operation) {
    switch (operation) {
        case syrec_operation::operation::addition:
        case syrec_operation::operation::subtraction:
        case syrec_operation::operation::bitwise_xor:
            return true;
        default:
            return false;
    }
}
