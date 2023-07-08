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
        /*createAndPushNewStatementBlock(statementAsIfStatement->elseStatements, BlockType::IfConditionFalseBranch);
        appendStatementsWithBlockTypeSwitch(statementAsIfStatement->thenStatements, BlockType::IfConditionTrueBranch, statementAsIfStatement->elseStatements.size());*/
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
    /*if (perControlBlockRelativeStatementCounter.size() == 1) {
        return perControlBlockRelativeStatementCounter;    
    }
    return {std::next(perControlBlockRelativeStatementCounter.begin()), perControlBlockRelativeStatementCounter.end()};*/
    std::vector<StatementIterationHelper::StatementIndexInBlock> builtIndexForStatement = std::vector(perControlBlockRelativeStatementCounter.begin(), perControlBlockRelativeStatementCounter.end());
    builtIndexForStatement.back().relativeIndexInBlock                                = statementIndexInCurrentBlock;
    return builtIndexForStatement;
}

void StatementIterationHelper::createAndPushNewStatementBlock(const syrec::Statement::vec& statements, BlockType typeOfNewBlock) {
    remainingStatementsToParse.push(statements);
    perControlBlockRelativeStatementCounter.back().relativeIndexInBlock = statementIndexInCurrentBlock;
    statementIndexInCurrentBlock                   = 0;
    perControlBlockRelativeStatementCounter.emplace_back(StatementIndexInBlock(typeOfNewBlock, statementIndexInCurrentBlock));
}

void StatementIterationHelper::appendStatementsWithBlockTypeSwitch(const syrec::Statement::vec& statements, BlockType typeOfNewBlock, std::size_t offsetForBlockSwitch) {
    auto& currentRemainingStatementsToParse = remainingStatementsToParse.top();
    currentRemainingStatementsToParse.insert(std::end(currentRemainingStatementsToParse), statements.begin(), statements.end());
    blockTypeSwitches.push(BlockTypeSwitch(offsetForBlockSwitch, typeOfNewBlock));
}

void StatementIterationHelper::updateStatementIndexInCurrentBlock() {
    if (perControlBlockRelativeStatementCounter.empty()) {
        statementIndexInCurrentBlock = 0;
        return;
    }
    
    ++perControlBlockRelativeStatementCounter.back().relativeIndexInBlock;
    ++statementIndexInCurrentBlock;
    if (!blockTypeSwitches.empty() && statementIndexInCurrentBlock == blockTypeSwitches.top().activeStartingFromStatementWithIndex) {
        perControlBlockRelativeStatementCounter.back().blockType = blockTypeSwitches.top().switchedToBlockType;
        statementIndexInCurrentBlock                             = 0;
        blockTypeSwitches.pop();
    }
}