#ifndef STATEMENT_ITERATION_HELPER_HPP
#define STATEMENT_ITERATION_HELPER_HPP
#pragma once
#include "core/syrec/statement.hpp"

#include <stack>

namespace deadStoreElimination {
    class StatementIterationHelper {
    public:
        struct StatementAndRelativeIndexPair {
            const syrec::Statement::ptr    statement;
            const std::vector<std::size_t> relativeIndexInControlFlowGraph;

            StatementAndRelativeIndexPair(syrec::Statement::ptr statement):
                StatementAndRelativeIndexPair(statement, {}) {}

            StatementAndRelativeIndexPair(syrec::Statement::ptr statement, std::vector<std::size_t> relativeIndexInControlFlowGraph):
                statement(std::move(statement)), relativeIndexInControlFlowGraph(std::move(relativeIndexInControlFlowGraph)) {}
        };

        StatementIterationHelper(const syrec::Statement::vec& statementsToIterate) {
            statementIndexInCurrentBlock = 0;
            perControlBlockRelativeStatementCounter.emplace_back(statementIndexInCurrentBlock);
            remainingStatementsToParse.push(statementsToIterate);
        }
            

        // TODO: Naming, module call representing dead store is technically also a control flow statement
        [[nodiscard]] std::optional<StatementAndRelativeIndexPair> getNextNonControlFlowStatement();

    private:
        std::vector<std::size_t>          perControlBlockRelativeStatementCounter;
        std::stack<syrec::Statement::vec> remainingStatementsToParse;
        std::size_t                       statementIndexInCurrentBlock;

        void                                   createAndPushNewStatementBlock(const syrec::Statement::vec& statements);
        [[nodiscard]] std::vector<std::size_t> buildRelativeIndexForCurrentStatement() const;
    };
}

#endif