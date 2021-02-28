#pragma once

#include "semantic_error.hpp"
#include "expression.hpp"
#include "type_nodes.hpp"
#include "statement.hpp"
#include <variant>

struct SymbolToken;

namespace nodes {

struct Number : Expression {
    std::string to_string() const {
        return std::visit([](auto&& arg) { return fmt::format("{}", arg); }, value);
    }
    TypeResult get_type(const SymbolTable& table) const override;
    ExprResult eval(const SymbolTable&) const override;
    Number(std::variant<Real, Integer> v) : value(v) {}
    std::variant<Real, Integer> value;
};

struct Char : Expression {
    std::string to_string() const {
        if (value < ' ' || value > '~')
            return fmt::format("{}X", (int)value);
        else
            return fmt::format("'{}'", value);
    }
    TypeResult get_type(const SymbolTable& table) const override;
    ExprResult eval(const SymbolTable&) const override;
    Char(char c) : value(c) {}
    char value;
};

struct String : Expression {
    std::string to_string() const { return fmt::format("\"{}\"", fmt::join(value, "")); }
    TypeResult get_type(const SymbolTable& table) const override;
    ExprResult eval(const SymbolTable&) const override;
    String(std::vector<char> v) : value(v) {}
    std::vector<char> value;
};

struct Nil : Expression {
    std::string to_string() const { return "NIL"; }
    TypeResult get_type(const SymbolTable& table) const override;
    ExprResult eval(const SymbolTable&) const override;
    Nil() {}
};

struct Boolean : Expression {
    std::string to_string() const { return fmt::format("{}", value); }
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
    std::string to_string() const { return fmt::format("{{{}}}", fmt::join(value, ", ")); }
    TypeResult get_type(const SymbolTable& table) const override;
    ExprResult eval(const SymbolTable&) const override;
    Set(std::optional<std::vector<SetElement>> v) {
        if (v)
            value = *v;
    }
    std::vector<SetElement> value;
};

using ExpList = std::vector<ExpressionPtr>;

using Selector = std::variant<Ident, ExpList, char, QualIdent>;

struct Designator {
    QualIdent ident;
    std::vector<Selector> selector;
    SemResult<SymbolToken> get_symbol(const SymbolTable& table, CodePlace place) const;
};

struct ProcCall : Expression, Statement {
    std::string to_string() const {
        if (params)
            return fmt::format("{}({})", ident, fmt::join(*params, ", "));
        else
            return fmt::format("{}", ident);
    }
    SemResult<SymbolToken> get_info(const SymbolTable& table) const;
    TypeResult get_type(const SymbolTable& table) const override;
    ExprResult eval(const SymbolTable&) const override;
    ProcCall(Designator i, std::optional<ExpList> e) : ident(i), params(e) {}
    Designator ident;
    std::optional<ExpList> params;
};

struct Tilda : Expression {
    std::string to_string() const { return fmt::format("~{}", expression); }
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
    std::string to_string() const {
        std::string res = "";
        if (sign)
            res += *sign;
        if (oper)
            return res + fmt::format("({} {} {})", first, oper->value, *second);
        else
            return res + fmt::format("{}", first);
    }
    TypeResult get_type(const SymbolTable& table) const override;
    ExprResult eval(const SymbolTable&) const override;
    Term() {}
    Term(std::optional<char> s, ExpressionPtr f, std::optional<std::tuple<Operator, ExpressionPtr>> sec)
        : sign(s), first(f) {
        if (sec) {
            auto [op, se] = *sec;
            oper = op;
            second = se;
        }
    }
    Term(ExpressionPtr f, std::optional<std::tuple<Operator, ExpressionPtr>> sec) : first(f) {
        if (sec) {
            auto [op, se] = *sec;
            oper = op;
            second = se;
        }
    }
    std::optional<char> sign;
    ExpressionPtr first;
    std::optional<Operator> oper;
    std::optional<ExpressionPtr> second;
};

}
