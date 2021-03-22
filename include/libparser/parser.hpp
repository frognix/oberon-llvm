#pragma once

#include "code_iterator.hpp"
#include "graph_ptr.hpp"
#include <optional>

template <class T>
using ParseResult = std::optional<T>;

const auto parse_error = std::nullopt;

template <class T>
class Parser {
  public:
    using ResultType = T;
    using PResult = ParseResult<T>;
    virtual PResult parse(CodeIterator& stream) const noexcept = 0;
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
