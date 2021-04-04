#pragma once

#include "node.hpp"

namespace nodes {

struct Type : Node {
    virtual Maybe<TypePtr> normalize(Context&, bool normalize_pointers) const = 0;
    virtual bool same(Context& context, const Type& other) const = 0;
    virtual bool equal(Context& context, const Type& other) const = 0;
    virtual bool assignment_compatible(Context& context, const Type& expr) = 0;
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
