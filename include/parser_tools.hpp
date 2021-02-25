#pragma once

#include <vector>
#include <string_view>
#include <memory>

namespace nodes {

using Real = double;
using Integer = int;
using Ident = std::vector<char>;

template <class T>
using OPtr = std::shared_ptr<T>;

template <class Out, class T, class... Args>
OPtr<Out> make_optr(Args&&... val) {
    return std::static_pointer_cast<Out>(std::make_shared<T>(val...));
}

struct QualIdent {
    std::string to_string() const {
        if (qual)
            return fmt::format("{}.{}", *qual, ident);
        else
            return fmt::format("{}", ident);
    }
    bool operator == (const QualIdent& other) const {
        return ident == other.ident;
    }
    std::optional<Ident> qual;
    Ident ident;
};

struct IdentDef {
    Ident ident;
    bool def;
    bool operator == (const IdentDef&) const = default;
};

}
