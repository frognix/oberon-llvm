#pragma once

#include "node.hpp"
#include "repairer.hpp"
#include "type.hpp"
#include "symbol_token.hpp"
#include <variant>

struct SymbolToken;

namespace nodes {

struct Expression : Node {
    virtual Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context&) const = 0;
    virtual Maybe<ExpressionPtr> eval_constant(Context&) const = 0;
    virtual ~Expression() = default;
};

enum class OpType {
    ADD, SUB, MUL, RDIV, IDIV, MOD, OR,
    AND, EQ, NEQ, LT, LTE, GT, GTE, IN, IS
};

struct Operator {
    Operator() {}
    Operator(Ident v);
    Operator(char v);
    OpType value;
};

Maybe<BaseType> expression_compatible(Context& context, CodePlace place, const Type& left, OpType oper, const Type& right);

template <class Subtype, class... Args>
ExpressionPtr make_expression(Args... args) {
    return make_optr<Expression, Subtype>(args...);
}

using ExpList = std::vector<ExpressionPtr>;

using Selector = std::variant<Ident, ExpList, char, QualIdent>;

struct Designator {
    Designator() {}
    Designator(QualIdent i, std::vector<Selector> s) : ident(i), selector(s) {}
    std::string to_string() const { return fmt::format("{}{}", ident, fmt::join(selector, "")); }
    bool is_simple() const { return selector.empty() && !ident.qual; }
    Maybe<SymbolToken> get_symbol(Context&, CodePlace) const;
    QualIdent ident;
    std::vector<Selector> selector;
};

bool designator_repair(Designator& value, Context& table);

using DesignatorRepairer = Repairer<Designator, Context, designator_repair>;

} // namespace nodes
