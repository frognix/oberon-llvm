#pragma once
#include "internal_error.hpp"
#include "semantic_context.hpp"
#include "node.hpp"
#include <unordered_map>
#include <unordered_set>

enum class SymbolGroup {
    TYPE,
    VAR,
    CONST,
    MODULE
};

inline const char* group_to_str(SymbolGroup gr) {
    switch (gr) {
        case SymbolGroup::TYPE: return "TYPE";
        case SymbolGroup::VAR: return "VAR";
        case SymbolGroup::CONST: return "CONST";
        case SymbolGroup::MODULE: return "MODULE";
        default: internal::compiler_error("Unexpected symbol group");
    }
}

struct SymbolToken {
    nodes::QualIdent name;
    SymbolGroup group;
    nodes::TypePtr type;
    size_t count;
};

using SymbolResult = Maybe<SymbolToken>;

template <>
struct fmt::formatter<SymbolToken> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(SymbolToken const& sym, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{{{}, {}, {}, {}}}", sym.name, group_to_str(sym.group), sym.type, sym.count);
    }
};

template <class T>
using SymbolMap = std::unordered_map<nodes::Ident, T>;

using SymbolSet = std::unordered_set<nodes::Ident>;
