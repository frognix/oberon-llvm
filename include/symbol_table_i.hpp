#pragma once

#include "plib/result.hpp"
#include "semantic_error.hpp"

enum class SymbolGroup
{
    TYPE,
    VAR,
    CONST,
    MODULE //, ANY
};

inline const char* group_to_str(SymbolGroup gr) {
    switch (gr) {
        case SymbolGroup::TYPE: return "TYPE";
        case SymbolGroup::VAR: return "VAR";
        case SymbolGroup::CONST: return "CONST";
        case SymbolGroup::MODULE: return "MODULE";
        // case SymbolGroup::ANY: return "ANY";
        default: throw std::runtime_error("Bad Symbol group");
    }
}

struct SymbolToken {
    nodes::QualIdent name;
    SymbolGroup group;
    nodes::TypePtr type;
    size_t count;
};

using SymbolResult = SemResult<SymbolToken>;

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

class SymbolTableI;

using TablePtr = std::shared_ptr<SymbolTableI>;


class SymbolTableI {
public:
    virtual ~SymbolTableI() {}

    virtual SemResult<SymbolToken> get_symbol(const nodes::QualIdent& ident, bool secretly = false) const = 0;
    virtual SemResult<nodes::ExpressionPtr> get_value(const nodes::QualIdent& ident, bool secretly = false) const = 0;
    virtual SemResult<TablePtr> get_table(const nodes::QualIdent& ident, bool secretly = false) const = 0;

    virtual Error add_symbol(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) = 0;
    virtual Error add_value(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, nodes::ExpressionPtr value) = 0;
    virtual Error add_table(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, TablePtr table) = 0;

    virtual std::string to_string() const = 0;
};
