#pragma once

#include "node.hpp"

namespace nodes {

struct Type : Node {
    bool operator==(const Type& other) const {
        return typeid(*this) == typeid(other) && is_equal(other);
    }
    virtual bool is_equal(const Type& other) const = 0;
};

using TypePtr = OPtr<Type>;

using IdentList = std::vector<IdentDef>;

struct FieldList {
    IdentList list;
    TypePtr type;
    bool operator == (const FieldList&) const = default;
};

template <class Subtype, class... Args>
TypePtr make_type(Args... args) {
    return make_optr<Type, Subtype>(args...);
}

}
