#pragma once

#include "parser.hpp"
#include <unordered_map>
#include <unordered_set>

class Procedure;

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

class Procedure : public SymbolTable {
public:
    Procedure(nodes::Ident name, nodes::ProcedureType type, std::optional<nodes::ExpressionPtr> ret, nodes::StatementSequence body, SymbolTable* parent)
        :  SymbolTable(), m_name(name), m_type(type), m_ret(ret), m_body(body), m_parent(parent) {}

    virtual SemResult<Symbol> get_symbol(const nodes::QualIdent& ident) const override {
        if (ident.qual) {
            return m_parent->get_symbol(ident);
        } else {
            return SymbolTable::get_symbol(ident);
        }
    }
    virtual SemResult<nodes::ExpressionPtr> get_value(const nodes::QualIdent& ident) const override {
        if (ident.qual) {
            return m_parent->get_value(ident);
        } else {
            return SymbolTable::get_value(ident);
        }
    }
    virtual SemResult<TablePtr> get_table(const nodes::QualIdent& ident) const override {
        if (ident.qual) {
            return m_parent->get_table(ident);
        } else {
            return SymbolTable::get_table(ident);
        }
    }

    virtual Error add_symbol(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) override {
        if (ident.def) {
            return ErrorBuilder(ident.ident.place).format("Cannot export local variable {}", ident.ident).build();
        } else {
            return SymbolTable::add_symbol(ident, group, type);
        }
    }
private:
    nodes::Ident m_name;
    nodes::ProcedureType m_type;
    std::optional<nodes::ExpressionPtr> m_ret;
    nodes::StatementSequence m_body;
    SymbolTable* m_parent;
};

class Module;

using ModulePtr = Module*;

struct Import {
    nodes::Ident name;
    ModulePtr module;
};

class Module : public SymbolTable {
public:
    Module(nodes::Ident name, nodes::DeclarationSequence body)
        : SymbolTable(), m_name(name), m_body(body) {}
    Error add_imports(nodes::ImportList imports) {
        for (auto& import : imports) {
            if (auto res = m_imports.find(import.name); res != m_imports.end()) {
                return ErrorBuilder(m_name.place).format("Unexpected nonunique import '{}'", import.name).build();
            }
            m_imports[import.name] = Import{import.real_name, nullptr};
        }
        return {};
    }
    SemResult<Symbol> get_symbol_out(const nodes::Ident& ident) const {
        if (m_exports.contains(ident)) {
            nodes::QualIdent name{{}, ident};
            return SymbolTable::get_symbol(name);
        } else {
            return ErrorBuilder(ident.place).format("Attempting to access a non-exported symbol {}.{}", m_name, ident).build();
        }
    }
    virtual SemResult<Symbol> get_symbol(const nodes::QualIdent& ident) const override {
        if (!ident.qual) {
            return SymbolTable::get_symbol(ident);
        } else {
            if (auto res = m_imports.find(*ident.qual); res != m_imports.end()) {
                auto& import = res->second;
                if (import.module == nullptr) {
                    Symbol symbol{ident.ident, SymbolGroup::ANY, nodes::make_type<nodes::AnyType>(), 0};
                    return symbol;
                } else {
                    return import.module->get_symbol_out(ident.ident);
                }
            } else {
                return ErrorBuilder(m_name.place).format("Import '{}' does not exist", res->second.name).build();
            }
        }
    }
    virtual SemResult<nodes::ExpressionPtr> get_value(const nodes::QualIdent& ident) const override {
    }
    virtual SemResult<TablePtr> get_table(const nodes::QualIdent& ident) const override {
    }

    virtual Error add_symbol(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) override {
        if (ident.def) {
            auto error = SymbolTable::add_symbol(ident, group, type);
            if (!error) {
                m_exports.insert(ident.ident);
            }
            return error;
        } else {
            return SymbolTable::add_symbol(ident, group, type);
        }
    }
private:
    nodes::Ident m_name;
    SymbolMap<Import> m_imports;
    SymbolSet m_exports;
    nodes::DeclarationSequence m_body;
};
