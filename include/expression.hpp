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
    virtual Maybe<ValuePtr> eval_constant(Context&) const = 0;
    virtual ~Expression() = default;
};

enum OpType : std::uint16_t {
    OP_ADD  = 1 << 0,
    OP_SUB  = 1 << 1,
    OP_MUL  = 1 << 2,
    OP_RDIV = 1 << 3,
    OP_IDIV = 1 << 4,
    OP_MOD  = 1 << 5,
    OP_OR   = 1 << 6,
    OP_AND  = 1 << 7,
    OP_EQ   = 1 << 8,
    OP_NEQ  = 1 << 9,
    OP_LT   = 1 << 10,
    OP_LTE  = 1 << 11,
    OP_GT   = 1 << 12,
    OP_GTE  = 1 << 13,
    OP_IN   = 1 << 14,
    OP_IS   = 1 << 15,
};

const char* optype_to_str(OpType type);

inline constexpr OpType operator | (OpType left, OpType right) {
    return static_cast<OpType>(static_cast<std::uint16_t>(left) | static_cast<std::uint16_t>(right));
}

inline constexpr OpType operator & (OpType left, OpType right) {
    return static_cast<OpType>(static_cast<std::uint16_t>(left) & static_cast<std::uint16_t>(right));
}

constexpr OpType OP_COMPARE = OP_EQ | OP_NEQ | OP_LT | OP_LTE | OP_GT | OP_GTE;

struct Value : Expression {
    virtual Maybe<ValuePtr> apply_operator(Context&, OpType, const Value&) const = 0;
};

struct Operator {
    Operator() {}
    Operator(Ident v);
    Operator(std::string_view);
    OpType value;
};

Maybe<BaseType> expression_compatible(Context& context, CodePlace place, const Type& left, OpType oper, const Type& right);

template <class Subtype, class... Args>
ExpressionPtr make_expression(Args... args) {
    return make_optr<Expression, Subtype>(args...);
}

template <class Subtype, class... Args>
ValuePtr make_value(Args... args) {
    return make_optr<Value, Subtype>(args...);
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
