#include "parser.hpp"

#include "plib/parser.hpp"
#include "plib/parsers.hpp"

using namespace nodes;

Ident str_to_ident(std::string_view str) {
    std::vector<char> vec;
    vec.insert(vec.end(), str.begin(), str.end());
    return vec;
}

ParserPtr<char> inverse(char c) {
    return predicate(fmt::format("not {}", c), [c](auto n) { return n != c; });
}

template <class T>
class EqualTo : public Parser<T> {
  public:
    EqualTo(ParserPtr<T> parser, T&& value) : m_parser(parser), m_value(std::forward<T>(value)) {}
    ParseResult<T> parse(CodeStream& stream) const noexcept override {
        BreakPoint point(stream);
        if (auto res = m_parser->parse(stream); res) {
            if (res.get_ok() == m_value) {
                point.close();
                return res;
            } else {
                auto err = ParseError(fmt::format("{}", m_value), res.get_ok(), stream);
                point.error(err);
                return err;
            }
        } else {
            point.error(res.err_ref());
            return res;
        }
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
    while (real_part > 0)
        real_part /= 10;
    Real real = ipart + real_part;
    if (scale_val) {
        auto [l, sign, scale_power] = *scale_val;
        Real scale = (sign == '+' ? 1 : -1) * std::pow(10, scale_power);
        real *= scale;
    }
    return real;
}

ParserPtr<char> any_func() {
    return predicate("any symbol", [](auto) { return true; });
}

template <class T, class P>
ParserPtr<T> change(ParserPtr<P> parser, T ch) {
    return extension(parser, [ch](const auto&) { return ch; });
}

template <class T, class E>
class Limiters : public Parser<std::vector<T>> {
  public:
    Limiters(ParserPtr<E> begin, ParserPtr<T> parser, ParserPtr<E> end)
        : m_begin(begin), m_parser(parser), m_end(end) {}
    ParseResult<std::vector<T>> parse(CodeStream& stream) const noexcept override {
        std::vector<T> result;
        BreakPoint point(stream);
        if (auto bres = m_begin->parse(stream); bres) {
            while (true) {
                if (auto eres = m_end->parse(stream); eres) {
                    point.close();
                    return result;
                } else if (auto pres = m_parser->parse(stream); pres) {
                    point.error(eres.err_ref());
                    if (eres.err_ref().is_undroppable())
                        return eres.get_err();
                    result.push_back(pres.get_ok());
                } else {
                    point.error(pres.err_ref());
                    if (pres.err_ref().is_undroppable())
                        return pres.get_err();
                    return ParseError("sequence", "??", stream);
                }
            }
        } else {
            point.error(bres.err_ref());
            return bres.get_err();
        }
    }

  private:
    ParserPtr<E> m_begin;
    ParserPtr<T> m_parser;
    ParserPtr<E> m_end;
};

template <class T, class E>
inline ParserPtr<std::vector<T>> limiters(ParserPtr<E> begin, ParserPtr<T> parser, ParserPtr<E> end) {
    return make_parser(Limiters(begin, parser, end));
}

ParserPtr<char> comment() {
    ParserLinker<std::vector<char>> comm;

    auto lcomm = comm.link(limiters(symbols("(*"), either({any_func(), change(comm.get(), '0')}), symbols("*)")));

    return change(lcomm, '0');
}

ParserPtr<std::vector<char>> delim() {
    return many(either({symbol(' '), symbol('\n'), comment()}));
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
    template <class... Types>
    static auto select(ParserPtr<Types>... parsers) {
        return delim_index<Is...>::select(delim(), parsers...);
    }
};

template <class T>
class NodeWrapper : public Parser<T> {
  public:
    NodeWrapper(ParserPtr<T> parser) : m_parser(parser) {}
    ParseResult<T> parse(CodeStream& stream) const noexcept override {
        auto place = stream.place();
        if (auto res = m_parser->parse(stream); res) {
            auto ok = res.get_ok();
            ok->place = place;
            return ok;
        } else
            return res;
    }

  private:
    ParserPtr<T> m_parser;
};

template <class T>
inline auto node_wrapper(ParserPtr<T> parser) {
    return make_parser(NodeWrapper(parser));
}


template <class T>
class SetPlaceWrapper : public Parser<T> {
  public:
    SetPlaceWrapper(ParserPtr<T> parser) : m_parser(parser) {}
    ParseResult<T> parse(CodeStream& stream) const noexcept override {
        auto place = stream.place();
        if (auto res = m_parser->parse(stream); res) {
            auto ok = res.get_ok();
            ok.place = place;
            return ok;
        } else
            return res;
    }
  private:
    ParserPtr<T> m_parser;
};

template <class T>
inline auto set_place(ParserPtr<T> parser) {
    return make_parser(SetPlaceWrapper(parser));
}

template <class Base, class... Types>
inline auto node_either(ParserPtr<Types>... parsers) {
    return node_wrapper(base_either<Base, Types...>(parsers...));
}

template <class T>
ParserPtr<std::vector<T>> unwrap_maybe_list(ParserPtr<std::vector<std::optional<T>>> parser) {
    return extension(parser, [](const auto& seq) {
        std::vector<T> res;
        for (auto& elem : seq) {
            if (elem)
                res.push_back(*elem);
        }
        return res;
    });
}

template <class T>
ParserPtr<std::vector<T>> maybe_list(ParserPtr<std::vector<T>> parser) {
    return extension(maybe(parser), [](const auto& l) {
        if (l)
            return *l;
        else
            return std::vector<T>{};
    });
}

const std::vector<Ident> keywords = []() {
    std::vector<std::string_view> vec = {
        "ARRAY",  "FOR",   "PROCEDURE", "BEGIN", "IF",    "RECORD", "BY", "IMPORT", "REPEAT", "CASE",    "IN",
        "RETURN", "CONST", "IS",        "THEN",  "DIV",   "MOD",    "TO", "DO",     "MODULE", "TRUE",    "ELSE",
        "NIL",    "TYPE",  "ELSIF",     "OF",    "UNTIL", "END",    "OR", "VAR",    "FALSE",  "POINTER", "WHILE"};
    std::vector<Ident> result{};
    std::transform(vec.begin(), vec.end(), std::back_inserter(result), [](auto s) { return str_to_ident(s); });
    return result;
}();

ParserPtr<char> any = any_func();
ParserPtr<char> letter =
    predicate("letter", [](auto c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); });
ParserPtr<char> digit = predicate("digit", isdigit);
ParserPtr<char> hexdigit = predicate("hexdigit", isxdigit);
ParserPtr<Ident> identifier = set_place(construct<Ident>(chain(letter, many(either({letter, digit})))));
ParserPtr<Ident> ident = not_from(identifier, keywords);

ParserPtr<Ident> keyword(std::string_view key) {
    std::vector<char> val({});
    val.insert(val.begin(), key.begin(), key.end());
    return no_return(equal_to(identifier, Ident(val)));
}

ParserPtr<QualIdent> qualident =
    construct<QualIdent>(sequence(maybe(parse_index<0>::select(ident, symbol('.'))), ident));

ParserPtr<IdentDef> identdef = extension(sequence(ident, maybe(symbol('*'))), [](const auto& pair) {
    auto [ident, def] = pair;
    return IdentDef{ident, (bool)def};
 });

auto type_parser(ParserPtr<ExpressionPtr> expression) {
    ParserLinker<TypePtr> typeLink;
    auto type = typeLink.get();

    ParserPtr<IdentList> identList = extra_delim(identdef, symbol(','));
    ParserPtr<FieldList> fieldList = construct<FieldList>(syntax_index<0, 2>::select(identList, symbol(':'), type));

    ParserPtr<BuiltInType> builtInType = construct<BuiltInType>(either({symbols("BOOLEAN"), symbols("CHAR"),
                symbols("INTEGER"), symbols("REAL"),
                symbols("BYTE"), symbols("SET")}));

    ParserPtr<TypePtr> typeName = node_either<Type>(builtInType, construct<TypeName>(qualident));

    ParserPtr<FieldListSequence> fieldListSequence = extra_delim(fieldList, symbol(';'));

    ParserPtr<RecordType> recordType =
        extension(syntax_index<1, 2>::select(keyword("RECORD"),
                                             maybe(syntax_index<1>::select(symbol('('), qualident, symbol(')'))),
                                             maybe(fieldListSequence), keyword("END")),
                  [](const auto& data) {
                      auto [basetype, fieldList] = data;
                      if (fieldList)
                          return RecordType(basetype, *fieldList);
                      else
                          return RecordType(basetype, {});
                  });

    ParserPtr<PointerType> pointerType =
        construct<PointerType>(syntax_index<2>::select(keyword("POINTER"), keyword("TO"), type));

    ParserPtr<ArrayType> arrayType = construct<ArrayType>(
        syntax_index<1, 3>::select(keyword("ARRAY"), extra_delim(expression, symbol(',')), keyword("OF"), type));

    ParserPtr<TypePtr> formalType = extension(syntax_sequence(many(syntax_sequence(keyword("ARRAY"), keyword("OF"))), qualident),
        [](const auto& data) {
            auto [array, ident] = data;
            auto typeName = is_base_type(ident.ident) ? make_type<BuiltInType>(ident.ident) : make_type<TypeName>(ident);
            if (array.size() > 0) {
                std::vector<ExpressionPtr> vec(array.size(), make_expression<Number>(Integer(1)));
                return make_type<ArrayType>(vec, typeName, true);
            } else return typeName;
        });

    auto fpSection = construct<FPSection>(syntax_sequence(maybe(either({keyword("CONST"), keyword("VAR")})),
                                                          extra_delim(ident, symbol(',')),
                                                          syntax_index<1>::select(symbol(':'), formalType)));

    auto formalParameters = construct<FormalParameters>(
        syntax_index<1, 3>::select(symbol('('), maybe_list(extra_delim(fpSection, symbol(';'))), symbol(')'),
                                   maybe(syntax_index<1>::select(symbol(':'), typeName))));

    auto procedureType = construct<ProcedureType>(syntax_index<1>::select(keyword("PROCEDURE"), maybe(formalParameters)));

    ParserPtr<TypePtr> strucType = node_either<Type>(recordType, pointerType, arrayType, procedureType);

    auto realType = typeLink.link(either({strucType, typeName}));

    return std::tuple{realType, formalParameters, fieldList};
}

auto expression_parser() {
    ParserLinker<ExpressionPtr> expressionLink;
    auto expression = expressionLink.get();

    ParserPtr<Nil> nil = extension(keyword("NIL"), [](const auto&) { return Nil{}; });

    ParserPtr<Boolean> boolean = extension(either({keyword("TRUE"), keyword("FALSE")}), [](const auto& key) {
        return Boolean(key == std::vector<char>{'T', 'R', 'U', 'E'});
    });

    ParserPtr<size_t> hexnumber = extension(chain(digit, many(hexdigit)), [](const auto& res) {
        size_t result = 0;
        for (auto symbol : res) {
            size_t num = 0;
            if (isdigit(symbol))
                num = symbol - '0';
            else
                num = symbol - 'A' + 10;
            result *= 16;
            result += num;
        }
        return result;
    });

    ParserPtr<size_t> decnumber = extension(some(digit), [](const auto& res) {
        size_t result = 0;
        for (auto symbol : res) {
            size_t num = symbol - '0';
            result *= 10;
            result += num;
        }
        return result;
    });

    ParserPtr<Integer> integer =
        construct<Integer>(either({parse_index<0>::select(hexnumber, symbol('H')), decnumber}));

    auto scale_parser = sequence(either({symbol('E'), symbol('D')}), either({symbol('+'), symbol('-')}), decnumber);

    auto real_parser = parse_index<0, 2, 3>::select(decnumber, symbol('.'), decnumber, maybe(scale_parser));

    ParserPtr<Real> real = extension(real_parser, &parse_real);

    ParserPtr<Number> number = construct<Number>(variant(real, integer));

    ParserPtr<Char> charConst =
        construct<Char>(either({parse_index<1>::select(symbol('\''), any, symbol('\'')),
                                construct<char>(parse_index<0>::select(hexnumber, symbol('X')))}));

    ParserPtr<String> string = construct<String>(either({
        parse_index<1>::select(symbol('\''), many(inverse('\'')), symbol('\'')),
        parse_index<1>::select(symbol('"'), many(inverse('"')), symbol('"')),
    }));

    ParserPtr<SetElement> set_element =
        construct<SetElement>(sequence(expression, maybe(syntax_index<1>::select(symbols(".."), expression))));

    ParserPtr<Set> set =
        construct<Set>(syntax_index<1>::select(symbol('{'), maybe(extra_delim(set_element, symbol(','))), symbol('}')));

    ParserPtr<ExpList> expList = extra_delim(expression, symbol(','));

    ParserPtr<Designator> designator = construct<Designator>(
        sequence(qualident, many(variant(parse_index<1>::select(symbol('.'), ident),
                                         syntax_index<1>::select(symbol('['), expList, symbol(']')), symbol('^'),
                                         syntax_index<1>::select(symbol('('), qualident, symbol(')'))))));

    ParserPtr<ExpList> actualParameters = syntax_index<1>::select(symbol('('), maybe_list(expList), symbol(')'));

    ParserPtr<ProcCall> procCall = construct<ProcCall>(sequence(designator, maybe(actualParameters)));

    ParserLinker<ExpressionPtr> factorLink;

    ParserPtr<Tilda> tilda = construct<Tilda>(syntax_index<1>::select(symbol('~'), factorLink.get()));

    auto preFactor = either({node_either<Expression>(charConst, number, string, nil, boolean, set, procCall, tilda),
                     syntax_index<1>::select(symbol('('), expression, symbol(')'))});
    auto factor = factorLink.link(preFactor);

    ParserPtr<Operator> mulOperator = either({construct<Operator>(either({keyword("DIV"), keyword("MOD")})),
                                              construct<Operator>(either({symbol('*'), symbol('/'), symbol('&')}))});

    ParserLinker<ExpressionPtr> termLink;
    auto preTerm = node_either<Expression>(construct<Term>(syntax_sequence(factor, maybe(syntax_sequence(mulOperator, termLink.get())))));
    auto term = termLink.link(preTerm);

    ParserPtr<Operator> addOperator =
        either({construct<Operator>(keyword("OR")), construct<Operator>(either({symbol('+'), symbol('-')}))});

    ParserLinker<ExpressionPtr> simpleExpressionLink;
    auto preSimpleExpression = node_either<Expression>(construct<Term>(syntax_sequence(
        maybe(either({symbol('+'), symbol('-')})), term, maybe(syntax_sequence(addOperator, simpleExpressionLink.get())))));
    auto simpleExpression = simpleExpressionLink.link(preSimpleExpression);

    ParserPtr<Operator> relation =
        either({construct<Operator>(either({keyword("IN"), keyword("IS")})),
                construct<Operator>(either({symbols("<="), symbols(">=")})),
                construct<Operator>(either({symbol('<'), symbol('>'), symbol('#'), symbol('=')}))});

    auto preExpression = node_either<Expression>(construct<Term>(syntax_sequence(simpleExpression, maybe(syntax_sequence(relation, simpleExpression)))));
    auto realExpression = expressionLink.link(preExpression);

    auto lbl = variant(integer, string, qualident);

    return std::tuple{realExpression, procCall, designator, lbl};
}

auto statement_parser(ParserPtr<ExpressionPtr> expression,
                      ParserPtr<Label> lbl, ParserPtr<Designator> designator, ParserPtr<ProcCall> procCall) {
    ParserLinker<std::vector<StatementPtr>> statementSequenceLink;
    auto statementSequence = statementSequenceLink.get();

    auto assignment = construct<Assignment>(syntax_index<0, 2>::select(designator, symbols(":="), expression));

    auto elsif = syntax_index<1, 3>::select(keyword("ELSIF"), expression, keyword("THEN"), statementSequence);

    auto ifStatement = construct<IfStatement>(syntax_index<0, 1>::select(
        chain(syntax_index<1, 3>::select(keyword("IF"), expression, keyword("THEN"), statementSequence), many(elsif)),
        maybe(syntax_index<1>::select(keyword("ELSE"), statementSequence)), keyword("END")));

    auto caseLabel = construct<CaseLabel>(syntax_sequence(lbl, maybe(syntax_index<1>::select(symbols(".."), lbl))));

    auto caseLabelList = extra_delim(caseLabel, symbol(','));

    auto oneCase = syntax_index<0, 2>::select(caseLabelList, symbol(':'), statementSequence);

    auto caseStatement = construct<CaseStatement>(
        syntax_index<1, 3>::select(keyword("CASE"), expression, keyword("OF"),
                                   unwrap_maybe_list(extra_delim(maybe(oneCase), symbol('|'))), keyword("END")));

    auto whileStatement = construct<WhileStatement>(syntax_index<0>::select(
        chain(syntax_index<1, 3>::select(keyword("WHILE"), expression, keyword("DO"), statementSequence), many(elsif)),
        keyword("END")));

    auto repeatStatement = construct<RepeatStatement>(
        syntax_index<1, 3>::select(keyword("REPEAT"), statementSequence, keyword("UNTIL"), expression));

    auto forStatement = construct<ForStatement>(syntax_index<1, 3, 5, 6, 8>::select(
        keyword("FOR"), ident, symbols(":="), expression, keyword("TO"), expression,
        maybe(syntax_index<1>::select(keyword("BY"), expression)), keyword("DO"), statementSequence, keyword("END")));

    auto statement = node_either<Statement>(assignment, construct<CallStatement>(set_place(procCall)), ifStatement, caseStatement, whileStatement,
                                       repeatStatement, forStatement);
    auto realStatementSequence = statementSequenceLink.link(unwrap_maybe_list(extra_delim(maybe(statement), symbol(';'))));

    return realStatementSequence;
}

auto declarations_parser(ParserPtr<TypePtr> type, ParserPtr<ExpressionPtr> expression,
                         ParserPtr<FieldList> fieldList, ParserPtr<FormalParameters> formalParameters,
                         ParserPtr<StatementSequence> statementSequence) {
    auto variableDecl = fieldList;

    ParserPtr<ConstDecl> constDecl =
        construct<ConstDecl>(syntax_index<0, 2>::select(identdef, symbol('='), expression));

    ParserPtr<TypeDecl> typeDecl = construct<TypeDecl>(syntax_index<0, 2>::select(identdef, symbol('='), type));

    ParserLinker<DeclarationSequence> declarationSequenceLink;

    auto procDeclBase = construct<ProcedureDeclaration>(syntax_index<1, 2, 4, 5, 6>::select(
        keyword("PROCEDURE"), identdef, construct<ProcedureType>(maybe(formalParameters)), symbol(';'),
        declarationSequenceLink.get(), maybe_list(syntax_index<1>::select(keyword("BEGIN"), statementSequence)),
        maybe(syntax_index<1>::select(keyword("RETURN"), expression)), keyword("END")));

    auto procedureDecl = node_either<Section>(
        parse_index<0>::tuple_select(except(syntax_sequence(procDeclBase, ident), "same ident", [](const auto& pair) {
            auto& [proc, ident] = pair;
            return proc.name.ident == ident;
        })));

    auto preDeclarationSequence = construct<DeclarationSequence>(
        syntax_sequence(maybe_list(syntax_index<1>::select(keyword("CONST"), extra_delim0(constDecl, symbol(';')))),
                        maybe_list(syntax_index<1>::select(keyword("TYPE"), extra_delim0(typeDecl, symbol(';')))),
                        maybe_list(syntax_index<1>::select(keyword("VAR"), extra_delim0(variableDecl, symbol(';')))),
                        maybe_list(extra_delim0(procedureDecl, symbol(';')))));

    auto declarationSequence = declarationSequenceLink.link(preDeclarationSequence);

    return declarationSequence;
}

auto module_parser(ParserPtr<DeclarationSequence> declarationSequence, ParserPtr<StatementSequence> statementSequence) {

    auto import = construct<Import>(syntax_sequence(ident, maybe(syntax_index<1>::select(symbols(":="), ident))));

    auto importList = syntax_index<1>::select(keyword("IMPORT"), extra_delim(import, symbol(',')), symbol(';'));

    auto moduleBase = construct<Module>(syntax_index<1, 3, 4, 5>::select(
        keyword("MODULE"), ident, symbol(';'), maybe_list(importList), declarationSequence,
        maybe_list(syntax_index<1>::select(keyword("BEGIN"), statementSequence)), keyword("END")));

    auto module = parse_index<0>::tuple_select(
        except(syntax_sequence(moduleBase, ident, symbol('.')), "same module name", [](const auto& pair) {
            auto& [mod, ident, dot] = pair;
            return mod.name == ident;
        }));

    return module;
}

ParserPtr<Module> get_parser() {

    auto [expression, procCall, designator, lbl] = expression_parser();

    auto statementSequence = statement_parser(expression, lbl, designator, procCall);

    auto [type, formalParameters, fieldList] = type_parser(expression);

    auto declarationSequence = declarations_parser(type, expression, fieldList, formalParameters, statementSequence);

    auto module = module_parser(declarationSequence, statementSequence);

    return module;
}
