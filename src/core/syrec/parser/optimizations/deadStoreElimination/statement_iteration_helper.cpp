#include "core/syrec/parser/optimizations/deadStoreElimination/statement_iteration_helper.hpp"

#include <algorithm>

using namespace deadStoreElimination;

std::optional<StatementIterationHelper::StatementAndRelativeIndexPair> StatementIterationHelper::getNextNonControlFlowStatement() {
    if (remainingStatementsToParse.empty()) {
        return std::nullopt;
    }

    std::optional<syrec::Statement::ptr> nextStatement;
    while (!remainingStatementsToParse.empty() && !nextStatement.has_value()) {
        const auto& statementsToIterate                       = remainingStatementsToParse.top();
        const auto  numberOfRemainingStatementsInCurrentBlock = statementsToIterate.empty() ? 0 : (statementsToIterate.size() - statementIndexInCurrentBlock);
        if (numberOfRemainingStatementsInCurrentBlock == 0) {
            remainingStatementsToParse.pop();
            perControlBlockRelativeStatementCounter.erase(std::prev(perControlBlockRelativeStatementCounter.end()));
        } else {
            nextStatement.emplace(statementsToIterate.at(statementIndexInCurrentBlock));
            perControlBlockRelativeStatementCounter.back()++;
        }
        statementIndexInCurrentBlock = !perControlBlockRelativeStatementCounter.empty() ? perControlBlockRelativeStatementCounter.back() : 0;
    }

    if (!nextStatement.has_value()) {
        return std::nullopt;
    }
    const auto& statementToProcess = *nextStatement;
    if (const auto statementAsIfStatement = std::dynamic_pointer_cast<syrec::IfStatement>(statementToProcess); statementAsIfStatement != nullptr) {
        const auto statementAndIndexPair = StatementAndRelativeIndexPair(statementAsIfStatement, buildRelativeIndexForCurrentStatement());
        //const auto currentNestingLevelOfControlFlowGraph = perControlBlockRelativeStatementCounter.size();
        createAndPushNewStatementBlock(statementAsIfStatement->elseStatements);
        createAndPushNewStatementBlock(statementAsIfStatement->thenStatements);
        //perControlBlockRelativeStatementCounter.at(currentNestingLevelOfControlFlowGraph - 1) += 1;
        return std::make_optional(statementAndIndexPair);
    }

    if (const auto statementAsLoopStatement = std::dynamic_pointer_cast<syrec::ForStatement>(statementToProcess); statementAsLoopStatement != nullptr) {
        //const auto currentNestingLevelOfControlFlowGraph = perControlBlockRelativeStatementCounter.size();
        createAndPushNewStatementBlock(statementAsLoopStatement->statements);
        //perControlBlockRelativeStatementCounter.at(currentNestingLevelOfControlFlowGraph - 1) += 1;
        return getNextNonControlFlowStatement();
    }

    const auto statementAndIndexPair = StatementAndRelativeIndexPair(statementToProcess, buildRelativeIndexForCurrentStatement());
    //statementIndexInCurrentBlock++;
    return std::make_optional(statementAndIndexPair);
}

[[nodiscard]] std::vector<std::size_t> StatementIterationHelper::buildRelativeIndexForCurrentStatement() const {
    return perControlBlockRelativeStatementCounter;
    //std::vector<std::size_t> relativeIndex;

    ///*
    // * Indizes of previous blocks reference the statement after the new block in the control flow graph
    // * thus we decrement the stored index to reference the original position
    // */
    //std::transform(
    //        perControlBlockRelativeStatementCounter.cbegin(),
    //        perControlBlockRelativeStatementCounter.cend(),
    //        std::back_inserter(relativeIndex),
    //        [](const std::size_t relativeStatementCounterInBlock) {
    //            return relativeStatementCounterInBlock - 1;
    //        });
    //// But this decrement should not be done for the current block since we are creating its relative index before creating any new block in the control flow graph
    //relativeIndex.back() = statementIndexInCurrentBlock;
    //return relativeIndex;
}

void StatementIterationHelper::createAndPushNewStatementBlock(const syrec::Statement::vec& statements) {
    remainingStatementsToParse.push(statements);
    perControlBlockRelativeStatementCounter.back() = statementIndexInCurrentBlock;
    statementIndexInCurrentBlock                   = 0;
    perControlBlockRelativeStatementCounter.emplace_back(statementIndexInCurrentBlock);
}
