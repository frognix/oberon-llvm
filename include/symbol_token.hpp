#pragma once
#include "semantic_context.hpp"
#include "node.hpp"

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
        default: throw std::runtime_error("Bad Symbol group");
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
