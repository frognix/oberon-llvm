#pragma once

#include "parser.hpp"
#include "selecter.hpp"
#include "break_point.hpp"


template <class T, size_t From>
class CountFrom : public Parser<std::vector<T>> {
public:
    CountFrom(ParserPtr<T> parser) : m_parser(std::move(parser)) {}
    ParseResult<std::vector<T>> parse(CodeStream& stream) override {
        std::vector<T> result;
        BreakPoint point(stream);
        while (true) {
            if (auto val = m_parser->parse(stream); val) {
                result.push_back(val.get_ok());
            } else {
                if (result.size() < From)
                    return val.get_err();
                point.close();
                return result;
            }
        }
    }
private:
    ParserPtr<T> m_parser;
};

template <class T>
inline ParserPtr<std::vector<T>> many(ParserPtr<T> p) {
    return make_parser(CountFrom<T, 0>(p));
}

template <class T>
inline ParserPtr<std::vector<T>> some(ParserPtr<T> p) {
    return make_parser(CountFrom<T, 1>(p));
}


template <class T, size_t From, size_t To>
class Count : public Parser<std::vector<T>> {
public:
    Count(ParserPtr<T> parser) : m_parser(std::move(parser)) {}
    ParseResult<std::vector<T>> parse(CodeStream& stream) override {
        std::vector<T> result;
        BreakPoint point(stream);
        while (true) {
            if (auto val = m_parser->parse(stream); val) {
                result.push_back(val.get_ok());
                if (result.size() == To) {
                    point.close();
                    return result;
                }
            } else {
                if (result.size() < From)
                    return val.get_err();
                point.close();
                return result;
            }
        }
    }
private:
    ParserPtr<T> m_parser;
};

template <class T>
class Chain : public Parser<std::vector<T>> {
public:
    Chain(ParserPtr<T> val, ParserPtr<std::vector<T>> vec) : m_value(val), m_vector(vec) {}
    ParseResult<std::vector<T>> parse(CodeStream& stream) override {
        BreakPoint point(stream);
        if (auto res = m_value->parse(stream); res) {
            std::vector<T> result;
            result.push_back(res.get_ok());
            if (auto res = m_vector->parse(stream); res) {
                auto ok = res.get_ok();
                result.insert(result.end(), ok.begin(), ok.end());
                point.close();
                return result;
            } else return res;
        } else return res.get_err();
    }
private:
    ParserPtr<T> m_value;
    ParserPtr<std::vector<T>> m_vector;
};

template <class T>
ParserPtr<std::vector<T>> chain(ParserPtr<T> val, ParserPtr<std::vector<T>> vec) {
    return make_parser(Chain(val, vec));
}

template <class... Types>
class Sequence : public Parser<std::tuple<Types...>> {
public:
    Sequence(ParserPtr<Types>... parsers) : m_parsers(parsers...) {}
    ParseResult<std::tuple<Types...>> parse(CodeStream& stream) override {
        return std::apply(parse_tuple, std::tuple_cat(std::tuple<CodeStream&>(stream), m_parsers));
    }
private:
    template <class T>
    static T parse_value(CodeStream& stream, ParseError& error, ParserPtr<T> parser) {
        if (auto val = parser->parse(stream); val) {
            return val.get_ok();
        } else {
            error = val.get_err();
            throw std::runtime_error("TODO: ");
        }
    }
    static ParseResult<std::tuple<Types...>> parse_tuple(CodeStream& stream, ParserPtr<Types>... parsers) {
        ParseError error(stream);
        BreakPoint point(stream);
        try {
            std::tuple<Types...> result{parse_value(stream, error, parsers)...};
            point.close();
            return result;
        } catch (std::exception& e) {
            return error;
        }
    }
    std::tuple<ParserPtr<Types>...> m_parsers;
};

template <class... Types>
ParserPtr<std::tuple<Types...>> sequence(ParserPtr<Types>... args) {
    return make_parser(Sequence(args...));
}

template <class... Types>
class Variant : public Parser<std::variant<Types...>> {
public:
    Variant(ParserPtr<Types>... parsers) : m_parsers(parsers...) {}
    ParseResult<std::variant<Types...>> parse(CodeStream& stream) override {
        return std::apply(parse_variant, std::tuple_cat(std::tuple<CodeStream&>(stream), m_parsers));
    }
private:
    template <class T>
    static void parse_value(CodeStream& stream, ParserPtr<T> parser, ParseError& error, std::variant<Types...>& result) {
        if (auto res = parser->parse(stream)) {
            result = res.get_ok();
            throw std::runtime_error("...");
        } else {
            error = error || res.get_err();
        }
    }
    static ParseResult<std::variant<Types...>> parse_variant(CodeStream& stream, ParserPtr<Types>... parsers) {
        std::variant<Types...> result;
        try {
            ParseError error(stream);
            (parse_value(stream, parsers, error, result),...);
            return error;
        } catch (std::exception& e) {
            return result;
        }
    }
    std::tuple<ParserPtr<Types>...> m_parsers;
};

template <class... Types>
ParserPtr<std::variant<Types...>> variant(ParserPtr<Types>... parsers) {
    return make_parser(Variant(parsers...));
}

template <class T>
class Either : public Parser<T> {
public:
    Either(std::initializer_list<ParserPtr<T>> list) : m_parsers(list) {}
    ParseResult<T> parse(CodeStream& stream) override {
        ParseError err(stream);
        for (auto& parser : m_parsers) {
            BreakPoint point(stream);
            if (auto val = parser->parse(stream); val) {
                point.close();
                return val.get_ok();
            } else {
                err = err || val.get_err();
            }
        }
        return err;
    }
private:
    std::vector<ParserPtr<T>> m_parsers;
};

template <class T>
ParserPtr<T> either(std::initializer_list<ParserPtr<T>> list) {
    return make_parser(Either(list));
}

class Symbol : public Parser<char> {
public:
    Symbol(char c) : m_char(c) {}
    PResult parse(CodeStream& stream) override {
        if (auto val = stream.peek()) {
            if (*val == m_char) {
                stream.get();
                return {*val + 0};
            } else return ParseError(fmt::format("{}", m_char), *val, stream);
        } else return ParseError(fmt::format("{}", m_char), "end of file", stream);
    }
private:
    char m_char;
};

ParserPtr<char> symbol(char c) {
    return make_parser(Symbol(c));
}

template <class T>
class Maybe : public Parser<std::optional<T>> {
public:
    Maybe(ParserPtr<T> parser) : m_parser(std::move(parser)) {}
    ParseResult<std::optional<T>> parse(CodeStream& stream) override {
        BreakPoint point(stream);
        if (auto val = m_parser->parse(stream); val) {
            point.close();
            return {val.get_ok()};
        } else {
            return {std::nullopt};
        }
    }
private:
    ParserPtr<T> m_parser;
};

template <class T>
ParserPtr<std::optional<T>> maybe(ParserPtr<T> p) {
    return make_parser(Maybe(p));
}

class Symbols : public Parser<std::vector<char>> {
public:
    Symbols(std::string string) : m_string(string) {}
    PResult parse(CodeStream& stream) override {
        BreakPoint point(stream);
        if (auto res = stream.get(m_string.size()); res) {
            if (*res == m_string) {
                point.close();
                std::vector<char> result;
                result.insert(result.end(), res->begin(), res->end());
                return std::move(result);
            } else return ParseError(m_string, *res, stream);
        } else return ParseError(m_string, "end of file", stream);
    }
private:
    std::string m_string;
};

ParserPtr<std::vector<char>> symbols(std::string str) { return make_parser(Symbols(str)); }

template <class Func>
class Predicate : public Parser<char> {
public:
    Predicate(std::string name, Func func) : m_func(func), m_name(name) {}
    ParseResult<char> parse(CodeStream& stream) {
        if (auto res = stream.peek(); res) {
            if (m_func(*res)) {
                stream.get();
                return res.value() + 0;
            } else return ParseError(m_name, *res, stream);
        } else return ParseError(m_name, "end of file", stream);
    }
private:
    Func m_func;
    std::string m_name;
};

template <class Func>
ParserPtr<char> predicate(std::string name, Func func) {
    return make_parser(Predicate(name, func));
}

template <size_t... Is>
struct Select {
    template <class... Types>
    class ParserSelect : public Parser<typename selecter<Is...>::template type<Types...>> {
    public:
        using result = typename selecter<Is...>::template type<Types...>;
        ParserSelect(ParserPtr<Types>... parsers) : m_parser(parsers...) {}
        ParseResult<result> parse(CodeStream& stream) override {
            if (auto res = m_parser.parse(stream); res) {
                return selecter<Is...>::select(res.get_ok());
            } else return res.get_err();
        }
    private:
        Sequence<Types...> m_parser;
    };
    template <class... Types>
    class TupleSelect : public Parser<typename selecter<Is...>::template type<Types...>> {
    public:
        using result = typename selecter<Is...>::template type<Types...>;
        TupleSelect(ParserPtr<std::tuple<Types...>> parser) : m_parser(parser) {}
        ParseResult<result> parse(CodeStream& stream) override {
            if (auto res = m_parser->parse(stream); res) {
                return selecter<Is...>::select(res.get_ok());
            } else return res.get_err();
        }
    private:
        ParserPtr<std::tuple<Types...>> m_parser;
    };
};

template <size_t... Is>
struct parse_index {
    template <class... Types>
    static auto select(ParserPtr<Types>... parsers) {
        return make_parser(typename Select<Is...>::template ParserSelect<Types...>(parsers...));
    }
    template <class... Types>
    static auto tuple_select(ParserPtr<std::tuple<Types...>> parser) {
        return make_parser(typename Select<Is...>::template TupleSelect<Types...>(parser));
    }
};

template <class Func, class In, class Out>
concept InvokeInOut = requires (In in) {
    { Func(in) } -> std::same_as<Out>;
};

template <class T, class Func>
class Extension : public Parser<typename std::invoke_result<Func, T&>::type> {
public:
    Extension(ParserPtr<T> parser, Func&& func) : m_parser(parser), m_func(std::forward<Func>(func)) {}
    ParseResult<typename std::invoke_result<Func, const T&>::type> parse(CodeStream& stream) override {
        if (auto res = m_parser->parse(stream); res) {
            return m_func(res.get_ok());
        } else return res.get_err();
    }
private:
    ParserPtr<T> m_parser;
    Func m_func;
};

template <class T, class Func>
auto extension(ParserPtr<T> p, Func&& func) {
    return make_parser(Extension(p, std::forward<Func>(func)));
}

template <class T, class... Types>
ParserPtr<std::tuple<Types...>> delim_sequence(ParserPtr<T> delim, ParserPtr<Types>... args) {
    return sequence(parse_index<0>::select(args, delim)...);
}

template <size_t... Is>
struct delim_index {
    template <class T, class... Types>
    static auto select(ParserPtr<T> delim, ParserPtr<Types>... parsers) {
        return make_parser(typename Select<Is...>::template ParserSelect<Types...>(parse_index<0>::select(parsers, delim)...));
    }
};

template <class Base, class... Types>
class BaseEither : public Parser<std::shared_ptr<Base>> {
public:
    BaseEither(ParserPtr<Types>... parsers) : m_parsers(parsers...) {}
    ParseResult<std::shared_ptr<Base>> parse(CodeStream& stream) override {
        return std::apply(parse_tuple, std::tuple_cat(std::tuple<CodeStream&>(stream), m_parsers));
    }
private:
    template <class T>
    static void parse_value(CodeStream& stream, ParserPtr<T> parser, ParseError& error, std::shared_ptr<Base>& result) {
        BreakPoint point(stream);
        if (auto res = parser->parse(stream); res) {
            result = std::static_pointer_cast<Base>(std::make_shared<T>(res.get_ok()));
            point.close();
            throw "It's not error";
        } else {
            error = error || res.get_err();
        }
    }
    static ParseResult<std::shared_ptr<Base>> parse_tuple(CodeStream& stream, ParserPtr<Types>... parsers) {
        std::shared_ptr<Base> result;
        try {
            ParseError error(stream);
            (parse_value(stream, parsers, error, result),...);
            return error;
        } catch (const char *) {
            return result;
        }
    }
    std::tuple<ParserPtr<Types>...> m_parsers;
};

template <class Base, class... Types>
ParserPtr<std::shared_ptr<Base>> base_either(ParserPtr<Types>... parsers) {
    return make_parser(BaseEither<Base, Types...>(parsers...));
}

template <class Type, class First, class... Types>
class Constructor : public Parser<Type> {
    using PType = typename std::conditional<
        sizeof...(Types) == 0,
        First,
        std::tuple<First, Types...>>::type;
public:
    Constructor(ParserPtr<PType> parser) : m_parser(parser) {}
    ParseResult<Type> parse(CodeStream& stream) override {
        if (auto res = m_parser->parse(stream); res) {
            auto result = res.get_ok();
            if constexpr (!std::is_same<First, PType>::value) {
                return std::make_from_tuple<Type>(result);
            } else {
                return Type(result);
            }
        } else return res.get_err();
    }
private:
    ParserPtr<PType> m_parser;
};

template <class Type, class... Types>
ParserPtr<Type> construct(ParserPtr<std::tuple<Types...>> parser) {
    return make_parser(Constructor<Type, Types...>(parser));
}

template <class Type, class First>
ParserPtr<Type> construct(ParserPtr<First> parser) {
    return make_parser(Constructor<Type, First>(parser));
}


template <class T>
class Debug : public Parser<T> {
public:
    Debug(ParserPtr<T> parser, std::string name) : m_parser(parser), m_name(name) {}
    ParseResult<T> parse(CodeStream& stream) {
        auto res = m_parser->parse(stream);
        auto place = stream.place();
        char peek = '\0';
        if (auto peek_res = stream.peek(); peek_res) peek = *peek_res;
        auto fmt_str = fmt::format("Debug: {{}} ({})\n {{}} Stream current place: {}:{}:{} ({})\n",
                       m_name, stream.get_filename(), place.line, place.column, peek);
        if (res) {
            fmt::print(fmt_str, "success", "");
            return res;
        } else {
            fmt::print(fmt_str, "error", res.get_err().to_string() + "\n");
            return res;
        }
    }
private:
    ParserPtr<T> m_parser;
    std::string m_name;
};

template <class T>
ParserPtr<T> debug(ParserPtr<T> parser, std::string name = "") {
    return make_parser(Debug(parser, name));
}

template <class T>
class NotFrom : public Parser<T> {
public:
    NotFrom(ParserPtr<T> parser, std::vector<T> values) : m_parser(parser), m_values(values) {}
    ParseResult<T> parse(CodeStream& stream) {
        BreakPoint point(stream);
        if (auto res = m_parser->parse(stream); res) {
            auto result = std::find(m_values.begin(), m_values.end(), res.get_ok());
            if (result != m_values.end())
                return ParseError(fmt::format("({})", fmt::join(m_values, " | ")), fmt::format("{}", res.get_ok()), stream);
            else {
                point.close();
                return res;
            }
        } else return res;
    }
private:
    ParserPtr<T> m_parser;
    std::vector<T> m_values;
};

template <class T>
ParserPtr<T> not_from(ParserPtr<T> parser, std::vector<T> values) {
    return make_parser(NotFrom(parser, values));
}

template <class T, class Func>
class Except : public Parser<T> {
public:
    Except(ParserPtr<T> parser, std::string name, Func&& func) : m_parser(parser), m_name(name), m_func(std::forward<Func>(func)) {}
    ParseResult<T> parse(CodeStream& stream) {
        BreakPoint point(stream);
        if (auto res = m_parser->parse(stream); res) {
            if (m_func(res.get_ok())) {
                point.close();
                return res;
            } else return ParseError(m_name, "??", stream);
        } else return res;
    }
private:
    ParserPtr<T> m_parser;
    std::string m_name;
    Func m_func;
};

template <class T, class Func>
ParserPtr<T> except(ParserPtr<T> parser, std::string name, Func&& func) {
    return make_parser(Except(parser, name, std::forward<Func>(func)));
}

template <class T>
class Option : public Parser<bool> {
public:
    Option(ParserPtr<T> parser) : m_parser(parser) {}
    ParseResult<bool> parse(CodeStream& stream) {
        BreakPoint point(stream);
        if (auto res = m_parser->parse(stream); res) {
            point.close();
            return true;
        } else {
            return false;
        }
    }
private:
    ParserPtr<T> m_parser;
};

template <class T>
ParserPtr<bool> option(ParserPtr<T> parser) {
    return make_parser(Option(parser));
}
