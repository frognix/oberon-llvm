#pragma once

#include "nodes.hpp"
#include <unordered_map>
#include <unordered_set>

class ProcedureTable;

namespace std {

template<>
struct hash<nodes::Ident> {
    std::size_t operator()(nodes::Ident const& id) const noexcept {
        return std::hash<std::string_view>{}(std::string_view(id.value.data(), id.value.size()));
    }
};

template<>
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

enum class SymbolGroup {
    TYPE, VAR, CONST, MODULE, ANY
};

inline const char* group_to_str(SymbolGroup gr) {
    switch (gr) {
        case SymbolGroup::TYPE: return "TYPE";
        case SymbolGroup::VAR: return "VAR";
        case SymbolGroup::CONST: return "CONST";
        case SymbolGroup::MODULE: return "MODULE";
        case SymbolGroup::ANY: return "ANY";
        default: throw std::runtime_error("Bad Symbol group");
    }
}

struct SymbolToken {
    nodes::QualIdent name;
    SymbolGroup group;
    nodes::TypePtr type;
    size_t count;
};

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

class SymbolTable;

using TablePtr = std::shared_ptr<SymbolTable>;

using SymbolResult = SemResult<SymbolToken>;

class TypeHierarchy {
public:
    TypeHierarchy() {}
    void add_extension(nodes::QualIdent extension, nodes::QualIdent base) {
        m_extended_types[extension] = base;
    }
    bool extends(nodes::QualIdent extension, nodes::QualIdent base) const {
        if (extension == base) return true;
        while (true) {
            if (auto res = m_extended_types.find(extension); res != m_extended_types.end()) {
                if (res->second == base) return true;
                else extension = res->second;
            } else {
                return false;
            }
        }
    }
private:
    std::unordered_map<nodes::QualIdent, nodes::QualIdent> m_extended_types;
};

class SymbolTable {
public:
    SymbolTable(nodes::StatementSequence b);
    virtual ~SymbolTable() {}
    Error parse(const nodes::DeclarationSequence&);

    virtual SemResult<SymbolToken> get_symbol(const nodes::QualIdent& ident, bool secretly = false) const;
    virtual SemResult<nodes::ExpressionPtr> get_value(const nodes::QualIdent& ident, bool secretly = false) const;
    virtual SemResult<TablePtr> get_table(const nodes::QualIdent& ident, bool secretly = false) const;

    virtual bool type_extends_base(const nodes::Type* extension, nodes::QualIdent base) const;

    bool has_symbol(const nodes::QualIdent& ident, bool secretly = false) const { return !get_symbol(ident, secretly); }

    virtual Error add_symbol(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type);
    Error add_value(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, nodes::ExpressionPtr value);
    Error add_table(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, TablePtr table);

    std::string to_string() const;
private:
    SymbolMap<SymbolToken> symbols;
    SymbolMap<nodes::ExpressionPtr> values;
    SymbolMap<TablePtr> tables;
    TypeHierarchy type_hierarchy;
    nodes::StatementSequence body;
};
