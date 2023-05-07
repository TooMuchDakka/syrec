#ifndef NO_ADDITIONAL_LINE_ASSIGNMENT_HPP
#define NO_ADDITIONAL_LINE_ASSIGNMENT_HPP
#pragma once

#include "core/syrec/statement.hpp"
#include "core/syrec/parser/operation.hpp"

#include <optional>

namespace optimizations {
	class LineAwareOptimization {
	public:
	    [[nodiscard]] static std::optional<syrec::AssignStatement::vec> optimize(const std::shared_ptr<syrec::AssignStatement>& assignmentStatement);

	protected:
		struct TreeTraversalNode {
			using LeafData = std::variant<std::shared_ptr<syrec::VariableExpression>, std::shared_ptr<syrec::NumericExpression>>;

            const bool isInternalNode;
            const std::variant<LeafData, syrec_operation::operation> nodeData;

		    static TreeTraversalNode CreateInternalNode(const syrec_operation::operation& operation) {
                return TreeTraversalNode(true, operation);
		    }

			static TreeTraversalNode CreateLeafNode(const LeafData& leafData) {
                return TreeTraversalNode(false, leafData);
		    }

		private:
			explicit TreeTraversalNode(bool isInternalNode, std::variant<LeafData, syrec_operation::operation> nodeData):
                isInternalNode(isInternalNode), nodeData(std::move(nodeData)) { }
		};
		
		struct PostOrderTreeTraversal {
            explicit PostOrderTreeTraversal(const std::vector<TreeTraversalNode>& nodesInPostOrder):
		        treeNodesInPostOrder(nodesInPostOrder), parentPerNodeLookup(determineParentPerNode(nodesInPostOrder)) {}

            [[nodiscard]] std::vector<std::size_t>               getTraversalIndexOfAllLeafNodes() const;
            [[nodiscard]] std::vector<std::size_t>               getTraversalIndexOfAllOperationNodes() const;
            [[nodiscard]] std::optional<const TreeTraversalNode> getParentOfNode(std::size_t postOrderTraversalIdx) const;
            [[nodiscard]] std::optional<const TreeTraversalNode> getNode(std::size_t postOrderTraversalIdx) const;

			[[nodiscard]] bool areOperandsOnlyAdditionSubtractionOrXor() const;
            [[nodiscard]] bool doOperandsOperateOnLeaveNodesOnly(const std::vector<syrec_operation::operation>& operands) const;
            [[nodiscard]] bool doTwoConsecutiveOperationNodesHaveNotAdditionSubtractionOrXorAsOperand() const;
		private:
            const std::vector<TreeTraversalNode>          treeNodesInPostOrder;
            const std::vector<std::optional<std::size_t>> parentPerNodeLookup;

			[[nodiscard]] static std::vector<std::optional<std::size_t>> determineParentPerNode(const std::vector<TreeTraversalNode>& nodesInPostOrder);
            [[nodiscard]] std::vector<std::size_t>                       getEitherInternalOrLeafNodes(bool getLeafNodes) const;
        };
		
        [[nodiscard]] static std::optional<syrec_operation::operation> tryInvertAssignmentOperation(syrec_operation::operation operation);
        [[nodiscard]] static PostOrderTreeTraversal                    createPostOrderRepresentation(const syrec::BinaryExpression::ptr& topLevelBinaryExpr);
        static void                                                    traverseExpressionOperand(const syrec::expression::ptr& expr, std::vector<TreeTraversalNode>& postOrderTraversalContainer);
        [[nodiscard]] static bool                                      isExpressionOperandLeafNode(const syrec::expression::ptr& expr);
        [[nodiscard]] static bool                                      isOperationAdditionSubtractionOrXor(syrec_operation::operation operation);
	};

};
#endif