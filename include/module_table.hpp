#pragma once

#include "symbol_table.hpp"

class ModuleTable;

using ModuleTablePtr = ModuleTable const*;

struct Import {
    nodes::Ident name;
    ModuleTablePtr module;
};

class ModuleTable : public SymbolTable {
public:
    ModuleTable(nodes::Ident name, nodes::StatementSequence body);
    bool add_imports(MessageManager&, nodes::ImportList imports);
    bool set_module(MessageManager&, ModuleTablePtr module);
    Maybe<SymbolToken> get_symbol_out(MessageManager&, const nodes::QualIdent& ident, bool secretly = false) const;
    virtual Maybe<SymbolToken> get_symbol(MessageManager&, const nodes::QualIdent& ident, bool secretly = false) const override;
    virtual Maybe<nodes::ExpressionPtr> get_value(MessageManager&, const nodes::QualIdent& ident, bool secretly = false) const override;
    virtual Maybe<TablePtr> get_table(MessageManager&, const nodes::QualIdent& ident, bool secretly = false) const override;

    virtual bool type_extends_base(const nodes::Type* extension, nodes::QualIdent base) const override;

    virtual bool add_symbol(MessageManager&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) override;
private:
    nodes::Ident m_name;
    SymbolMap<Import> m_imports;
    SymbolSet m_exports;
};
