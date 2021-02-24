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

template <class Out, class T>
OPtr<Out> make_optr(T&& val) {
    return std::static_pointer_cast<Out>(std::make_shared(std::forward<T>(val)));
}

struct QualIdent {
    std::string to_string() const {
        if (qual)
            return fmt::format("{}.{}", *qual, ident);
        else
            return fmt::format("{}", ident);
    }
    std::optional<Ident> qual;
    Ident ident;
};

struct IdentDef {
    Ident ident;
    bool def;
};

}
