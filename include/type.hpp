#pragma once

#include "node.hpp"

namespace nodes {

struct Type : Node {
    virtual Maybe<TypePtr> normalize(Context&, bool normalize_pointers) = 0;
    virtual ~Type() = default;
};

enum class BaseType {
    NONE,
    BOOL,
    CHAR,
    INTEGER,
    REAL,
    BYTE,
    SET,
    NIL
};

bool same_types(Context& context, const Type& left, const Type& right);
bool equal_types(Context& context, const Type& left, const Type& right);
bool assignment_compatible_types(Context& context, const Type& var, const Type& expr);
bool array_compatible(Context& context, const Type& left, const Type& right);

using IdentList = std::vector<IdentDef>;

struct FieldList {
    IdentList list;
    TypePtr type;
    bool operator==(const FieldList&) const = default;
};

template <class Subtype, class... Args>
TypePtr make_type(Args... args) {
    return make_optr<Type, Subtype>(args...);
}

} // namespace nodes
