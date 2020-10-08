#include <fmt/format.h>
#include <fmt/ranges.h>

#include <type_traits>

#include <tuple>
#include <fstream>

#include <cctype>

#include "parsers.hpp"

std::vector<char> str_to_vec(std::string_view str) {
    std::vector<char> vec;
    vec.insert(vec.end(), str.begin(), str.end());
    return vec;
}

const std::vector<std::vector<char>> keywords = [](){
    std::vector<std::string_view> vec = {
        "ARRAY", "FOR",     "PROCEDURE",
        "BEGIN", "IF",      "RECORD",
        "BY",    "IMPORT",  "REPEAT",
        "CASE",  "IN",      "RETURN",
        "CONST", "IS",      "THEN",
        "DIV",   "MOD",     "TO",
        "DO",    "MODULE",  "TRUE",
        "ELSE",  "NIL"      "TYPE",
        "ELSIF", "OF"       "UNTIL",
        "END",   "OR"       "VAR",
        "FALSE", "POINTER", "WHILE"
    };
    std::vector<std::vector<char>> result{};
    std::transform(vec.begin(), vec.end(), std::back_inserter(result), [](auto s){ return str_to_vec(s); });
    return result;
 }();

template<>
struct fmt::formatter<std::vector<char>> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
      return ctx.begin();
  }

  template<typename FormatContext>
  auto format(std::vector<char> const& vec, FormatContext& ctx) {
      return fmt::format_to(ctx.out(), "{}", fmt::join(vec, ""));
  }
};

ParserPtr<std::vector<char>> delim() {
    return many(either({symbol(' '), symbol('\n')}));
}

template <class T, class D>
ParserPtr<std::vector<T>> extra_delim(ParserPtr<T> parser, ParserPtr<D> extra) {
    return chain(parser, many(parse_index<3>::select(delim(), extra, delim(), parser)));
}

template <class T, class D>
ParserPtr<std::vector<T>> extra_delim0(ParserPtr<T> parser, ParserPtr<D> extra) {
    return many(parse_index<1>::select(delim(), parser, delim(), extra));
}

template <class... Types>
ParserPtr<std::tuple<Types...>> syntax_sequence(ParserPtr<Types>... parsers) {
    return delim_sequence(delim(), parsers...);
}

template <size_t... Is>
struct syntax_index {
    template < class... Types>
    static auto select(ParserPtr<Types>... parsers) {
        return delim_index<Is...>::select(delim(), parsers...);
    }
};

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
    virtual const std::type_info& type_info() const = 0;
    virtual std::string to_string() const = 0;
    virtual ~Node() {}
};

struct Type : Node {};

using TypePtr = OPtr<Type>;

template<>
struct fmt::formatter<TypePtr> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(TypePtr const& id, FormatContext& ctx) {
      return fmt::format_to(ctx.out(), "{}", id->to_string());
  }
};

struct Expression : Node {};

using ExpressionPtr = OPtr<Expression>;

template<>
struct fmt::formatter<ExpressionPtr> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(ExpressionPtr const& id, FormatContext& ctx) {
      return fmt::format_to(ctx.out(), "{}", id->to_string());
  }
};

struct Statement : Node {};

using StatementPtr = OPtr<Statement>;

template<>
struct fmt::formatter<StatementPtr> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(StatementPtr const& id, FormatContext& ctx) {
      return fmt::format_to(ctx.out(), "{}", id->to_string());
  }
};

struct CodeBlock : Node {};

using CodeBlockPtr = OPtr<CodeBlock>;

template<>
struct fmt::formatter<CodeBlockPtr> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(CodeBlockPtr const& id, FormatContext& ctx) {
      return fmt::format_to(ctx.out(), "{}", id->to_string());
  }
};

struct Number : Expression {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return std::visit([](auto&& arg){return fmt::format("{}", arg); }, value); }
    Number(std::variant<Real, Integer> v) : value(v) {}
    std::variant<Real, Integer> value;
};

struct Char : Expression {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("'{}'", value); }
    Char(char c) : value(c) {}
    char value;
};

struct String : Expression {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("\"{}\"", fmt::join(value,"")); }
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

template<>
struct fmt::formatter<SetElement> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(SetElement const& id, FormatContext& ctx) {
      if (id.second) return fmt::format_to(ctx.out(), "{}..{}", id.first, *id.second);
      else return fmt::format_to(ctx.out(), "{}", id.first);
  }
};

struct Set : Expression {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        return fmt::format("{{{}}}", fmt::join(value, ", "));
    }
    Set(std::optional<std::vector<SetElement>> v) {
        if (v) value = *v;
    }
    std::vector<SetElement> value;
};

ParserPtr<char> inverse(char c) {
    return predicate(fmt::format("not {}", c), [c](auto n){ return n != c;});
}

template <class T>
class EqualTo : public Parser<T> {
public:
    EqualTo(ParserPtr<T> parser, T&& value) : m_parser(parser), m_value(std::forward<T>(value)) {}
    ParseResult<T> parse(CodeStream& stream) override {
        BreakPoint point(stream);
        if (auto res = m_parser->parse(stream); res) {
            if (res.get_ok() == m_value) {
                point.close();
                return res;
            } else return ParseError(fmt::format("{}", m_value), res.get_ok(), stream);
        } else return res;
    }
private:
    ParserPtr<T> m_parser;
    T m_value;
};

template <class T>
ParserPtr<T> equal_to(ParserPtr<T> parser, T value) {
    return make_parser(EqualTo(parser, std::move(value)));
}

Real parse_real(const std::tuple<size_t, size_t, std::optional<std::tuple<char, char, size_t>>>& res) {
    auto [ipart, rpart, scale_val] = res;
    Real real_part = rpart;
    while (real_part > 0) real_part /= 10;
    Real real = ipart + real_part;
    if (scale_val) {
        auto [l, sign, scale_power] = *scale_val;
        Real scale = (sign == '+' ? 1 : -1) * std::pow(10, scale_power);
        real *= scale;
    }
    return real;
}

struct QualIdent {
    std::string to_string() const {
        if (qual) return fmt::format("{}.{}", *qual, ident);
        else return fmt::format("{}", ident);
    }
    std::optional<Ident> qual;
    Ident ident;
};

template<>
struct fmt::formatter<QualIdent> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(QualIdent const& id, FormatContext& ctx) {
      if (id.qual) return fmt::format_to(ctx.out(), "{}.{}", *id.qual, id.ident);
      else return fmt::format_to(ctx.out(), "{}", id.ident);
  }
};

struct IdentDef {
    Ident ident;
    bool def;
};

template<>
struct fmt::formatter<IdentDef> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(IdentDef const& id, FormatContext& ctx) {
      if (id.def) return fmt::format_to(ctx.out(), "{}*", id.ident);
      else return fmt::format_to(ctx.out(), "{}", id.ident);
  }
};

using IdentList = std::vector<IdentDef>;

template<>
struct fmt::formatter<IdentList> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(IdentList const& id, FormatContext& ctx) {
      return fmt::format_to(ctx.out(), "{}", fmt::join(id, ", "));
  }
};

struct TypeName : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        return ident.to_string();
    }
    TypeName(QualIdent i) : ident(i) {}
    QualIdent ident;
};

struct FieldList {
    IdentList list;
    TypePtr type;
};

template<>
struct fmt::formatter<FieldList> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(FieldList const& id, FormatContext& ctx) {
      return fmt::format_to(ctx.out(), "{} : {}", id.list, id.type);
  }
};

using FieldListSequence = std::vector<FieldList>;

template<>
struct fmt::formatter<FieldListSequence> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(FieldListSequence const& id, FormatContext& ctx) {
      return fmt::format_to(ctx.out(), "{}", fmt::join(id, "; "));
  }
};

struct RecordType : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        if (basetype) return fmt::format("RECORD ({}) {} END", *basetype, seq);
        else return fmt::format("RECORD {} END", seq);
    }
    RecordType(std::optional<QualIdent> b, FieldListSequence s)
        : basetype(b), seq(s) {}
    std::optional<QualIdent> basetype;
    FieldListSequence seq;
};

struct PointerType : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        return fmt::format("POINTER TO {}", type);
    }
    PointerType(TypePtr t) : type(t) {}
    TypePtr type;
};

struct ArrayType : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        return fmt::format("ARRAY {} OF {}", fmt::join(lengths, ", "), type);
    }
    ArrayType(std::vector<ExpressionPtr> l, TypePtr t)
        : lengths(l), type(t) {}
    std::vector<ExpressionPtr> lengths;
    TypePtr type;
};

using ExpList = std::vector<ExpressionPtr>;

using Selector = std::variant<Ident, ExpList, char, QualIdent>;

template<>
struct fmt::formatter<Selector> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(Selector const& id, FormatContext& ctx) {
      if (auto sel = std::get_if<Ident>(&id); sel) {
          return fmt::format_to(ctx.out(), ".{}", *sel);
      } else if (auto sel = std::get_if<ExpList>(&id); sel) {
          return fmt::format_to(ctx.out(), "[{}]", fmt::join(*sel, ", "));
      } else if (auto sel = std::get_if<char>(&id); sel) {
          return fmt::format_to(ctx.out(), "{}", *sel);
      } else if (auto sel = std::get_if<QualIdent>(&id); sel) {
          return fmt::format_to(ctx.out(), "({})", *sel);
      } else throw std::runtime_error("Bad variant");
  }
};

struct Designator {
    QualIdent ident;
    std::vector<Selector> selector;
};

template<>
struct fmt::formatter<Designator> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(Designator const& id, FormatContext& ctx) {
      return fmt::format_to(ctx.out(), "{}{}", id.ident, fmt::join(id.selector, ""));
  }
};


template <class T>
ParserPtr<std::vector<T>> maybe_list(ParserPtr<std::vector<T>> parser) {
    return extension(maybe(parser), [](const auto& l) {
        if (l) return *l;
        else return std::vector<T>{};
    });
}

struct ProcCall : Expression, Statement {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        if (params) return fmt::format("{}({})", ident, fmt::join(*params, ", "));
        else return fmt::format("{}", ident);
    }
    ProcCall(Designator i, std::optional<ExpList> e) : ident(i), params(e) {}
    Designator ident;
    std::optional<ExpList> params;
};

struct Tilda : Expression {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        return fmt::format("~{}", expression);
    }
    Tilda(ExpressionPtr ptr) : expression(ptr) {}
    ExpressionPtr expression;
};

struct ConstDecl {
    IdentDef ident;
    ExpressionPtr expression;
};

template<>
struct fmt::formatter<ConstDecl> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(ConstDecl const& id, FormatContext& ctx) {
      return fmt::format_to(ctx.out(), "{} = {}", id.ident, id.expression);
  }
};

struct TypeDecl {
    IdentDef ident;
    TypePtr type;
};

template<>
struct fmt::formatter<TypeDecl> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(TypeDecl const& id, FormatContext& ctx) {
      return fmt::format_to(ctx.out(), "{} = {}", id.ident, id.type);
  }
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
        if (sign) res += *sign;
        if (oper) return res+fmt::format("({} {} {})", first, oper->value, *second);
        else return res+fmt::format("{}", first);
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
    Term(ExpressionPtr f, std::optional<std::tuple<Operator, ExpressionPtr>> sec)
        : first(f) {
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
    std::string to_string() const {
        return fmt::format("{} := {}", variable, value);
    }
    Assignment(Designator var, ExpressionPtr val) : variable(var), value(val) {}
    Designator variable;
    ExpressionPtr value;
};

using StatementSequence = std::vector<StatementPtr>;

template<>
struct fmt::formatter<StatementSequence> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(StatementSequence const& id, FormatContext& ctx) {
      return fmt::format_to(ctx.out(), "{}", fmt::join(id, ";\n"));
  }
};

using IfBlock = std::tuple<ExpressionPtr, StatementSequence>;

template<>
struct fmt::formatter<IfBlock> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(IfBlock const& id, FormatContext& ctx) {
      auto [cond, block] = id;
      return fmt::format_to(ctx.out(), "ELSIF {} THEN {}", cond, block);
  }
};

struct IfStatement : Statement {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        auto [cond, block] = if_blocks[0];
        std::vector<IfBlock> elsif_blocks = {if_blocks.begin()+1, if_blocks.end()};
        if (else_block)
            return fmt::format("IF {} THEN\n{}\n{}\nELSE {}\n END", cond, block, fmt::join(elsif_blocks, "\n"), *else_block);
        else
            return fmt::format("IF {} THEN\n{}\n{}\nEND", cond, block, fmt::join(elsif_blocks, "\n"));
    }
    IfStatement(std::vector<IfBlock> ib, std::optional<StatementSequence> eb) : if_blocks(ib), else_block(eb) {}
    std::vector<IfBlock> if_blocks;
    std::optional<StatementSequence> else_block;
};

struct CaseLabel {
    ExpressionPtr first;
    std::optional<ExpressionPtr> second;
};

template<>
struct fmt::formatter<CaseLabel> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(CaseLabel const& id, FormatContext& ctx) {
      if (id.second) return fmt::format_to(ctx.out(), "{}..{}", id.first, *id.second);
      return fmt::format_to(ctx.out(), "{}", id.first);
  }
};

using CaseLabelList = std::vector<CaseLabel>;

template<>
struct fmt::formatter<CaseLabelList> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(CaseLabelList const& id, FormatContext& ctx) {
      return fmt::format_to(ctx.out(), "{}", fmt::join(id, ", "));
  }
};

using Case = std::tuple<CaseLabelList, StatementSequence>;

template<>
struct fmt::formatter<Case> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(Case const& id, FormatContext& ctx) {
      auto [label, state] = id;
      return fmt::format_to(ctx.out(), "{} : {}", label, state);
  }
};

struct CaseStatement : Statement {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        return fmt::format("CASE {} OF {} END", expression, fmt::join(cases, " |\n"));
    }
    CaseStatement(ExpressionPtr e, std::vector<Case> c) : expression(e), cases(c) {}
    ExpressionPtr expression;
    std::vector<Case> cases;
};

struct WhileStatement : Statement {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        auto [cond, block] = if_blocks[0];
        std::vector<IfBlock> elsif_blocks = {if_blocks.begin()+1, if_blocks.end()};
        return fmt::format("WHILE {} DO\n{}\n{}\nEND", cond, block, fmt::join(elsif_blocks, "\n"));
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

template <class T>
ParserPtr<std::vector<T>> unwrap_maybe_list(ParserPtr<std::vector<std::optional<T>>> parser) {
    return extension(parser, [](const auto& seq){
        std::vector<T> res;
        for (auto& elem : seq) {
            if (elem) res.push_back(*elem);
        }
        return res;
    });
}

using VariableDecl = FieldList;

struct DeclarationSequence {
    std::vector<ConstDecl> constDecls;
    std::vector<TypeDecl> typeDecls;
    std::vector<VariableDecl> variableDecls;
    std::vector<CodeBlockPtr> procedureDecls;
};

template<>
struct fmt::formatter<DeclarationSequence> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(DeclarationSequence const& id, FormatContext& ctx) {
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

struct FormalType {
    bool array;
    QualIdent ident;
};

template<>
struct fmt::formatter<FormalType> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(FormalType const& id, FormatContext& ctx) {
      if (id.array) return fmt::format_to(ctx.out(), "ARRAY OF {}", id.ident);
      else return fmt::format_to(ctx.out(), "{}", id.ident);
  }
};

struct FPSection {
    std::optional<Ident> var;
    std::vector<Ident> idents;
    FormalType type;
};

template<>
struct fmt::formatter<FPSection> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(FPSection const& id, FormatContext& ctx) {
      if (id.var) return fmt::format_to(ctx.out(), "{} {} : {}", *id.var, fmt::join(id.idents, ", "), id.type);
      else return fmt::format_to(ctx.out(), "{} : {}", fmt::join(id.idents, ", "), id.type);
  }
};

struct FormalParameters {
    std::vector<FPSection> sections;
    std::optional<QualIdent> rettype;
};

template<>
struct fmt::formatter<FormalParameters> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(FormalParameters const& id, FormatContext& ctx) {
      if (id.rettype) return fmt::format_to(ctx.out(), "({}) : {}", fmt::join(id.sections, "; "), *id.rettype);
      else return fmt::format_to(ctx.out(), "({})", fmt::join(id.sections, "; "));
  }
};

struct ProcedureDeclaration : CodeBlock {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        auto formal = params ? fmt::format("{}", *params) : "";
        std::string statements;
        if (!body.empty()) statements = fmt::format("BEGIN {}\n", body);
        std::string ret_str;
        if (ret) statements = fmt::format("RETURN {}\n", *ret);
        return fmt::format("PROCEDURE {} {};\n{}\n{}{}END {}", name, formal, decls, statements, ret_str, name);
    }
    ProcedureDeclaration() {}
    ProcedureDeclaration(IdentDef n,
                         std::optional<FormalParameters> p,
                         DeclarationSequence d,
                         StatementSequence b,
                         std::optional<ExpressionPtr> r)
        : name(n), params(p), decls(d), body(b), ret(r) {}
    IdentDef name;
    std::optional<FormalParameters> params;
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

template<>
struct fmt::formatter<Import> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
  template<typename FormatContext>
  auto format(Import const& id, FormatContext& ctx) {
      if (id.real_name == id.name) return fmt::format_to(ctx.out(), "{}", id.name);
      else return fmt::format_to(ctx.out(), "{} := {}", id.name, id.real_name);
  }
};

using ImportList = std::vector<Import>;

struct Module : CodeBlock {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        return fmt::format("MODULE {}; IMPORT {}\n{}\nBEGIN\n{}\nEND {}", name, fmt::join(imports, ", "), declarations, body, name);
    }
    Module() {}
    Module(Ident n, ImportList i, DeclarationSequence d, StatementSequence b)
        : name(n), imports(i), declarations(d), body(b) {}
    Ident name;
    ImportList imports;
    DeclarationSequence declarations;
    StatementSequence body;
};

auto oberon_parser() {

    ParserPtr<TypePtr> type;
    ParserPtr<ExpressionPtr> expression;

    ParserPtr<char> any = predicate("any symbol", [](auto){ return true; });
    ParserPtr<char> letter = predicate("letter", [](auto c){ return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); });
    ParserPtr<char> digit = predicate("digit", isdigit);
    ParserPtr<char> hexdigit = predicate("hexdigit", isxdigit);
    ParserPtr<Ident> identifier = chain(letter, many(either({letter, digit})));
    ParserPtr<Ident> ident = not_from(identifier, keywords);

    auto keyword = [identifier](std::string_view key){
        std::vector<char> val({});
        val.insert(val.begin(), key.begin(), key.end());
        return equal_to(identifier, val);
    };

    ParserPtr<Nil> nil = extension(keyword("NIL"), [](const auto&){ return Nil{}; });

    ParserPtr<Boolean> boolean = extension(either({keyword("TRUE"), keyword("FALSE")}), [](const auto& key){
        return Boolean(key == std::vector<char>{'T', 'R', 'U', 'E'});
    });

    ParserPtr<size_t> hexnumber = extension(chain(digit, many(hexdigit)), [](const auto& res){
        size_t result = 0;
        for (auto symbol : res) {
            size_t num = 0;
            if (isdigit(symbol)) num = symbol - '0';
            else num = symbol - 'A' + 10;
            result *= 16;
            result += num;
        }
        return result;
    });

    ParserPtr<size_t> decnumber = extension(some(digit), [](const auto& res){
        size_t result = 0;
        for (auto symbol : res) {
            size_t num = symbol - '0';
            result *= 10;
            result += num;
        }
        return result;
    });

    ParserPtr<Integer> integer = construct<Integer>(either({parse_index<0>::select(hexnumber, symbol('H')), decnumber}));

    auto scale_parser = sequence(either({symbol('E'), symbol('D')}), either({symbol('+'), symbol('-')}), decnumber);
    auto real_parser = parse_index<0,2,3>::select(decnumber, symbol('.'), decnumber, maybe(scale_parser));
    ParserPtr<Real> real = extension(real_parser, &parse_real);


    ParserPtr<Number> number = construct<Number>(variant(real, integer));

    ParserPtr<Char> charConst = construct<Char>(either({
                parse_index<1>::select(symbol('\''), any, symbol('\'')),
                construct<char>(parse_index<0>::select(hexnumber, symbol('X')))
            }));

    ParserPtr<String> string = construct<String>(either({
                parse_index<1>::select(symbol('\''), many(inverse('\'')), symbol('\'')),
                parse_index<1>::select(symbol('"'), many(inverse('"')), symbol('"')),
            }));

    ParserPtr<SetElement>
        set_element =
        construct<SetElement>(sequence(expression, maybe(syntax_index<1>::select(symbols(".."), expression))));

    ParserPtr<Set> set = construct<Set>(syntax_index<1>::select(symbol('{'), maybe(extra_delim(set_element, symbol(','))) , symbol('}')));

    ParserPtr<QualIdent> qualident = construct<QualIdent>(sequence(maybe(parse_index<0>::select(ident, symbol('.'))), ident));

    ParserPtr<IdentDef> identdef =  extension(sequence(ident, maybe(symbol('*'))), [](const auto& pair) {
        auto [ident, def] = pair;
        return IdentDef{ident, (bool)def};
    });

    ParserPtr<IdentList> identList = extra_delim(identdef, symbol(','));

    ParserPtr<TypeName> typeName = construct<TypeName>(qualident);

    ParserPtr<FieldList> fieldList = construct<FieldList>(syntax_index<0,2>::select(identList, symbol(':'), type));

    auto variableDecl = fieldList;

    ParserPtr<FieldListSequence> fieldListSequence = extra_delim(fieldList, symbol(';'));

    ParserPtr<RecordType> recordType = extension(
                                                 syntax_index<1, 2>::select(
                                                                            keyword("RECORD"),
                                                                            maybe(syntax_index<1>::select(symbol('('), qualident, symbol(')'))),
                                                                            maybe(fieldListSequence), keyword("END")),
                                                 [](const auto &data) {
                                                     auto [basetype, fieldList] = data;
                                                     if (fieldList)
                                                         return RecordType(basetype, *fieldList);
                                                     else
                                                         return RecordType(basetype, {});
                                                 });

    ParserPtr<PointerType> pointerType = construct<PointerType>(syntax_index<2>::select(keyword("POINTER"), keyword("TO"), type));

    ParserPtr<ArrayType> arrayType =
        construct<ArrayType>(syntax_index<1, 3>::select(
                                                        keyword("ARRAY"), extra_delim(expression, symbol(',')), keyword("OF"),
                                                        type));

    ParserPtr<TypePtr> strucType = base_either<Type>(recordType, pointerType, arrayType);

    type = either({strucType, base_either<Type>(typeName)});

    ParserPtr<ExpList> expList = extra_delim(expression, symbol(','));

    ParserPtr<Designator> designator =
        construct<Designator>(sequence(qualident, many(variant(
                                                            parse_index<1>::select(symbol('.'), ident),
                                                            syntax_index<1>::select(symbol('['), expList, symbol(']')),
                                                            symbol('^'),
                                                            syntax_index<1>::select(symbol('('), qualident, symbol(')'))
                                                               ))));

    ParserPtr<ExpList> actualParameters =
        syntax_index<1>::select(symbol('('), maybe_list(expList), symbol(')'));

    ParserPtr<ProcCall> procCall = construct<ProcCall>(sequence(designator, maybe(actualParameters)));

    ParserPtr<ExpressionPtr> factor;

    ParserPtr<Tilda> tilda = construct<Tilda>(syntax_index<1>::select(symbol('~'), factor));

    factor = either({base_either<Expression>(number, charConst, string, nil, boolean, set, procCall, tilda),
            syntax_index<1>::select(symbol('('), expression, symbol(')'))});

    ParserPtr<Operator> mulOperator = either({
            construct<Operator>(either({keyword("DIV"), keyword("MOD")})),
            construct<Operator>(either({symbol('*'), symbol('/'), symbol('&')}))
        });

    ParserPtr<ExpressionPtr> term;
    term = base_either<Expression>(construct<Term>(syntax_sequence(factor,
                                                                   maybe(syntax_sequence(mulOperator, term)))));

    ParserPtr<Operator> addOperator = either({
            construct<Operator>(keyword("OR")),
            construct<Operator>(either({symbol('+'), symbol('-')}))
        });

    ParserPtr<ExpressionPtr> simpleExpression;
    simpleExpression = base_either<Expression>(construct<Term>(syntax_sequence(maybe(either({symbol('+'), symbol('-')})),
                                                                                                        term,
                                                                                                        maybe(syntax_sequence(addOperator, simpleExpression)))));

    ParserPtr<Operator> relation = either({
            construct<Operator>(either({keyword("IN"), keyword("IS")})),
            construct<Operator>(either({symbols("<="), symbols(">=")})),
            construct<Operator>(either({symbol('<'), symbol('>'), symbol('#'), symbol('=')}))
        });

    expression = base_either<Expression>(construct<Term>(syntax_sequence(simpleExpression,
                                                                         maybe(syntax_sequence(relation, simpleExpression)))));

    ParserPtr<ConstDecl> constDecl = construct<ConstDecl>(syntax_index<0,2>::select(identdef, symbol('='), expression));

    ParserPtr<TypeDecl> typeDecl = construct<TypeDecl>(syntax_index<0,2>::select(identdef, symbol('='), strucType));

    ParserPtr<StatementPtr> statement;

    auto assignment = construct<Assignment>(syntax_index<0,2>::select(designator, symbols(":="), expression));

    auto statementSequence = unwrap_maybe_list(extra_delim(maybe(statement), symbol(';')));

    auto elsif = syntax_index<1,3>::select(keyword("ELSIF"), expression, keyword("THEN"), statementSequence);

    auto ifStatement
        = construct<IfStatement>(syntax_index<0,1>::select(chain(syntax_index<1,3>::select(keyword("IF"), expression, keyword("THEN"), statementSequence),
                                                                   many(elsif)),
                                                             maybe(syntax_index<1>::select(keyword("ELSE"), statementSequence)),
                                                             keyword("END")));

    auto lbl = base_either<Expression>(
                                       except(number, "integer value", [](const auto& num){
                                           return (bool)std::get_if<Integer>(&num.value);
                                       }),
                                       except(procCall, "simple ident", [](const auto& proc){
                                           return !proc.params && proc.ident.selector.empty() && !proc.ident.ident.qual;
                                       }));
    auto caseLabel = construct<CaseLabel>(syntax_sequence(lbl, maybe(syntax_index<1>::select(symbols(".."), lbl))));
    auto caseLabelList = extra_delim(caseLabel, symbol(','));
    auto oneCase = syntax_index<0,2>::select(caseLabelList, symbol(':'), statementSequence);
    auto caseStatement = construct<CaseStatement>(syntax_index<1,3>::select(keyword("CASE"), expression,
                                                                          keyword("OF"),
                                                                          unwrap_maybe_list(extra_delim(maybe(oneCase), symbol('|'))),
                                                                            keyword("END")));

    auto whileStatement
        = construct<WhileStatement>(syntax_index<0>::select(chain(syntax_index<1,3>::select(keyword("WHILE"), expression, keyword("DO"), statementSequence),
                                                                    many(elsif)),
                                                              keyword("END")));

    auto repeatStatement = construct<RepeatStatement>(syntax_index<1,3>::select(keyword("REPEAT"), statementSequence, keyword("UNTIL"), expression));

    auto forStatement = construct<ForStatement>(syntax_index<1,3,5,6,8>::select(keyword("FOR"),
                                                                              ident, symbols(":="), expression, keyword("TO"), expression,
                                                                        maybe(syntax_index<1>::select(keyword("BY"), expression)),
                                                                        keyword("DO"), statementSequence, keyword("END")));

    statement = base_either<Statement>(assignment, procCall, ifStatement, caseStatement, whileStatement, repeatStatement, forStatement);

    ParserPtr<CodeBlockPtr> procedureDecl;

    auto declarationSequence = construct<DeclarationSequence>(syntax_sequence(maybe_list(syntax_index<1>::select(keyword("CONST"), extra_delim0(constDecl, symbol(';')))),
                                                                              maybe_list(syntax_index<1>::select(keyword("TYPE"), extra_delim0(typeDecl, symbol(';')))),
                                                                              maybe_list(syntax_index<1>::select(keyword("VAR"), extra_delim0(variableDecl, symbol(';')))),
                                                                              syntax_index<0>::select(extra_delim0(procedureDecl, symbol(';')))));

    auto formalType = construct<FormalType>(syntax_sequence(option(syntax_sequence(keyword("ARRAY"), keyword("OF"))), qualident));
    auto fpSection = construct<FPSection>(syntax_sequence(maybe(either({keyword("CONST"), keyword("VAR")})),
                                                          extra_delim(ident, symbol(',')),
                                                          syntax_index<1>::select(symbol(':'), formalType)));

    auto formalParameters = construct<FormalParameters>(syntax_index<1,3>::select(symbol('('), maybe_list(extra_delim(fpSection, symbol(';'))), symbol(')'),
                                                                                  maybe(syntax_index<1>::select(symbol(':'), qualident))));

    auto procDeclBase = construct<ProcedureDeclaration>(syntax_index<1, 2, 4, 5, 6>::select(
            keyword("PROCEDURE"), identdef, maybe(formalParameters), symbol(';'),
            declarationSequence,
            maybe_list(syntax_index<1>::select(keyword("BEGIN"), statementSequence)),
            maybe(syntax_index<1>::select(keyword("RETURN"), expression)),
            keyword("END")));

    procedureDecl = base_either<CodeBlock>(parse_index<0>::tuple_select(except(syntax_sequence(procDeclBase, ident), "same ident", [](const auto& pair){
        auto& [proc, ident] = pair;
        return proc.name.ident == ident;
    })));

    auto import = construct<Import>(syntax_sequence(ident, maybe(syntax_index<1>::select(symbols(":="), ident))));

    auto importList = syntax_index<1>::select(keyword("IMPORT"), extra_delim(import, symbol(',')), symbol(';'));

    auto moduleBase = construct<Module>(syntax_index<1,3,4,5>::select(keyword("MODULE"),
                                                                      ident, symbol(';'),
                                                                      maybe_list(importList),
                                                                      declarationSequence,
                                                                      maybe_list(syntax_index<1>::select(keyword("BEGIN"), statementSequence)),
                                                                      keyword("END")));

    auto module = parse_index<0>::tuple_select(except(syntax_sequence(moduleBase, ident), "same module name", [](const auto& pair){
        auto& [mod, ident] = pair;
        return mod.name == ident;
    }));

    return module;
}

int main() {
    CodeStream code("data.txt");
    auto res = oberon_parser()->parse(code);
    if (res) {
        fmt::print("{}", res.get_ok().to_string());
    } else {
        fmt::print(res.get_err().to_string());
    }
    // if (res) {
    //     // std::visit([](auto&& arg){fmt::print("result: {}", arg);}, res.get_ok());
    //     // fmt::print("result: {}", res.get_ok());
    //     auto ok = res.get_ok();
    //     for (auto fieldList : ok.seq) {
    //         for (auto ident : fieldList.list) {
    //             fmt::print("{}, ", fmt::join(ident.ident, ""));
    //         }
    //         fmt::print("| {}.{} |", fmt::join(*fieldList.type.ident.qual, ""), fmt::join(fieldList.type.ident.ident, ""));
    //     }
    // } else {
    //     fmt::print(res.get_err().to_string());
    // }
    return 0;
}
