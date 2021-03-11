#pragma once

#include "code_stream.hpp"
#include "parse_error.hpp"
// #include "reassign_ptr.hpp"
#include "graph_ptr.hpp"
#include "result.hpp"

template <class T>
using ParseResult = Result<T, ParseError>;

template <class T>
class Parser {
  public:
    using ResultType = T;
    using PResult = ParseResult<T>;
    virtual PResult parse(CodeStream& stream) const noexcept = 0;
    virtual ~Parser() {}
};

template <class T>
using ParserPtr = GraphPtr<Parser<T>>;

template <class T>
using ParserLinker = GraphLinker<Parser<T>>;

template <class P>
ParserPtr<typename P::ResultType> make_parser(P parser) {
    return GraphPtr<Parser<typename P::ResultType>>::create_with_cast(std::move(parser));
}

template <typename Test, template <typename...> class Ref>
struct is_specialization_of : std::false_type {};

template <template <typename...> class Ref, typename... Args>
struct is_specialization_of<Ref<Args...>, Ref> : std::true_type {};
