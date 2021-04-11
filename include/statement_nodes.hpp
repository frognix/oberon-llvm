#pragma once

#include "expression_nodes.hpp"
#include "statement.hpp"
#include "const_value.hpp"
#include <optional>

namespace nodes {

struct Assignment : Statement {
    std::string to_string() const override;
    virtual bool check(Context&) const override;
    Assignment(DesignatorRepairer var, ExpressionPtr val) : variable(var), value(val) {}
    DesignatorRepairer variable;
    ExpressionPtr value;
};

using IfBlock = std::tuple<ExpressionPtr, StatementSequence>;

struct IfStatement : Statement {
    std::string to_string() const override;
    bool check(Context&) const override;
    IfStatement(std::vector<IfBlock> ib, std::optional<StatementSequence> eb) : if_blocks(ib), else_block(eb) {}
    std::vector<IfBlock> if_blocks;
    std::optional<StatementSequence> else_block;
};

using Label = std::variant<Integer, StringValue, QualIdent>;

struct CaseLabel {
    Label first;
    std::optional<Label> second;
};

using CaseLabelList = std::vector<CaseLabel>;

using Case = std::tuple<CaseLabelList, StatementSequence>;

struct CaseStatement : Statement {
    std::string to_string() const override;
    bool check(Context&) const override;
    CaseStatement(ExpressionPtr e, std::vector<Case> c) : expression(e), cases(c) {}
    ExpressionPtr expression;
    std::vector<Case> cases;
};

struct WhileStatement : Statement {
    std::string to_string() const override;
    bool check(Context&) const override;
    WhileStatement(std::vector<IfBlock> ib) : if_blocks(ib) {}
    std::vector<IfBlock> if_blocks;
};

struct RepeatStatement : Statement {
    std::string to_string() const override;
    bool check(Context&) const override;
    RepeatStatement(StatementSequence s, ExpressionPtr e) : if_block({e, s}) {}
    IfBlock if_block;
};

struct ForStatement : Statement {
    std::string to_string() const override;
    bool check(Context&) const override;
    ForStatement(Ident i, ExpressionPtr f, ExpressionPtr to, std::optional<ExpressionPtr> by, StatementSequence b)
        : ident(i), for_expr(f), to_expr(to), by_expr(by ? by : std::nullopt), block(b) {}
    Ident ident;
    ExpressionPtr for_expr;
    ExpressionPtr to_expr;
    std::optional<ConstValue> by_expr;
    StatementSequence block;
};

struct CallStatement : Statement {
    std::string to_string() const override;
    bool check(Context&) const override;
    CallStatement(ExpressionPtr c) : call(c) {}
    ExpressionPtr call;
};

} // namespace nodes
