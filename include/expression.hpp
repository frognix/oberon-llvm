#pragma once

#include "node.hpp"
#include "type.hpp"
#include "type_error.hpp"

class SymbolTable;

namespace nodes {

struct Expression : Node {
    virtual TypeResult get_type(const SymbolTable&) const = 0;
};

using ExpressionPtr = OPtr<Expression>;

}
