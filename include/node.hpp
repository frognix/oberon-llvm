#pragma once

#include "parser_tools.hpp"
#include "libparser/code_iterator.hpp"
#include "semantic_context.hpp"
#include "internal_error.hpp"

struct SemanticContext;

namespace nodes {

using Context = SemanticContext;

struct Node {
    CodePlace place;
    template <class T>
    const T* is() const {
        return dynamic_cast<const T*>(this);
    }
    template <class T>
    T* is() {
        return dynamic_cast<T*>(this);
    }
    virtual std::string to_string() const = 0;
    virtual ~Node() {}
};

// using Ident = std::vector<char>;

struct Ident : public Node {
    Ident() {}
    Ident(std::vector<char> v) : value(v) {}
    std::string to_string() const override { return fmt::format("{}", fmt::join(value, "")); };
    bool equal_to(const char* str) const {
        size_t i = 0;
        for (; str[i] != '\0'; i++) {
            if (value[i] != str[i])
                return false;
        }
        return value.size() == i;
    }
    bool operator==(const Ident& other) const { return value == other.value; };
    std::vector<char> value;
};

inline Ident str_to_ident(const char* str) {
    Ident ident;
    for (int i = 0; str[i] != '\0'; i++) {
        ident.value.push_back(str[i]);
    }
    return ident;
}

struct QualIdent {
    std::string to_string() const {
        if (qual)
            return fmt::format("{}.{}", *qual, ident);
        else
            return fmt::format("{}", ident);
    }
    bool operator==(const QualIdent& other) const { return ident == other.ident; }
    std::optional<Ident> qual;
    Ident ident;
};

struct IdentDef {
    Ident ident;
    bool def;
    bool operator==(const IdentDef&) const = default;
};

struct Expression;
using ExpressionPtr = OPtr<Expression>;

struct Value;
using ValuePtr = OPtr<Value>;

struct Type;
using TypePtr = OPtr<Type>;

struct Statement;
using StatementPtr = OPtr<Statement>;

struct Section;
using SectionPtr = OPtr<Section>;

} // namespace nodes
