#include "core/syrec/parser/utils/copy_utils.hpp"

using namespace copyUtils;

std::unique_ptr<syrec::Module> copyUtils::createCopyOfModule(const syrec::Module& module) {
    return std::make_unique<syrec::Module>(module);
}

std::vector<std::unique_ptr<syrec::Statement>> copyUtils::createCopyOfStatements(const syrec::Statement::vec& statements) {
    std::vector<std::unique_ptr<syrec::Statement>> statementCopyContainer;
    statementCopyContainer.reserve(statements.size());

    for (const auto& stmt: statements) {
        statementCopyContainer.emplace_back(createCopyOfStmt(*stmt));
    }
    return statementCopyContainer;
}

std::unique_ptr<syrec::Statement> copyUtils::createCopyOfStmt(const syrec::Statement& stmt) {
    if (typeid(stmt) == typeid(syrec::SkipStatement)) {
        return std::make_unique<syrec::SkipStatement>();
    }
    if (const auto& statementAsAssignmentStmt = dynamic_cast<const syrec::AssignStatement*>(&stmt); statementAsAssignmentStmt) {
        return std::make_unique<syrec::AssignStatement>(statementAsAssignmentStmt->lhs, statementAsAssignmentStmt->op, statementAsAssignmentStmt->rhs);
    }
    if (const auto& statementAsUnaryAssignmentStmt = dynamic_cast<const syrec::UnaryStatement*>(&stmt); statementAsUnaryAssignmentStmt) {
        return std::make_unique<syrec::UnaryStatement>(statementAsUnaryAssignmentStmt->op, statementAsUnaryAssignmentStmt->var);
    }
    if (const auto& statementAsIfStatement = dynamic_cast<const syrec::IfStatement*>(&stmt); statementAsIfStatement) {
        auto copyOfIfStatement            = std::make_unique<syrec::IfStatement>();
        copyOfIfStatement->condition      = statementAsIfStatement->condition;
        copyOfIfStatement->fiCondition    = statementAsIfStatement->fiCondition;
        copyOfIfStatement->thenStatements = statementAsIfStatement->thenStatements;
        copyOfIfStatement->elseStatements = statementAsIfStatement->elseStatements;
        return copyOfIfStatement;
    }
    if (const auto& statementAsLoopStatement = dynamic_cast<const syrec::ForStatement*>(&stmt); statementAsLoopStatement) {
        auto copyOfLoopStatement          = std::make_unique<syrec::ForStatement>();
        copyOfLoopStatement->loopVariable = statementAsLoopStatement->loopVariable;
        copyOfLoopStatement->range        = statementAsLoopStatement->range;
        copyOfLoopStatement->step         = statementAsLoopStatement->step;
        copyOfLoopStatement->statements   = statementAsLoopStatement->statements;
        return copyOfLoopStatement;
    }
    if (const auto& statementAsSwapStatement = dynamic_cast<const syrec::SwapStatement*>(&stmt); statementAsSwapStatement) {
        return std::make_unique<syrec::SwapStatement>(statementAsSwapStatement->lhs, statementAsSwapStatement->rhs);
    }
    if (const auto& statementAsCallStatement = dynamic_cast<const syrec::CallStatement*>(&stmt); statementAsCallStatement) {
        return std::make_unique<syrec::CallStatement>(statementAsCallStatement->target, statementAsCallStatement->parameters);
    }
    if (const auto& statementAsUncallStatement = dynamic_cast<const syrec::UncallStatement*>(&stmt); statementAsUncallStatement) {
        return std::make_unique<syrec::UncallStatement>(statementAsUncallStatement->target, statementAsUncallStatement->parameters);
    }
    return nullptr;
}

std::unique_ptr<syrec::Expression> copyUtils::createCopyOfExpression(const syrec::Expression& expr) {
    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&expr); exprAsBinaryExpr) {
        return std::make_unique<syrec::BinaryExpression>(
                exprAsBinaryExpr->lhs,
                exprAsBinaryExpr->op,
                exprAsBinaryExpr->rhs);
    }
    if (const auto& exprAsShiftExpr = dynamic_cast<const syrec::ShiftExpression*>(&expr); exprAsShiftExpr) {
        return std::make_unique<syrec::ShiftExpression>(
                exprAsShiftExpr->lhs,
                exprAsShiftExpr->op,
                exprAsShiftExpr->rhs);
    }
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr) {
        return std::make_unique<syrec::NumericExpression>(
                createCopyOfNumber(*exprAsNumericExpr->value),
                exprAsNumericExpr->bwidth);
    }
    if (const auto& exprAsVariableExpr = dynamic_cast<const syrec::VariableExpression*>(&expr); exprAsVariableExpr) {
        return std::make_unique<syrec::VariableExpression>(exprAsVariableExpr->var);
    }
    return nullptr;
}

std::unique_ptr<syrec::Number> copyUtils::createCopyOfNumber(const syrec::Number& number) {
    if (number.isConstant()) {
        return std::make_unique<syrec::Number>(number.evaluate({}));
    }
    if (number.isLoopVariable()) {
        return std::make_unique<syrec::Number>(number.variableName());
    }
    if (number.isCompileTimeConstantExpression()) {
        return std::make_unique<syrec::Number>(number.getExpression());
    }
    return nullptr;
}

std::unique_ptr<syrec::AssignStatement> copyUtils::createDeepCopyOfAssignmentStmt(const syrec::AssignStatement& stmt) {
    if (std::unique_ptr<syrec::Expression> copyOfRhsOperand = createDeepCopyOfExpression(*stmt.rhs); copyOfRhsOperand) {
        return std::make_unique<syrec::AssignStatement>(stmt.lhs, stmt.op, std::move(copyOfRhsOperand));    
    }
    return nullptr;
}

std::unique_ptr<syrec::Expression> copyUtils::createDeepCopyOfExpression(const syrec::Expression& expr) {
    if (const auto& exprAsBinaryExpr = dynamic_cast<const syrec::BinaryExpression*>(&expr); exprAsBinaryExpr) {
        std::unique_ptr<syrec::Expression> deepCopyOfLhsOperand = createDeepCopyOfExpression(*exprAsBinaryExpr->lhs);
        std::unique_ptr<syrec::Expression> deepCopyOfRhsOperand = createDeepCopyOfExpression(*exprAsBinaryExpr->rhs);
        if (deepCopyOfLhsOperand && deepCopyOfRhsOperand) {
            return std::make_unique<syrec::BinaryExpression>(
                    std::move(deepCopyOfLhsOperand),
                    exprAsBinaryExpr->op,
                    std::move(deepCopyOfRhsOperand));            
        }
    }
    if (const auto& exprAsShiftExpr = dynamic_cast<const syrec::ShiftExpression*>(&expr); exprAsShiftExpr) {
        std::unique_ptr<syrec::Expression> deepCopyOfShiftExpr = createDeepCopyOfExpression(*exprAsShiftExpr->lhs);
        std::unique_ptr<syrec::Number>     deepCopyOfShiftAmount = createCopyOfNumber(*exprAsShiftExpr->rhs);

        if (deepCopyOfShiftExpr && deepCopyOfShiftAmount) {
            return std::make_unique<syrec::ShiftExpression>(
                    std::move(deepCopyOfShiftExpr),
                    exprAsShiftExpr->op,
                    std::move(deepCopyOfShiftAmount));   
        }
    }
    if (const auto& exprAsNumericExpr = dynamic_cast<const syrec::NumericExpression*>(&expr); exprAsNumericExpr) {
        return std::make_unique<syrec::NumericExpression>(
                createCopyOfNumber(*exprAsNumericExpr->value),
                exprAsNumericExpr->bwidth);
    }
    if (const auto& exprAsVariableExpr = dynamic_cast<const syrec::VariableExpression*>(&expr); exprAsVariableExpr) {
        return std::make_unique<syrec::VariableExpression>(exprAsVariableExpr->var);
    }
    return nullptr;
}


