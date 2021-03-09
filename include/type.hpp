#pragma once

#include "node.hpp"

namespace nodes {

struct Type : Node {
    bool operator==(const Type& other) const {
        return typeid(*this) == typeid(other) && (is_equal(other) || other.is_equal(*this));
    }
    virtual bool is_equal(const Type& other) const = 0;
    virtual Maybe<TypePtr> normalize(Context&, bool normalize_pointers) = 0;
    virtual ~Type() = default;
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
