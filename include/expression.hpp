#pragma once

#include "node.hpp"
#include "type.hpp"
#include "semantic_error.hpp"

class SymbolTable;

namespace nodes {

struct Expression : Node {
    virtual SemResult get_type(const SymbolTable&) const = 0;
    virtual SemResult eval(const SymbolTable&) const = 0;
};

using ExpressionPtr = OPtr<Expression>;

template <class Subtype, class... Args>
ExpressionPtr make_expression(Args... args) {
    return make_optr<Expression, Subtype>(args...);
}

}
