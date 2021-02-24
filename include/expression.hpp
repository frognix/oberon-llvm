#pragma once

#include "node.hpp"

namespace nodes {

struct Expression : Node {
    // virtual TypePtr get_type() const = 0;
};

using ExpressionPtr = OPtr<Expression>;

}
