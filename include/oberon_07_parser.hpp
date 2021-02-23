#pragma once

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <variant>
#include <vector>

#include "parser.hpp"

namespace oberon07parser {

std::vector<char> str_to_vec(std::string_view str);

const std::vector<std::vector<char>> keywords = []() {
    std::vector<std::string_view> vec = {
        "ARRAY",  "FOR",   "PROCEDURE", "BEGIN", "IF",    "RECORD", "BY", "IMPORT", "REPEAT", "CASE",    "IN",
        "RETURN", "CONST", "IS",        "THEN",  "DIV",   "MOD",    "TO", "DO",     "MODULE", "TRUE",    "ELSE",
        "NIL",    "TYPE",  "ELSIF",     "OF",    "UNTIL", "END",    "OR", "VAR",    "FALSE",  "POINTER", "WHILE"};
    std::vector<std::vector<char>> result{};
    std::transform(vec.begin(), vec.end(), std::back_inserter(result), [](auto s) { return str_to_vec(s); });
    return result;
}();

using Real = double;
using Integer = int;
using Ident = std::vector<char>;

template <class T>
using OPtr = std::shared_ptr<T>;

template <class Out, class T>
OPtr<Out> make_optr(T&& val) {
    return std::static_pointer_cast<Out>(std::make_shared(std::forward<T>(val)));
}

struct Node {
    CodePlace place;
    virtual const std::type_info& type_info() const = 0;
    virtual std::string to_string() const = 0;
    virtual ~Node() {}
};

struct Type : Node {};

using TypePtr = OPtr<Type>;

struct Expression : Node {};

using ExpressionPtr = OPtr<Expression>;

struct Statement : Node {};

using StatementPtr = OPtr<Statement>;

struct CodeBlock : Node {};

using CodeBlockPtr = OPtr<CodeBlock>;

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

struct QualIdent {
    std::string to_string() const {
        if (qual)
            return fmt::format("{}.{}", *qual, ident);
        else
            return fmt::format("{}", ident);
    }
    std::optional<Ident> qual;
    Ident ident;
};

struct IdentDef {
    Ident ident;
    bool def;
};

using IdentList = std::vector<IdentDef>;

struct TypeName : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return ident.to_string(); }
    TypeName(QualIdent i) : ident(i) {}
    QualIdent ident;
};

struct FieldList {
    IdentList list;
    TypePtr type;
};

using FieldListSequence = std::vector<FieldList>;

struct RecordType : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        if (basetype)
            return fmt::format("RECORD ({}) {} END", *basetype, seq);
        else
            return fmt::format("RECORD {} END", seq);
    }
    RecordType(std::optional<QualIdent> b, FieldListSequence s) : basetype(b), seq(s) {}
    std::optional<QualIdent> basetype;
    FieldListSequence seq;
};

struct PointerType : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("POINTER TO {}", type); }
    PointerType(TypePtr t) : type(t) {}
    TypePtr type;
};

struct ArrayType : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("ARRAY {} OF {}", fmt::join(lengths, ", "), type); }
    ArrayType(std::vector<ExpressionPtr> l, TypePtr t) : lengths(l), type(t) {}
    std::vector<ExpressionPtr> lengths;
    TypePtr type;
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

struct ConstDecl {
    IdentDef ident;
    ExpressionPtr expression;
};

struct TypeDecl {
    IdentDef ident;
    TypePtr type;
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

struct Assignment : Statement {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("{} := {}", variable, value); }
    Assignment(Designator var, ExpressionPtr val) : variable(var), value(val) {}
    Designator variable;
    ExpressionPtr value;
};

using StatementSequence = std::vector<StatementPtr>;

using IfBlock = std::tuple<ExpressionPtr, StatementSequence>;

struct IfStatement : Statement {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        auto [cond, block] = if_blocks[0];
        std::vector<IfBlock> elsif_blocks = {if_blocks.begin() + 1, if_blocks.end()};
        if (else_block)
            return fmt::format("IF {} THEN\n{}\n{}ELSE {}\nEND", cond, block, fmt::join(elsif_blocks, "\n"),
                               *else_block);
        else
            return fmt::format("IF {} THEN\n{}\n{}END", cond, block, fmt::join(elsif_blocks, "\n"));
    }
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
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("CASE {} OF {} END", expression, fmt::join(cases, " |\n")); }
    CaseStatement(ExpressionPtr e, std::vector<Case> c) : expression(e), cases(c) {}
    ExpressionPtr expression;
    std::vector<Case> cases;
};

struct WhileStatement : Statement {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        auto [cond, block] = if_blocks[0];
        std::vector<IfBlock> elsif_blocks = {if_blocks.begin() + 1, if_blocks.end()};
        return fmt::format("WHILE {} DO\n{}\n{}END", cond, block, fmt::join(elsif_blocks, "\n"));
    }
    WhileStatement(std::vector<IfBlock> ib) : if_blocks(ib) {}
    std::vector<IfBlock> if_blocks;
};

struct RepeatStatement : Statement {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        auto [cond, block] = if_block;
        return fmt::format("REPEAT {} UNTIL\n{}", block, cond);
    }
    RepeatStatement(StatementSequence s, ExpressionPtr e) : if_block({e, s}) {}
    IfBlock if_block;
};

struct ForStatement : Statement {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        if (by_expr)
            return fmt::format("FOR {} := {} TO {} BY {} DO\n{}\nEND", ident, for_expr, to_expr, *by_expr, block);
        else
            return fmt::format("FOR {} := {} TO {} DO\n{}\nEND", ident, for_expr, to_expr, block);
    }
    ForStatement(Ident i, ExpressionPtr f, ExpressionPtr to, std::optional<ExpressionPtr> by, StatementSequence b)
        : ident(i), for_expr(f), to_expr(to), by_expr(by), block(b) {}
    Ident ident;
    ExpressionPtr for_expr;
    ExpressionPtr to_expr;
    std::optional<ExpressionPtr> by_expr;
    StatementSequence block;
};

using VariableDecl = FieldList;

struct DeclarationSequence {
    std::vector<ConstDecl> constDecls;
    std::vector<TypeDecl> typeDecls;
    std::vector<VariableDecl> variableDecls;
    std::vector<CodeBlockPtr> procedureDecls;
};

struct FormalType {
    bool array;
    QualIdent ident;
};

struct FPSection {
    std::optional<Ident> var;
    std::vector<Ident> idents;
    FormalType type;
};

struct FormalParameters {
    std::vector<FPSection> sections;
    std::optional<QualIdent> rettype;
};

struct ProcedureType : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("PROCEDURE {}", params); }
    ProcedureType() {}
    ProcedureType(std::optional<FormalParameters> par) {
        if (par)
            params = *par;
    }
    FormalParameters params;
};

struct ProcedureDeclaration : CodeBlock {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        std::string statements;
        if (!body.empty())
            statements = fmt::format("BEGIN\n{}\n", body);
        std::string ret_str;
        if (ret)
            ret_str = fmt::format("RETURN {}\n", *ret);
        return fmt::format("PROCEDURE {} {};\n{}{}{}END {}", name, type.params, decls, statements, ret_str, name);
    }
    ProcedureDeclaration() {}
    ProcedureDeclaration(IdentDef n, ProcedureType t, DeclarationSequence d, StatementSequence b,
                         std::optional<ExpressionPtr> r)
        : name(n), type(t), decls(d), body(b), ret(r) {}
    IdentDef name;
    ProcedureType type;
    DeclarationSequence decls;
    StatementSequence body;
    std::optional<ExpressionPtr> ret;
};

struct Import {
    Import() {}
    Import(Ident first, std::optional<Ident> second) : name(first) {
        if (second) {
            real_name = *second;
        } else {
            real_name = name;
        }
    }
    Ident name;
    Ident real_name;
};

using ImportList = std::vector<Import>;

struct Module : CodeBlock {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        return fmt::format("MODULE {}; IMPORT {}\n{}\nBEGIN\n{}\nEND {}.", name, fmt::join(imports, ", "), declarations,
                           body, name);
    }
    Module() {}
    Module(Ident n, ImportList i, DeclarationSequence d, StatementSequence b)
        : name(n), imports(i), declarations(d), body(b) {}
    Ident name;
    ImportList imports;
    DeclarationSequence declarations;
    StatementSequence body;
};

ParserPtr<Module> get_parser();

} // namespace oberon07parser

template <>
struct fmt::formatter<std::vector<char>> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(std::vector<char> const& vec, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", fmt::join(vec, ""));
    }
};

template <>
struct fmt::formatter<oberon07parser::TypePtr> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::TypePtr const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", id->to_string());
    }
};

template <>
struct fmt::formatter<oberon07parser::ExpressionPtr> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::ExpressionPtr const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", id->to_string());
    }
};

template <>
struct fmt::formatter<oberon07parser::StatementPtr> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::StatementPtr const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", id->to_string());
    }
};

template <>
struct fmt::formatter<oberon07parser::CodeBlockPtr> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::CodeBlockPtr const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", id->to_string());
    }
};

template <>
struct fmt::formatter<oberon07parser::SetElement> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::SetElement const& id, FormatContext& ctx) {
        if (id.second)
            return fmt::format_to(ctx.out(), "{}..{}", id.first, *id.second);
        else
            return fmt::format_to(ctx.out(), "{}", id.first);
    }
};

template <>
struct fmt::formatter<oberon07parser::QualIdent> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::QualIdent const& id, FormatContext& ctx) {
        if (id.qual)
            return fmt::format_to(ctx.out(), "{}.{}", *id.qual, id.ident);
        else
            return fmt::format_to(ctx.out(), "{}", id.ident);
    }
};

template <>
struct fmt::formatter<oberon07parser::IdentDef> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::IdentDef const& id, FormatContext& ctx) {
        if (id.def)
            return fmt::format_to(ctx.out(), "{}*", id.ident);
        else
            return fmt::format_to(ctx.out(), "{}", id.ident);
    }
};

template <>
struct fmt::formatter<oberon07parser::IdentList> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::IdentList const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", fmt::join(id, ", "));
    }
};

template <>
struct fmt::formatter<oberon07parser::FieldList> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::FieldList const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{} : {}", id.list, id.type);
    }
};

template <>
struct fmt::formatter<oberon07parser::FieldListSequence> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::FieldListSequence const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", fmt::join(id, "; "));
    }
};

template <>
struct fmt::formatter<oberon07parser::Selector> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::Selector const& id, FormatContext& ctx) {
        if (auto sel = std::get_if<oberon07parser::Ident>(&id); sel) {
            return fmt::format_to(ctx.out(), ".{}", *sel);
        } else if (auto sel = std::get_if<oberon07parser::ExpList>(&id); sel) {
            return fmt::format_to(ctx.out(), "[{}]", fmt::join(*sel, ", "));
        } else if (auto sel = std::get_if<char>(&id); sel) {
            return fmt::format_to(ctx.out(), "{}", *sel);
        } else if (auto sel = std::get_if<oberon07parser::QualIdent>(&id); sel) {
            return fmt::format_to(ctx.out(), "({})", *sel);
        } else
            throw std::runtime_error("Bad variant");
    }
};

template <>
struct fmt::formatter<oberon07parser::Designator> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::Designator const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}{}", id.ident, fmt::join(id.selector, ""));
    }
};

template <>
struct fmt::formatter<oberon07parser::ConstDecl> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::ConstDecl const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{} = {}", id.ident, id.expression);
    }
};

template <>
struct fmt::formatter<oberon07parser::TypeDecl> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::TypeDecl const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{} = {}", id.ident, id.type);
    }
};

template <>
struct fmt::formatter<oberon07parser::StatementSequence> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::StatementSequence const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", fmt::join(id, ";\n"));
    }
};

template <>
struct fmt::formatter<oberon07parser::IfBlock> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::IfBlock const& id, FormatContext& ctx) {
        auto [cond, block] = id;
        return fmt::format_to(ctx.out(), "ELSIF {} THEN\n{}\n", cond, block);
    }
};

template <>
struct fmt::formatter<oberon07parser::CaseLabel> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::CaseLabel const& id, FormatContext& ctx) {
        if (id.second)
            return fmt::format_to(ctx.out(), "{}..{}", id.first, *id.second);
        return fmt::format_to(ctx.out(), "{}", id.first);
    }
};

template <>
struct fmt::formatter<oberon07parser::CaseLabelList> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::CaseLabelList const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", fmt::join(id, ", "));
    }
};

template <>
struct fmt::formatter<oberon07parser::Case> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::Case const& id, FormatContext& ctx) {
        auto [label, state] = id;
        return fmt::format_to(ctx.out(), "{} : {}", label, state);
    }
};

template <>
struct fmt::formatter<oberon07parser::DeclarationSequence> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::DeclarationSequence const& id, FormatContext& ctx) {
        std::string constDecl, typeDecl, variableDecl, procDecl;
        if (!id.constDecls.empty())
            constDecl = fmt::format("CONST {};\n", fmt::join(id.constDecls, ";\n"));
        if (!id.typeDecls.empty())
            typeDecl = fmt::format("TYPE {};\n", fmt::join(id.typeDecls, ";\n"));
        if (!id.variableDecls.empty())
            variableDecl = fmt::format("VAR {};\n", fmt::join(id.variableDecls, ";\n"));
        if (!id.procedureDecls.empty())
            procDecl = fmt::format("{};\n", fmt::join(id.procedureDecls, ";\n"));
        return fmt::format_to(ctx.out(), "{}{}{}{}", constDecl, typeDecl, variableDecl, procDecl);
    }
};

template <>
struct fmt::formatter<oberon07parser::FormalType> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::FormalType const& id, FormatContext& ctx) {
        if (id.array)
            return fmt::format_to(ctx.out(), "ARRAY OF {}", id.ident);
        else
            return fmt::format_to(ctx.out(), "{}", id.ident);
    }
};

template <>
struct fmt::formatter<oberon07parser::FPSection> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::FPSection const& id, FormatContext& ctx) {
        if (id.var)
            return fmt::format_to(ctx.out(), "{} {} : {}", *id.var, fmt::join(id.idents, ", "), id.type);
        else
            return fmt::format_to(ctx.out(), "{} : {}", fmt::join(id.idents, ", "), id.type);
    }
};

template <>
struct fmt::formatter<oberon07parser::FormalParameters> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::FormalParameters const& id, FormatContext& ctx) {
        if (id.rettype)
            return fmt::format_to(ctx.out(), "({}) : {}", fmt::join(id.sections, "; "), *id.rettype);
        else
            return fmt::format_to(ctx.out(), "({})", fmt::join(id.sections, "; "));
    }
};

template <>
struct fmt::formatter<oberon07parser::Import> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(oberon07parser::Import const& id, FormatContext& ctx) {
        if (id.real_name == id.name)
            return fmt::format_to(ctx.out(), "{}", id.name);
        else
            return fmt::format_to(ctx.out(), "{} := {}", id.name, id.real_name);
    }
};
