#pragma once

#include "expression.hpp"
#include "semantic_error.hpp"
#include "statement.hpp"
#include "type_nodes.hpp"
#include <variant>

struct SymbolToken;

namespace nodes {

struct Number : Expression {
    std::string to_string() const override;
    TypeResult get_type(const SymbolTable& table) const override;
    ExprResult eval(const SymbolTable&) const override;
    Number(std::variant<Real, Integer> v) : value(v) {}
    std::variant<Real, Integer> value;
};

struct Char : Expression {
    std::string to_string() const override;
    TypeResult get_type(const SymbolTable& table) const override;
    ExprResult eval(const SymbolTable&) const override;
    Char(char c) : value(c) {}
    char value;
};

struct String : Expression {
    std::string to_string() const override;
    TypeResult get_type(const SymbolTable& table) const override;
    ExprResult eval(const SymbolTable&) const override;
    String(std::vector<char> v) : value(v) {}
    std::vector<char> value;
};

struct Nil : Expression {
    std::string to_string() const override;
    TypeResult get_type(const SymbolTable& table) const override;
    ExprResult eval(const SymbolTable&) const override;
    Nil() {}
};

struct Boolean : Expression {
    std::string to_string() const override;
    TypeResult get_type(const SymbolTable& table) const override;
    ExprResult eval(const SymbolTable&) const override;
    Boolean(bool v) : value(v) {}
    bool value;
};

struct SetElement {
    ExpressionPtr first;
    std::optional<ExpressionPtr> second;
};

struct Set : Expression {
    std::string to_string() const override;
    TypeResult get_type(const SymbolTable& table) const override;
    ExprResult eval(const SymbolTable&) const override;
    Set(std::optional<std::vector<SetElement>> v);
    std::vector<SetElement> value;
};

struct ProcCallData {
    DesignatorRepairer ident;
    std::optional<ExpList> params;
};

std::optional<SemanticError> proccall_repair(ProcCallData&, const SymbolTable&);

using ProcCallDataRepairer = Repairer<ProcCallData, SymbolTable, SemanticError, proccall_repair>;

struct ProcCall : Expression {
    std::string to_string() const override;
    SemResult<SymbolToken> get_info(const SymbolTable& table) const;
    TypeResult get_type(const SymbolTable& table) const override;
    ExprResult eval(const SymbolTable&) const override;
    ProcCall(DesignatorRepairer i, std::optional<ExpList> e) : data(i, e) {}
    ProcCallDataRepairer data;
};

struct Tilda : Expression {
    std::string to_string() const override;
    TypeResult get_type(const SymbolTable& table) const override;
    ExprResult eval(const SymbolTable&) const override;
    Tilda(ExpressionPtr ptr) : expression(ptr) {}
    ExpressionPtr expression;
};

struct Operator {
    Operator() {}
    Operator(Ident v) : value(v) {}
    Operator(char v) : value({v}) {}
    Ident value;
};

struct Term : Expression {
    std::string to_string() const override;
    TypeResult get_type(const SymbolTable& table) const override;
    ExprResult eval(const SymbolTable&) const override;
    Term() {}
    Term(std::optional<char> s, ExpressionPtr f, std::optional<std::tuple<Operator, ExpressionPtr>> sec);
    Term(ExpressionPtr f, std::optional<std::tuple<Operator, ExpressionPtr>> sec);
    std::optional<char> sign;
    ExpressionPtr first;
    std::optional<Operator> oper;
    std::optional<ExpressionPtr> second;
};

} // namespace nodes
