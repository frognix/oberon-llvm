#pragma once

#include "symbol_table_i.hpp"
#include "nodes.hpp"
#include <unordered_map>
#include <unordered_set>

class ProcedureTable;

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

template <class T>
using SymbolMap = std::unordered_map<nodes::Ident, T>;

using SymbolSet = std::unordered_set<nodes::Ident>;

class TypeHierarchy {
public:
    TypeHierarchy() {}
    void add_extension(nodes::QualIdent extension, nodes::QualIdent base);
    bool extends(nodes::QualIdent extension, nodes::QualIdent base) const;

private:
    std::unordered_map<nodes::QualIdent, nodes::QualIdent> m_extended_types;
};

class SymbolTable : public SymbolTableI {
public:
    SymbolTable(nodes::StatementSequence b);
    Error parse(const nodes::DeclarationSequence&);

    virtual SemResult<SymbolToken> get_symbol(const nodes::QualIdent& ident, bool secretly = false) const override;
    virtual SemResult<nodes::ExpressionPtr> get_value(const nodes::QualIdent& ident, bool secretly = false) const override;
    virtual SemResult<TablePtr> get_table(const nodes::QualIdent& ident, bool secretly = false) const override;

    virtual bool type_extends_base(const nodes::Type* extension, nodes::QualIdent base) const;

    virtual Error add_symbol(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) override;
    Error add_value(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, nodes::ExpressionPtr value) override;
    Error add_table(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, TablePtr table) override;

    std::string to_string() const override;

private:
    SymbolMap<SymbolToken> symbols;
    SymbolMap<nodes::ExpressionPtr> values;
    SymbolMap<TablePtr> tables;
    TypeHierarchy type_hierarchy;
    nodes::StatementSequence body;
};
