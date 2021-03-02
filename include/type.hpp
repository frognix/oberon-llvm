#pragma once

#include "node.hpp"
#include "semantic_error.hpp"

class SymbolTable;

namespace nodes {

struct Type : Node {
    bool operator==(const Type& other) const {
        return typeid(*this) == typeid(other) && (is_equal(other) || other.is_equal(*this));
    }
    virtual bool is_equal(const Type& other) const = 0;
    virtual TypeResult normalize(const SymbolTable&, bool normalize_pointers) = 0;
    virtual ~Type() = default;
};

// using TypePtr = OPtr<Type>;

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
