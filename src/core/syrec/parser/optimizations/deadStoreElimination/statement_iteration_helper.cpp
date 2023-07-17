#include "core/syrec/parser/optimizations/deadStoreElimination/statement_iteration_helper.hpp"
#include <algorithm>

using namespace deadStoreElimination;

std::optional<StatementIterationHelper::StatementAndRelativeIndexPair> StatementIterationHelper::getNextStatement() {
    if (remainingStatementsToParse.empty()) {
        return std::nullopt;
    }

    std::optional<syrec::Statement::ptr> nextStatement;
    while (!remainingStatementsToParse.empty() && !nextStatement.has_value()) {
        const auto& statementsToIterate                       = remainingStatementsToParse.top();
        const auto  numberOfRemainingStatementsInCurrentBlock = statementsToIterate.empty() ? 0 : (statementsToIterate.size() - perControlBlockRelativeStatementCounter.back().relativeIndexInBlock);
        if (numberOfRemainingStatementsInCurrentBlock == 0) {
            remainingStatementsToParse.pop();
            perControlBlockRelativeStatementCounter.erase(std::prev(perControlBlockRelativeStatementCounter.end()));
            if (!perControlBlockRelativeStatementCounter.empty()) {
                statementIndexInCurrentBlock = perControlBlockRelativeStatementCounter.back().relativeIndexInBlock;
            }

            if (!relativeStatementIndexNormalizers.empty() && relativeStatementIndexNormalizers.back().activeForBlockAtNestingLevel > perControlBlockRelativeStatementCounter.size() - 1) {
                relativeStatementIndexNormalizers.erase(std::prev(relativeStatementIndexNormalizers.cend()));
            }
            updateStatementIndexInCurrentBlock();
        } else {
            nextStatement.emplace(statementsToIterate.at(perControlBlockRelativeStatementCounter.back().relativeIndexInBlock));
        }
    }

    if (!nextStatement.has_value()) {
        return std::nullopt;
    }
    const auto& statementToProcess = *nextStatement;
    if (const auto statementAsIfStatement = std::dynamic_pointer_cast<syrec::IfStatement>(statementToProcess); statementAsIfStatement != nullptr) {
        const auto statementAndIndexPair = StatementAndRelativeIndexPair(statementAsIfStatement, buildRelativeIndexForCurrentStatement());
        createAndPushNewStatementBlock(statementAsIfStatement->thenStatements, BlockType::IfConditionTrueBranch);
        appendStatementsWithBlockTypeSwitch(statementAsIfStatement->elseStatements, BlockType::IfConditionFalseBranch, statementAsIfStatement->thenStatements.size());
        return std::make_optional(statementAndIndexPair);
    }

    if (const auto statementAsLoopStatement = std::dynamic_pointer_cast<syrec::ForStatement>(statementToProcess); statementAsLoopStatement != nullptr) {
        const auto statementAndIndexPair = StatementAndRelativeIndexPair(statementAsLoopStatement, buildRelativeIndexForCurrentStatement());
        createAndPushNewStatementBlock(statementAsLoopStatement->statements, BlockType::Loop);
        return std::make_optional(statementAndIndexPair);
    }

    const auto statementAndIndexPair = StatementAndRelativeIndexPair(statementToProcess, buildRelativeIndexForCurrentStatement());
    updateStatementIndexInCurrentBlock();
    return std::make_optional(statementAndIndexPair);
}

[[nodiscard]] std::vector<StatementIterationHelper::StatementIndexInBlock> StatementIterationHelper::buildRelativeIndexForCurrentStatement() const {
    std::vector<StatementIndexInBlock> builtIndexForStatement = std::vector(perControlBlockRelativeStatementCounter.begin(), perControlBlockRelativeStatementCounter.end());
    

    if (relativeStatementIndexNormalizers.empty()) {
        builtIndexForStatement.back().relativeIndexInBlock = statementIndexInCurrentBlock;    
    } else {
        for (std::size_t i = 0; i < relativeStatementIndexNormalizers.size(); ++i) {
            const auto& relativeStatementIndexNormalizerForCurrBlock = relativeStatementIndexNormalizers.at(i);
            const auto  indexOfEffectedBlockForBlockTypeSwitch       = relativeStatementIndexNormalizerForCurrBlock.activeForBlockAtNestingLevel;
            if (perControlBlockRelativeStatementCounter.at(indexOfEffectedBlockForBlockTypeSwitch).relativeIndexInBlock >= relativeStatementIndexNormalizerForCurrBlock.activeStartingFromStatementWithIndex) {
                builtIndexForStatement.at(indexOfEffectedBlockForBlockTypeSwitch).relativeIndexInBlock -= relativeStatementIndexNormalizerForCurrBlock.activeStartingFromStatementWithIndex;
            }
        }
    }
    return builtIndexForStatement;
}

void StatementIterationHelper::createAndPushNewStatementBlock(const syrec::Statement::vec& statements, BlockType typeOfNewBlock) {
    remainingStatementsToParse.push(statements);
    statementIndexInCurrentBlock                   = 0;
    perControlBlockRelativeStatementCounter.emplace_back(StatementIndexInBlock(typeOfNewBlock, statementIndexInCurrentBlock));
}

void StatementIterationHelper::appendStatementsWithBlockTypeSwitch(const syrec::Statement::vec& statements, BlockType typeOfNewBlock, std::size_t offsetForBlockSwitch) {
    auto& currentRemainingStatementsToParse = remainingStatementsToParse.top();
    currentRemainingStatementsToParse.insert(std::end(currentRemainingStatementsToParse), statements.begin(), statements.end());
    blockTypeSwitches.emplace_back(BlockTypeSwitch(perControlBlockRelativeStatementCounter.size(), offsetForBlockSwitch, typeOfNewBlock));
    relativeStatementIndexNormalizers.emplace_back(PerBlockRelativeStatementIndexNormalizer(perControlBlockRelativeStatementCounter.size() - 1, offsetForBlockSwitch));
}

void StatementIterationHelper::updateStatementIndexInCurrentBlock() {
    if (perControlBlockRelativeStatementCounter.empty()) {
        statementIndexInCurrentBlock = 0;
        return;
    }
    
    ++perControlBlockRelativeStatementCounter.back().relativeIndexInBlock;
    ++statementIndexInCurrentBlock;
    if (!blockTypeSwitches.empty() 
        && perControlBlockRelativeStatementCounter.size() == blockTypeSwitches.back().activeForBlockAtNestingLevel 
        && statementIndexInCurrentBlock == blockTypeSwitches.back().activeStartingFromStatementWithIndex) {
        perControlBlockRelativeStatementCounter.back().blockType = blockTypeSwitches.back().switchedToBlockType;
        statementIndexInCurrentBlock                             = 0;
        blockTypeSwitches.erase(std::prev(blockTypeSwitches.cend()));
    }
}