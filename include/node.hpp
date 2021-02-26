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

struct Expression;
using ExpressionPtr = OPtr<Expression>;

struct Type;
using TypePtr = OPtr<Type>;

struct Statement;
using StatementPtr = OPtr<Statement>;

struct Section;
using SectionPtr = OPtr<Section>;

}
