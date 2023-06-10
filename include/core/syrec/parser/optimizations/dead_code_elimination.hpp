#ifndef DEAD_CODE_ELIMINATION_HPP
#define DEAD_CODE_ELIMINATION_HPP

#include "core/syrec/statement.hpp"
#include "core/syrec/parser/symbol_table.hpp"

namespace optimizations {
    class DeadCodeElimination {
    public:
        void tryRemove(syrec::Statement::ptr& stmt, parser::SymbolTable::ptr& symbolTable);
        void removeDeadParametersAndLocals(syrec::Module::ptr& module, parser::SymbolTable::ptr& symbolTable);
        void removeUnneededModules(parser::SymbolTable::ptr& symbolTable, const std::string_view& moduleExcludedFromRemoval);

    private:
        void tryRemove(std::shared_ptr<syrec::IfStatement>& ifStmt, parser::SymbolTable::ptr& symbolTable);
        void tryRemove(std::shared_ptr<syrec::AssignStatement>& assignmentStmt, parser::SymbolTable::ptr& symbolTable);
    };
    
};

#endif