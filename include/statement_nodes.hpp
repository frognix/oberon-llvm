#pragma once

#include "expression_nodes.hpp"
#include "statement.hpp"

namespace nodes {

struct Assignment : Statement {
    std::string to_string() const override;
    virtual Error check(const SymbolTable&) const override;
    Assignment(Designator var, ExpressionPtr val) : variable(var), value(val) {}
    Designator variable;
    ExpressionPtr value;
};

using IfBlock = std::tuple<ExpressionPtr, StatementSequence>;

struct IfStatement : Statement {
    std::string to_string() const override;
    Error check(const SymbolTable&) const override;
    IfStatement(std::vector<IfBlock> ib, std::optional<StatementSequence> eb) : if_blocks(ib), else_block(eb) {}
    std::vector<IfBlock> if_blocks;
    std::optional<StatementSequence> else_block;
};

using Label = std::variant<Integer,String,QualIdent>;

struct CaseLabel {
    Label first;
    std::optional<Label> second;
};

using CaseLabelList = std::vector<CaseLabel>;

using Case = std::tuple<CaseLabelList, StatementSequence>;

struct CaseStatement : Statement {
    std::string to_string() const override;
    Error check(const SymbolTable&) const override;
    CaseStatement(ExpressionPtr e, std::vector<Case> c) : expression(e), cases(c) {}
    ExpressionPtr expression;
    std::vector<Case> cases;
};

struct WhileStatement : Statement {
    std::string to_string() const override;
    Error check(const SymbolTable&) const override;
    WhileStatement(std::vector<IfBlock> ib) : if_blocks(ib) {}
    std::vector<IfBlock> if_blocks;
};

struct RepeatStatement : Statement {
    std::string to_string() const override;
    Error check(const SymbolTable&) const override;
    RepeatStatement(StatementSequence s, ExpressionPtr e) : if_block({e, s}) {}
    IfBlock if_block;
};

struct ForStatement : Statement {
    std::string to_string() const override;
    Error check(const SymbolTable&) const override;
    ForStatement(Ident i, ExpressionPtr f, ExpressionPtr to, std::optional<ExpressionPtr> by, StatementSequence b)
        : ident(i), for_expr(f), to_expr(to), by_expr(by), block(b) {}
    Ident ident;
    ExpressionPtr for_expr;
    ExpressionPtr to_expr;
    std::optional<ExpressionPtr> by_expr;
    StatementSequence block;
};

struct CallStatement : Statement {
    std::string to_string() const override;
    Error check(const SymbolTable&) const override;
    CallStatement(ProcCall c) : call(c) {}
    ProcCall call;
};

} // namespace nodes
