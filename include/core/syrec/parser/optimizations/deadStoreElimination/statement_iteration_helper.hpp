#ifndef STATEMENT_ITERATION_HELPER_HPP
#define STATEMENT_ITERATION_HELPER_HPP
#pragma once
#include "core/syrec/statement.hpp"

#include <stack>

namespace deadStoreElimination {
    class StatementIterationHelper {
    public:
        enum BlockType {
            Module,
            Loop,
            IfConditionTrueBranch,
            IfConditionFalseBranch
        };

        struct StatementIndexInBlock {
            BlockType blockType;
            std::size_t relativeIndexInBlock;

            StatementIndexInBlock(BlockType blockType, std::size_t relativeIndexInBlock):
                blockType(blockType), relativeIndexInBlock(relativeIndexInBlock) {}
        };

        struct StatementAndRelativeIndexPair {
            const syrec::Statement::ptr    statement;
            const std::vector<StatementIndexInBlock> relativeIndexInControlFlowGraph;

            StatementAndRelativeIndexPair(syrec::Statement::ptr statement):
                StatementAndRelativeIndexPair(std::move(statement), {}) {}

            StatementAndRelativeIndexPair(syrec::Statement::ptr statement, std::vector<StatementIndexInBlock> relativeIndexInControlFlowGraph):
                statement(std::move(statement)), relativeIndexInControlFlowGraph(std::move(relativeIndexInControlFlowGraph)) {}
        };

        StatementIterationHelper(const syrec::Statement::vec& statementsToIterate) {
            statementIndexInCurrentBlock = 0;
            perControlBlockRelativeStatementCounter.emplace_back(StatementIndexInBlock(BlockType::Module, statementIndexInCurrentBlock));
            remainingStatementsToParse.push(statementsToIterate);
        }
            

        // TODO: Naming, module call representing dead store is technically also a control flow statement
        [[nodiscard]] std::optional<StatementAndRelativeIndexPair> getNextStatement();

    private:
        struct BlockTypeSwitch {
            std::size_t activeStartingFromStatementWithIndex;
            BlockType   switchedToBlockType;

            BlockTypeSwitch(std::size_t activeStartingFromStatementWithIndex, BlockType switchedToBlockType):
                activeStartingFromStatementWithIndex(activeStartingFromStatementWithIndex), switchedToBlockType(switchedToBlockType) {}
        };

        std::vector<StatementIndexInBlock> perControlBlockRelativeStatementCounter;
        std::stack<syrec::Statement::vec>  remainingStatementsToParse;
        std::stack<BlockTypeSwitch>        blockTypeSwitches;
        std::size_t                        statementIndexInCurrentBlock;

        void                                             createAndPushNewStatementBlock(const syrec::Statement::vec& statements, BlockType typeOfNewBlock);
        void                                             appendStatementsWithBlockTypeSwitch(const syrec::Statement::vec& statements, BlockType typeOfNewBlock, std::size_t offsetForBlockSwitch);
        void                                             updateStatementIndexInCurrentBlock();
        [[nodiscard]] std::vector<StatementIndexInBlock> buildRelativeIndexForCurrentStatement() const;
    };
}

#endif