#pragma once

#include "node.hpp"

namespace nodes {

struct Type : Node {};

using TypePtr = OPtr<Type>;

using IdentList = std::vector<IdentDef>;

struct FieldList {
    IdentList list;
    TypePtr type;
};

}
