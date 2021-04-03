#pragma once

#include "break_point.hpp"
#include "parser.hpp"
#include "code_iterator.hpp"
#include "selector.hpp"
#include <optional>
#include <variant>

template <class T, size_t From>
class CountFrom : public Parser<std::vector<T>> {
  public:
    CountFrom(ParserPtr<T> parser) : m_parser(std::move(parser)) {}
    ParseResult<std::vector<T>> parse(CodeIterator& stream) const noexcept override {
        std::vector<T> result;
        BreakPoint point(stream);
        while (true) {
            if (auto val = m_parser->parse(stream); val) {
                result.push_back(val.value());
            } else {
                if (result.size() < From) {
                    return parse_error;
                }
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
    ParseResult<std::vector<T>> parse(CodeIterator& stream) const noexcept override {
        std::vector<T> result;
        BreakPoint point(stream);
        while (true) {
            if (auto val = m_parser->parse(stream); val) {
                result.push_back(val.value());
                if (result.size() == To) {
                    point.close();
                    return result;
                }
            } else {
                if (result.size() < From) {
                    return parse_error;
                }
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
    ParseResult<std::vector<T>> parse(CodeIterator& stream) const noexcept override {
        BreakPoint point(stream);
        if (auto res = m_value->parse(stream); res) {
            std::vector<T> result;
            result.push_back(res.value());
            if (auto res = m_vector->parse(stream); res) {
                auto ok = res.value();
                result.insert(result.end(), ok.begin(), ok.end());
                point.close();
                return result;
            }
        }
        return parse_error;
    }

  private:
    ParserPtr<T> m_value;
    ParserPtr<std::vector<T>> m_vector;
};

template <class T>
inline ParserPtr<std::vector<T>> chain(ParserPtr<T> val, ParserPtr<std::vector<T>> vec) {
    return make_parser(Chain(val, vec));
}

template <class... Types>
class Sequence : public Parser<std::tuple<Types...>> {
public:
    Sequence(ParserPtr<Types>... parsers) : m_parsers(parsers...) {}
    ParseResult<std::tuple<Types...>> parse(CodeIterator& stream) const noexcept override {
        auto start_parse = [&stream](const auto&... parsers) { return parse_tuple(stream, parsers...); };
        return std::apply(start_parse, m_parsers);
    }

private:
    inline static ParseResult<std::tuple<Types...>> parse_tuple(CodeIterator& stream,
                                                         const ParserPtr<Types>&... parsers) noexcept {
        bool error = false;
        BreakPoint point(stream);
        auto func = [&stream] <class T> (bool& error, const ParserPtr<T>& parser) {
            if (!error) {
                if (auto val = parser->parse(stream); val)
                    return val.value();
                error = true;
            }
            return T{};
        };
        std::tuple<Types...> result{func(error, parsers)...};
        if (error) return parse_error;
        point.close();
        return result;
    }
    std::tuple<ParserPtr<Types>...> m_parsers;
};

template <class... Types>
inline ParserPtr<std::tuple<Types...>> sequence(ParserPtr<Types>... args) {
    return make_parser(Sequence(args...));
}

template <class... Types>
class Variant : public Parser<std::variant<Types...>> {
public:
    Variant(ParserPtr<Types>... parsers) : m_parsers(parsers...) {}
    ParseResult<std::variant<Types...>> parse(CodeIterator& stream) const noexcept override {
        auto start_parse = [&stream](const auto&... parsers) { return parse_variant(stream, parsers...); };
        return std::apply(start_parse, m_parsers);
    }

private:
    inline static auto parse_variant(CodeIterator& stream, const ParserPtr<Types>&... parsers) noexcept {
        auto func = [&stream](const auto& parser) {
            BreakPoint point(stream);
            auto res = parser->parse(stream);
            if (res) point.close();
            return res;
        };
        ParseResult<std::variant<Types...>> result;
        ((result = func(parsers)) || ...);
        return result;
    }
    std::tuple<ParserPtr<Types>...> m_parsers;
};

template <class... Types>
inline ParserPtr<std::variant<Types...>> variant(ParserPtr<Types>... parsers) {
    return make_parser(Variant(parsers...));
}

template <class T>
class Either : public Parser<T> {
  public:
    Either(std::initializer_list<ParserPtr<T>> list) : m_parsers(list) {}
    ParseResult<T> parse(CodeIterator& stream) const noexcept override {
        for (auto parser : m_parsers) {
            BreakPoint point(stream);
            if (auto val = parser->parse(stream); val) {
                point.close();
                return val.value();
            } else {
                if (stream.has_undroppable_error())
                    return parse_error;
            }
        }
        return parse_error;
    }

  private:
    std::vector<ParserPtr<T>> m_parsers;
};

template <class T>
inline ParserPtr<T> either(std::initializer_list<ParserPtr<T>> list) {
    return make_parser(Either(list));
}

class Symbol : public Parser<char> {
  public:
    Symbol(char c) : m_char(c) {}
    PResult parse(CodeIterator& stream) const noexcept override {
        if (auto val = stream.peek(); val) {
            if (val.value() == m_char) {
                stream.get();
                return val.value();
            }
        }
        stream.error_expected(fmt::format("{}", m_char));
        return parse_error;
    }

  private:
    char m_char;
};

inline ParserPtr<char> symbol(char c) {
    return make_parser(Symbol(c));
}

template <class T>
class MaybeParser : public Parser<std::optional<T>> {
  public:
    MaybeParser(ParserPtr<T> parser) : m_parser(std::move(parser)) {}
    ParseResult<std::optional<T>> parse(CodeIterator& stream) const noexcept override {
        BreakPoint point(stream);
        if (auto val = m_parser->parse(stream); val) {
            point.close();
            return ParseResult{val};
        } else {
            if (stream.has_undroppable_error())
                return parse_error;
            auto res = std::make_optional<std::optional<T>>();
            res.value() = std::nullopt;
            return res;
        }
    }

  private:
    ParserPtr<T> m_parser;
};

template <class T>
inline ParserPtr<std::optional<T>> maybe(ParserPtr<T> p) {
    return make_parser(MaybeParser(p));
}

class Symbols : public Parser<std::string_view> {
  public:
    Symbols(const char* string) : m_string(string) {}
    PResult parse(CodeIterator& stream) const noexcept override {
        BreakPoint point(stream);
        if (auto res = stream.get(m_string.size()); res) {
            if (res.value() == m_string) {
                point.close();
                return res.value();
            }
        }
        stream.error_expected(fmt::format("{}", m_string));
        return parse_error;
    }

  private:
    std::string_view m_string;
};

inline ParserPtr<std::string_view> symbols(const char* str) {
    return make_parser(Symbols(str));
}

template <class Func>
class Predicate : public Parser<char> {
  public:
    Predicate(std::string name, Func func) : m_func(func), m_name(name) {}
    ParseResult<char> parse(CodeIterator& stream) const noexcept override {
        if (auto res = stream.peek(); res) {
            if (m_func(res.value())) {
                stream.get();
                return res.value();
            }
        }
        stream.error_expected(m_name);
        return parse_error;
    }

  private:
    Func m_func;
    std::string m_name;
};

template <class Func>
inline ParserPtr<char> predicate(std::string name, Func func) {
    return make_parser(Predicate(name, func));
}

template <size_t Idx, size_t... Is>
struct Select {
    template <class... Types>
    using result_type = tuple_select_type<std::tuple<Types...>, Idx, Is...>;

    template <template <typename...> class BaseType, class... Types>
    class ParserSelect : public Parser<result_type<Types...>> {
      public:
        ParserSelect(ParserPtr<Types>... parsers) : m_parser(parsers...) {}
        ParseResult<result_type<Types...>> parse(CodeIterator& stream) const noexcept override {
            if (auto res = m_parser.parse(stream); res) {
                return tuple_select<Idx, Is...>(res.value());
            } else
                return parse_error;
        }

      private:
        BaseType<Types...> m_parser;
    };
    template <class... Types>
    class TupleSelect : public Parser<result_type<Types...>> {
      public:
        TupleSelect(ParserPtr<std::tuple<Types...>> parser) : m_parser(parser) {}
        ParseResult<result_type<Types...>> parse(CodeIterator& stream) const noexcept override {
            if (auto res = m_parser->parse(stream); res) {
                return tuple_select<Idx, Is...>(res.value());
            } else
                return parse_error;
        }

      private:
        ParserPtr<std::tuple<Types...>> m_parser;
    };
};

template <class... Types>
ParserPtr<std::tuple<Types...>> all_or_nothing(ParserPtr<Types>... parsers) {
    return make_parser(AllOrNothing(parsers...));
}

template <size_t... Is>
struct parse_index {
    template <class... Types>
    static auto select(ParserPtr<Types>... parsers) {
        return make_parser(typename Select<Is...>::template ParserSelect<Sequence, Types...>(parsers...));
    }
    template <class... Types>
    static auto tuple_select(ParserPtr<std::tuple<Types...>> parser) {
        return make_parser(typename Select<Is...>::template TupleSelect<Types...>(parser));
    }
};

template <class T, class Func>
class Extension : public Parser<typename std::invoke_result<Func, T&>::type> {
  public:
    Extension(ParserPtr<T> parser, Func&& func) : m_parser(parser), m_func(std::forward<Func>(func)) {}
    ParseResult<typename std::invoke_result<Func, const T&>::type> parse(CodeIterator& stream) const noexcept override {
        if (auto res = m_parser->parse(stream); res) {
            return m_func(res.value());
        } else
            return parse_error;
    }

  private:
    ParserPtr<T> m_parser;
    Func m_func;
};

template <class T, class Func>
inline auto extension(ParserPtr<T> p, Func&& func) {
    return make_parser(Extension(p, std::forward<Func>(func)));
}

template <class T, class... Types>
inline ParserPtr<std::tuple<Types...>> delim_sequence(ParserPtr<T> delim, ParserPtr<Types>... args) {
    return sequence(parse_index<0>::select(args, delim)...);
}

template <size_t... Is>
struct delim_index {
    template <class T, class... Types>
    static auto select(ParserPtr<T> delim, ParserPtr<Types>... parsers) {
        return make_parser(typename Select<Is...>::template ParserSelect<Sequence, Types...>(
            parse_index<0>::select(parsers, delim)...));
    }
};

template <class Base, class... Types>
class BaseEither : public Parser<std::shared_ptr<Base>> {
  public:
    BaseEither(ParserPtr<Types>... parsers) : m_parsers(parsers...) {}
    ParseResult<std::shared_ptr<Base>> parse(CodeIterator& stream) const noexcept override {
        auto start_parse = [&stream](const ParserPtr<Types>&... parsers) { return parse_tuple(stream, parsers...); };
        return std::apply(start_parse, m_parsers);
    }

  private:
    inline static auto parse_tuple(CodeIterator& stream, const ParserPtr<Types>&... parsers) noexcept {
        auto func = [&stream] <class T> (const ParserPtr<T>& parser) {
            BreakPoint point(stream);
            auto res = parser->parse(stream);
            if (!res) return ParseResult<std::shared_ptr<Base>>{parse_error};
            point.close();
            return ParseResult{std::static_pointer_cast<Base>(std::make_shared<T>(res.value()))};
        };
        ParseResult<std::shared_ptr<Base>> result;
        ((result = func(parsers)) || ...);
        return result;
    }
    std::tuple<ParserPtr<Types>...> m_parsers;
};

template <class Base, class... Types>
inline ParserPtr<std::shared_ptr<Base>> base_either(ParserPtr<Types>... parsers) {
    return make_parser(BaseEither<Base, Types...>(parsers...));
}

template <class Type, class First, class... Types>
class Constructor : public Parser<Type> {
    using PType = typename std::conditional<sizeof...(Types) == 0, First, std::tuple<First, Types...>>::type;

  public:
    Constructor(ParserPtr<PType> parser) : m_parser(parser) {}
    ParseResult<Type> parse(CodeIterator& stream) const noexcept override {
        if (auto res = m_parser->parse(stream); res) {
            auto result = res.value();
            if constexpr (!std::is_same<First, PType>::value) {
                return std::make_from_tuple<Type>(result);
            } else {
                return Type(result);
            }
        } else
            return parse_error;
    }

  private:
    ParserPtr<PType> m_parser;
};

template <class Type, class... Types>
inline ParserPtr<Type> construct(ParserPtr<std::tuple<Types...>> parser) {
    return make_parser(Constructor<Type, Types...>(parser));
}

template <class Type, class First>
inline ParserPtr<Type> construct(ParserPtr<First> parser) {
    return make_parser(Constructor<Type, First>(parser));
}

template <class T>
class NotFrom : public Parser<T> {
  public:
    NotFrom(ParserPtr<T> parser, std::vector<T> values) : m_parser(parser), m_values(values) {}
    ParseResult<T> parse(CodeIterator& stream) const noexcept override {
        BreakPoint point(stream);
        if (auto res = m_parser->parse(stream); res) {
            auto result = std::find(m_values.begin(), m_values.end(), res.value());
            if (result != m_values.end()) {
                stream.error_expected(fmt::format("not one of {}", m_values));
            } else {
                point.close();
                return res;
            }
        }
        return parse_error;
    }

  private:
    ParserPtr<T> m_parser;
    std::vector<T> m_values;
};

template <class T>
inline ParserPtr<T> not_from(ParserPtr<T> parser, std::vector<T> values) {
    return make_parser(NotFrom(parser, values));
}

template <class T, class Func>
class Except : public Parser<T> {
  public:
    Except(ParserPtr<T> parser, std::string name, Func&& func)
        : m_parser(parser), m_name(name), m_func(std::forward<Func>(func)) {}
    ParseResult<T> parse(CodeIterator& stream) const noexcept override {
        BreakPoint point(stream);
        if (auto res = m_parser->parse(stream); res) {
            if (m_func(res.value())) {
                point.close();
                return res;
            } else {
                stream.error_expected(fmt::format("{}", m_name));
            }
        }
        return parse_error;
    }

  private:
    ParserPtr<T> m_parser;
    std::string m_name;
    Func m_func;
};

template <class T, class Func>
inline ParserPtr<T> except(ParserPtr<T> parser, std::string name, Func&& func) {
    return make_parser(Except(parser, name, std::forward<Func>(func)));
}

template <class T>
class Option : public Parser<bool> {
  public:
    Option(ParserPtr<T> parser) : m_parser(parser) {}
    ParseResult<bool> parse(CodeIterator& stream) const noexcept override {
        BreakPoint point(stream);
        if (auto res = m_parser->parse(stream); res) {
            point.close();
            return ParseResult{true};
        } else {
            if (stream.has_undroppable_error())
                return parse_error;
            return ParseResult{false};
        }
    }

  private:
    ParserPtr<T> m_parser;
};

template <class T>
inline ParserPtr<bool> option(ParserPtr<T> parser) {
    return make_parser(Option(parser));
}

template <class T>
class NoReturn : public Parser<T> {
  public:
    NoReturn(ParserPtr<T> parser) : m_parser(parser) {}
    ParseResult<T> parse(CodeIterator& stream) const noexcept override {
        if (auto res = m_parser->parse(stream); res) {
            stream.set_no_return_point();
            return res;
        } else
            return res;
    }

  private:
    ParserPtr<T> m_parser;
};

template <class T>
inline auto no_return(ParserPtr<T> parser) {
    return make_parser(NoReturn(parser));
}

template <class T>
class Debug : public Parser<T> {
public:
    Debug(ParserPtr<T> parser, std::string message) : m_parser(parser), m_message(message) {}
    ParseResult<T> parse(CodeIterator& stream) const noexcept override {
        auto res = m_parser->parse(stream);
        std::string status;
        if (res) {
            status = fmt::format("Success(undroppable:{})", stream.has_undroppable_error());
        } else {
            status = fmt::format("Error(undroppable:{}):\"{}\"", stream.has_undroppable_error(), stream.format_error());
        }
        fmt::print("\nDebug: {}, {}, '{}', \"{}\"\n", status, stream.place().get_index(), *stream.peek(), m_message);
        return res;
    }
private:
    ParserPtr<T> m_parser;
    std::string m_message;
};

template <class T>
inline auto debug(ParserPtr<T> parser, std::string message) {
    return make_parser(Debug(parser, message));
}
