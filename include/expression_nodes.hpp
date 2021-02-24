#pragma once

#include "expression.hpp"
#include "type.hpp"
#include "statement.hpp"

namespace nodes {

struct Number : Expression {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        return std::visit([](auto&& arg) { return fmt::format("{}", arg); }, value);
    }
    Number(std::variant<Real, Integer> v) : value(v) {}
    std::variant<Real, Integer> value;
};

struct Char : Expression {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        if (value < ' ' || value > '~')
            return fmt::format("{}X", (int)value);
        else
            return fmt::format("'{}'", value);
    }
    Char(char c) : value(c) {}
    char value;
};

struct String : Expression {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("\"{}\"", fmt::join(value, "")); }
    String(std::vector<char> v) : value(v) {}
    std::vector<char> value;
};

struct Nil : Expression {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return "NIL"; }
    Nil() {}
};

struct Boolean : Expression {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("{}", value); }
    Boolean(bool v) : value(v) {}
    bool value;
};

struct SetElement {
    ExpressionPtr first;
    std::optional<ExpressionPtr> second;
};

struct Set : Expression {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("{{{}}}", fmt::join(value, ", ")); }
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
};

struct ProcCall : Expression, Statement {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        if (params)
            return fmt::format("{}({})", ident, fmt::join(*params, ", "));
        else
            return fmt::format("{}", ident);
    }
    ProcCall(Designator i, std::optional<ExpList> e) : ident(i), params(e) {}
    Designator ident;
    std::optional<ExpList> params;
};

struct Tilda : Expression {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("~{}", expression); }
    Tilda(ExpressionPtr ptr) : expression(ptr) {}
    ExpressionPtr expression;
};

struct Operator {
    Operator() {}
    Operator(std::vector<char> v) : value(v) {}
    Operator(char v) : value({v}) {}
    std::vector<char> value;
};

struct Term : Expression {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        std::string res = "";
        if (sign)
            res += *sign;
        if (oper)
            return res + fmt::format("({} {} {})", first, oper->value, *second);
        else
            return res + fmt::format("{}", first);
    }
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
