#pragma once

#include "code_stream.hpp"
#include "parse_error.hpp"
#include "result.hpp"
#include "reassign_ptr.hpp"

template <class T> using ParseResult = Result<T, ParseError>;

template <class T>
class Parser {
public:
    using ResultType = T;
    using PResult = ParseResult<T>;
    virtual PResult parse(CodeStream& stream) const noexcept = 0;
    inline PResult operator()(CodeStream& stream) const noexcept {
        return parse(stream);
    }
    virtual ~Parser() {}
};

template <class T>
using ParserPtr = reassign_ptr<Parser<T>>;

template <class P>
ParserPtr<typename P::ResultType> make_parser(P parser) {
    return reassign_ptr<P>::template to_base<Parser<typename P::ResultType>>(std::move(parser));
    // return std::static_pointer_cast<Parser<typename P::ResultType>>(std::make_shared<P>(parser));
}

template<typename Test, template<typename...> class Ref>
struct is_specialization_of : std::false_type {};

template<template<typename...> class Ref, typename... Args>
struct is_specialization_of<Ref<Args...>, Ref>: std::true_type {};
