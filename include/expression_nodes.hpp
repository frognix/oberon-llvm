#pragma once

#include "expression.hpp"
#include "parser_tools.hpp"
#include "statement.hpp"
#include "type_nodes.hpp"
#include <variant>

struct SymbolToken;

namespace nodes {

struct ConstInteger : Expression {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context&) const override;
    Maybe<ExpressionPtr> eval_constant(Context&) const override;
    ConstInteger(Integer i) : value(i) {}
    Integer value;
};

struct ConstReal : Expression {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context&) const override;
    Maybe<ExpressionPtr> eval_constant(Context&) const override;
    ConstReal(Real d) : value(d) {}
    Real value;
};

struct Char : Expression {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context& table) const override;
    Maybe<ExpressionPtr> eval_constant(Context&) const override;
    Char(char c) : value(c) {}
    char value;
};

struct String : Expression {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context& table) const override;
    Maybe<ExpressionPtr> eval_constant(Context&) const override;
    String(std::vector<char> v) : value(v) {}
    std::vector<char> value;
};

struct Nil : Expression {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context& table) const override;
    Maybe<ExpressionPtr> eval_constant(Context&) const override;
    Nil() {}
};

struct Boolean : Expression {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context& table) const override;
    Maybe<ExpressionPtr> eval_constant(Context&) const override;
    Boolean(bool v) : value(v) {}
    bool value;
};

struct SetElement {
    ExpressionPtr first;
    std::optional<ExpressionPtr> second;
};

struct Set : Expression {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context& table) const override;
    Maybe<ExpressionPtr> eval_constant(Context&) const override;
    Set(std::optional<std::vector<SetElement>> v);
    std::vector<SetElement> value;
};

struct ProcCallData {
    DesignatorRepairer ident;
    std::optional<ExpList> params;
};

bool proccall_repair(ProcCallData&, Context&);

using ProcCallDataRepairer = Repairer<ProcCallData, Context, proccall_repair>;

struct ProcCall : Expression {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_info(Context& table) const;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context& table) const override;
    Maybe<ExpressionPtr> eval_constant(Context&) const override;
    ProcCall(DesignatorRepairer i, std::optional<ExpList> e) : data(i, e) {}
    ProcCallDataRepairer data;
};

struct Tilda : Expression {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context& table) const override;
    Maybe<ExpressionPtr> eval_constant(Context&) const override;
    Tilda(ExpressionPtr ptr) : expression(ptr) {}
    ExpressionPtr expression;
};

struct Term : Expression {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context& table) const override;
    Maybe<ExpressionPtr> eval_constant(Context&) const override;
    Term() {}
    Term(std::optional<char> s, ExpressionPtr f, std::optional<std::tuple<Operator, ExpressionPtr>> sec);
    Term(ExpressionPtr f, std::optional<std::tuple<Operator, ExpressionPtr>> sec);
    std::optional<char> sign;
    ExpressionPtr first;
    std::optional<Operator> oper;
    std::optional<ExpressionPtr> second;
};

} // namespace nodes
