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

        explicit StatementIterationHelper(const syrec::Statement::vec& statementsToIterate) {
            statementIndexInCurrentBlock = 0;
            perControlBlockRelativeStatementCounter.emplace_back(StatementIndexInBlock(BlockType::Module, statementIndexInCurrentBlock));
            remainingStatementsToParse.push(statementsToIterate);
        }

        [[nodiscard]] std::optional<StatementAndRelativeIndexPair> getNextStatement();

    private:
        struct BlockTypeSwitch {
            std::size_t activeForBlockAtNestingLevel;
            std::size_t activeStartingFromStatementWithIndex;
            BlockType   switchedToBlockType;

            BlockTypeSwitch(std::size_t activeForBlockAtNestingLevel, std::size_t activeStartingFromStatementWithIndex, BlockType switchedToBlockType):
                activeForBlockAtNestingLevel(activeForBlockAtNestingLevel), activeStartingFromStatementWithIndex(activeStartingFromStatementWithIndex), switchedToBlockType(switchedToBlockType) {}
        };

        struct PerBlockRelativeStatementIndexNormalizer {
            std::size_t activeForBlockAtNestingLevel;
            std::size_t activeStartingFromStatementWithIndex;

            PerBlockRelativeStatementIndexNormalizer(std::size_t activeForBlockAtNestingLevel,
                                                     std::size_t activeStartingFromStatementWithIndex):
                activeForBlockAtNestingLevel(activeForBlockAtNestingLevel), activeStartingFromStatementWithIndex(activeStartingFromStatementWithIndex) {}
        };
        
        std::vector<StatementIndexInBlock>                    perControlBlockRelativeStatementCounter;
        std::stack<syrec::Statement::vec>                     remainingStatementsToParse;
        std::vector<BlockTypeSwitch>                          blockTypeSwitches;
        std::vector<PerBlockRelativeStatementIndexNormalizer> relativeStatementIndexNormalizers;
        
        std::size_t                          statementIndexInCurrentBlock;

        void                                             createAndPushNewStatementBlock(const syrec::Statement::vec& statements, BlockType typeOfNewBlock);
        void                                             appendStatementsWithBlockTypeSwitch(const syrec::Statement::vec& statements, BlockType typeOfNewBlock, std::size_t offsetForBlockSwitch);
        void                                             updateStatementIndexInCurrentBlock();
        [[nodiscard]] std::vector<StatementIndexInBlock> buildRelativeIndexForCurrentStatement() const;
    };
}

#endif