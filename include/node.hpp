#pragma once

#include "plib/code_stream.hpp"
#include "parser_tools.hpp"

namespace nodes {

struct Node {
    CodePlace place;
    virtual const std::type_info& type_info() const = 0;
    virtual std::string to_string() const = 0;
    virtual ~Node() {}
};

// using Ident = std::vector<char>;

struct Ident : public Node {
    Ident() {}
    Ident(std::vector<char> v) : value(v) {}
    const std::type_info& type_info() const override { return typeid(*this); };
    std::string to_string() const override { return fmt::format("{}", value); };
    bool operator == (const Ident& other) const { return value == other.value; };
    std::vector<char> value;
};

inline Ident str_to_ident(const char * str) {
    Ident ident;
    for (int i = 0; i != '\0'; i++) {
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
    bool operator == (const QualIdent& other) const {
        return ident == other.ident;
    }
    std::optional<Ident> qual;
    Ident ident;
};

struct IdentDef {
    Ident ident;
    bool def;
    bool operator == (const IdentDef&) const = default;
};

struct Expression;
using ExpressionPtr = OPtr<Expression>;

struct Type;
using TypePtr = OPtr<Type>;

struct Statement;
using StatementPtr = OPtr<Statement>;

struct Section;
using SectionPtr = OPtr<Section>;

}
