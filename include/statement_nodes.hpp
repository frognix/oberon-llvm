#pragma once

#include "statement.hpp"
#include "expression.hpp"

namespace nodes {

struct Assignment : Statement {
    std::string to_string() const { return fmt::format("{} := {}", variable.to_string(), value); }
    virtual Error check(const SymbolTable&) const override;
    Assignment(Designator var, ExpressionPtr val) : variable(var), value(val) {}
    Designator variable;
    ExpressionPtr value;
};

using IfBlock = std::tuple<ExpressionPtr, StatementSequence>;

struct IfStatement : Statement {
    std::string to_string() const {
        auto [cond, block] = if_blocks[0];
        std::vector<IfBlock> elsif_blocks = {if_blocks.begin() + 1, if_blocks.end()};
        if (else_block)
            return fmt::format("IF {} THEN\n{}\n{}ELSE {}\nEND", cond, block, fmt::join(elsif_blocks, "\n"),
                               *else_block);
        else
            return fmt::format("IF {} THEN\n{}\n{}END", cond, block, fmt::join(elsif_blocks, "\n"));
    }
    Error check(const SymbolTable&) const override;
    IfStatement(std::vector<IfBlock> ib, std::optional<StatementSequence> eb) : if_blocks(ib), else_block(eb) {}
    std::vector<IfBlock> if_blocks;
    std::optional<StatementSequence> else_block;
};

struct CaseLabel {
    ExpressionPtr first;
    std::optional<ExpressionPtr> second;
};

using CaseLabelList = std::vector<CaseLabel>;

using Case = std::tuple<CaseLabelList, StatementSequence>;

struct CaseStatement : Statement {
    std::string to_string() const { return fmt::format("CASE {} OF {} END", expression, fmt::join(cases, " |\n")); }
    Error check(const SymbolTable&) const override;
    CaseStatement(ExpressionPtr e, std::vector<Case> c) : expression(e), cases(c) {}
    ExpressionPtr expression;
    std::vector<Case> cases;
};

struct WhileStatement : Statement {
    std::string to_string() const {
        auto [cond, block] = if_blocks[0];
        std::vector<IfBlock> elsif_blocks = {if_blocks.begin() + 1, if_blocks.end()};
        return fmt::format("WHILE {} DO\n{}\n{}END", cond, block, fmt::join(elsif_blocks, "\n"));
    }
    Error check(const SymbolTable&) const override;
    WhileStatement(std::vector<IfBlock> ib) : if_blocks(ib) {}
    std::vector<IfBlock> if_blocks;
};

struct RepeatStatement : Statement {
    std::string to_string() const {
        auto [cond, block] = if_block;
        return fmt::format("REPEAT {} UNTIL\n{}", block, cond);
    }
    Error check(const SymbolTable&) const override;
    RepeatStatement(StatementSequence s, ExpressionPtr e) : if_block({e, s}) {}
    IfBlock if_block;
};

struct ForStatement : Statement {
    std::string to_string() const {
        if (by_expr)
            return fmt::format("FOR {} := {} TO {} BY {} DO\n{}\nEND", ident, for_expr, to_expr, *by_expr, block);
        else
            return fmt::format("FOR {} := {} TO {} DO\n{}\nEND", ident, for_expr, to_expr, block);
    }
    Error check(const SymbolTable&) const override;
    ForStatement(Ident i, ExpressionPtr f, ExpressionPtr to, std::optional<ExpressionPtr> by, StatementSequence b)
        : ident(i), for_expr(f), to_expr(to), by_expr(by), block(b) {}
    Ident ident;
    ExpressionPtr for_expr;
    ExpressionPtr to_expr;
    std::optional<ExpressionPtr> by_expr;
    StatementSequence block;
};

}
