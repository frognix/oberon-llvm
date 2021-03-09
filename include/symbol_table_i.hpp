#pragma once

#include "plib/result.hpp"
#include "semantic_context.hpp"
#include "nodes.hpp"

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

class SymbolTableI;

using TablePtr = std::shared_ptr<SymbolTableI>;


class SymbolTableI {
public:
    virtual ~SymbolTableI() {}

    virtual Maybe<SymbolToken> get_symbol(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const = 0;
    virtual Maybe<nodes::ExpressionPtr> get_value(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const = 0;
    virtual Maybe<TablePtr> get_table(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const = 0;

    virtual bool add_symbol(MessageContainer&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) = 0;
    virtual bool add_value(MessageContainer&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, nodes::ExpressionPtr value) = 0;
    virtual bool add_table(MessageContainer&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, TablePtr table) = 0;

    virtual std::string to_string() const = 0;
};

class CodeSectionI {
public:
    virtual bool analyze_code(MessageContainer& messages) const = 0;
    virtual ~CodeSectionI() {}
};

struct SemanticUnitI : public SymbolTableI, public CodeSectionI {};
