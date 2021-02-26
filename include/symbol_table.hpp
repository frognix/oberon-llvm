#pragma once

#include "parser.hpp"
#include <unordered_map>
#include <unordered_set>

class Procedure;

namespace std {

template<>
struct hash<nodes::Ident> {
    std::size_t operator()(nodes::Ident const& id) const noexcept {
        return std::hash<std::string_view>{}(std::string_view(id.data(), id.size()));
    }
};

} // namespace std

class Module;

struct Import {
    nodes::Ident name;
    std::unique_ptr<Module> table;
};

enum class SymbolGroup {
    TYPE, VAR, CONST
};

struct Symbol {
    nodes::Ident name;
    SymbolGroup group;
    nodes::TypePtr type;
    CodePlace place;
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

    virtual SemResult<Symbol> get_symbol(const nodes::QualIdent& ident, CodePlace place) const;
    virtual SemResult<nodes::ExpressionPtr> get_value(const nodes::QualIdent& ident, CodePlace place) const;
    virtual SemResult<TablePtr> get_table(const nodes::QualIdent& ident, CodePlace place) const;

    bool has_symbol(const nodes::QualIdent& ident) const { return !get_symbol(ident, CodePlace()); }

    virtual Error add_symbol(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, CodePlace place);
    Error add_value(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, CodePlace place, nodes::ExpressionPtr value);
    Error add_table(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, CodePlace place, TablePtr table);
private:
    SymbolMap<Symbol> symbols;
    SymbolMap<nodes::ExpressionPtr> values;
    SymbolMap<TablePtr> tables;
    nodes::StatementSequence body;
};

class Procedure : public SymbolTable {
public:
    Procedure(nodes::Ident name, nodes::ProcedureType type, std::optional<nodes::ExpressionPtr> ret, nodes::StatementSequence body, SymbolTable* parent)
        :  SymbolTable(), m_name(name), m_type(type), m_ret(ret), m_body(body), m_parent(parent) {}

    virtual SemResult<Symbol> get_symbol(const nodes::QualIdent& ident, CodePlace place) const override {
        if (ident.qual) {
            return m_parent->get_symbol(ident, place);
        } else {
            return SymbolTable::get_symbol(ident, place);
        }
    }
    virtual SemResult<nodes::ExpressionPtr> get_value(const nodes::QualIdent& ident, CodePlace place) const override {
        if (ident.qual) {
            return m_parent->get_value(ident, place);
        } else {
            return SymbolTable::get_value(ident, place);
        }
    }
    virtual SemResult<TablePtr> get_table(const nodes::QualIdent& ident, CodePlace place) const override {
        if (ident.qual) {
            return m_parent->get_table(ident, place);
        } else {
            return SymbolTable::get_table(ident, place);
        }
    }

    virtual Error add_symbol(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, CodePlace place) override {
        if (ident.def) {
            return ErrorBuilder(place).format("Cannot export local variable {}", ident.ident).build();
        } else {
            return SymbolTable::add_symbol(ident, group, type, place);
        }
    }
private:
    nodes::Ident m_name;
    nodes::ProcedureType m_type;
    std::optional<nodes::ExpressionPtr> m_ret;
    nodes::StatementSequence m_body;
    SymbolTable* m_parent;
};

class Module {};
