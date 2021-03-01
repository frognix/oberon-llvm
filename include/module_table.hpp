#pragma once

#include "symbol_table.hpp"

class ModuleTable;

using ModuleTablePtr = ModuleTable*;

struct Import {
    nodes::Ident name;
    ModuleTablePtr module;
};

class ModuleTable : public SymbolTable {
public:
    ModuleTable(nodes::Ident name, nodes::StatementSequence body);
    Error add_imports(nodes::ImportList imports);
    SemResult<SymbolToken> get_symbol_out(const nodes::QualIdent& ident) const;
    virtual SemResult<SymbolToken> get_symbol(const nodes::QualIdent& ident) const override;
    virtual SemResult<nodes::ExpressionPtr> get_value(const nodes::QualIdent& ident) const override;
    virtual SemResult<TablePtr> get_table(const nodes::QualIdent& ident) const override;

    virtual bool type_extends_base(nodes::QualIdent extension, nodes::QualIdent base) const override;

    virtual Error add_symbol(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) override;
private:
    nodes::Ident m_name;
    SymbolMap<Import> m_imports;
    SymbolSet m_exports;
};
