#pragma once

#include "expression.hpp"
#include "parser_tools.hpp"
#include "statement.hpp"
#include <variant>
#include <bitset>

struct SymbolToken;

namespace nodes {

struct ConstInteger : Value {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context&) const override;
    Maybe<ValuePtr> eval_constant(Context&) const override;
    Maybe<ValuePtr> apply_operator(Context&, OpType, const Value&) const override;

    bool operator == (const ConstInteger& other) const { return value == other.value; }

    ConstInteger(Integer i) : value(i) {}
    Integer value;
};

struct ConstReal : Value {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context&) const override;
    Maybe<ValuePtr> eval_constant(Context&) const override;
    Maybe<ValuePtr> apply_operator(Context&, OpType, const Value&) const override;
    ConstReal(Real d) : value(d) {}
    Real value;
};

struct Char : Value {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context& table) const override;
    Maybe<ValuePtr> eval_constant(Context&) const override;
    Maybe<ValuePtr> apply_operator(Context&, OpType, const Value&) const override;
    Char(char c) : value(c) {}
    char value;
};

struct String : Value {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context& table) const override;
    Maybe<ValuePtr> eval_constant(Context&) const override;
    Maybe<ValuePtr> apply_operator(Context&, OpType, const Value&) const override;

    bool operator == (const String& other) const  { return value == other.value; }

    String(std::vector<char> v) : value(v) {}
    String(char v) : value{v} {}
    std::vector<char> value;
};

struct Nil : Value {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context& table) const override;
    Maybe<ValuePtr> eval_constant(Context&) const override;
    Maybe<ValuePtr> apply_operator(Context&, OpType, const Value&) const override;
    Nil() {}
};

struct Boolean : Value {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context& table) const override;
    Maybe<ValuePtr> eval_constant(Context&) const override;
    Maybe<ValuePtr> apply_operator(Context&, OpType, const Value&) const override;
    Boolean(bool v) : value(v) {}
    bool value;
};

struct ConstSet : Value {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context& table) const override;
    Maybe<ValuePtr> eval_constant(Context&) const override;
    Maybe<ValuePtr> apply_operator(Context&, OpType, const Value&) const override;
    ConstSet(std::bitset<64> s) : values(s) {};

    using SetType = std::bitset<64>;
    SetType values;
};

enum class BPType {
    ABS, ODD, LEN, LSL, ASR, ROR,
    FLOOR, FLT, ORD, CHR,
    INC, DEC, INCL, EXCL,
    NEW, ASSERT, PACK, UNPK
};

struct BaseProcedure : Value {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context&) const override;
    Maybe<ValuePtr> eval_constant(Context&) const override;
    Maybe<ValuePtr> apply_operator(Context&, OpType, const Value&) const override;

    BaseProcedure(std::string_view, ExpList);

    BPType name;
    ExpList params;
};

struct SetElement {
    ExpressionPtr first;
    std::optional<ExpressionPtr> second;
};

struct Set : Expression {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context& table) const override;
    Maybe<ValuePtr> eval_constant(Context&) const override;
    Set(std::optional<std::vector<SetElement>> v);
    std::vector<SetElement> value;
};

struct ProcCallData {
    std::optional<std::vector<QualIdent>> commonParams;
    DesignatorRepairer ident;
    std::optional<ExpList> params;
};

bool proccall_repair(ProcCallData&, Context&);

using ProcCallDataRepairer = Repairer<ProcCallData, Context, proccall_repair>;

struct ProcCall : Expression {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context& table) const override;
    Maybe<ValuePtr> eval_constant(Context&) const override;
    ProcCall(std::optional<std::vector<QualIdent>> c, DesignatorRepairer i, std::optional<ExpList> e) : data(c, i, e) {}
    ProcCallDataRepairer data;
};

struct Tilda : Expression {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context& table) const override;
    Maybe<ValuePtr> eval_constant(Context&) const override;
    Tilda(ExpressionPtr ptr) : expression(ptr) {}
    ExpressionPtr expression;
};

struct Term : Expression {
    std::string to_string() const override;
    Maybe<std::pair<SymbolGroup, TypePtr>> get_type(Context& table) const override;
    Maybe<ValuePtr> eval_constant(Context&) const override;
    Term() {}
    Term(std::optional<char> s, ExpressionPtr f, std::optional<std::tuple<Operator, ExpressionPtr>> sec);
    Term(ExpressionPtr f, std::optional<std::tuple<Operator, ExpressionPtr>> sec);
    std::optional<char> sign;
    ExpressionPtr first;
    std::optional<Operator> oper;
    std::optional<ExpressionPtr> second;
};

} // namespace nodes
