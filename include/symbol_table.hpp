#pragma once

#include "parser.hpp"
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
    TYPE, VAR, CONST, ANY
};

struct Symbol {
    nodes::Ident name;
    SymbolGroup group;
    nodes::TypePtr type;
    size_t count;
};

template <class T>
using SymbolMap = std::unordered_map<nodes::Ident, T>;

using SymbolSet = std::unordered_set<nodes::Ident>;

class SymbolTable;

using TablePtr = std::shared_ptr<SymbolTable>;

using SymbolResult = SemResult<Symbol>;

class SymbolTable {
public:
    SymbolTable();
    virtual ~SymbolTable() {}
    Error parse(const nodes::DeclarationSequence&);

    virtual SemResult<Symbol> get_symbol(const nodes::QualIdent& ident) const;
    virtual SemResult<nodes::ExpressionPtr> get_value(const nodes::QualIdent& ident) const;
    virtual SemResult<TablePtr> get_table(const nodes::QualIdent& ident) const;

    bool has_symbol(const nodes::QualIdent& ident) const { return !get_symbol(ident); }

    virtual Error add_symbol(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type);
    Error add_value(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, nodes::ExpressionPtr value);
    Error add_table(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, TablePtr table);
private:
    SymbolMap<Symbol> symbols;
    SymbolMap<nodes::ExpressionPtr> values;
    SymbolMap<TablePtr> tables;
    nodes::StatementSequence body;
};
