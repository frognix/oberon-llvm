#pragma once

#include "libparser/result.hpp"
#include "symbol_token.hpp"

namespace std {

template <>
struct hash<nodes::Ident> {
    std::size_t operator()(nodes::Ident const& id) const noexcept {
        return std::hash<std::string_view>{}(std::string_view(id.value.data(), id.value.size()));
    }
};

template <>
struct hash<nodes::QualIdent> {
    std::size_t operator()(nodes::QualIdent const& id) const noexcept {
        if (!id.qual) {
            return std::hash<nodes::Ident>{}(id.ident);
        } else {
            return std::hash<nodes::Ident>{}(id.ident) ^ std::hash<nodes::Ident>{}(*id.qual);
        }
    }
};

} // namespace std

class SymbolTableI {
public:
    virtual ~SymbolTableI() {}

    virtual Maybe<SymbolToken> get_symbol(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const = 0;
    virtual Maybe<nodes::ExpressionPtr> get_value(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const = 0;

    virtual std::string to_string() const = 0;
};

class CodeSectionI {
public:
    virtual bool analyze_code(MessageContainer& messages) const = 0;
    virtual ~CodeSectionI() {}
};

struct SemanticUnitI : public SymbolTableI, public CodeSectionI {};
