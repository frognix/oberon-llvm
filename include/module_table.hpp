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
    static ParseReturnType parse(nodes::Ident name, std::vector<std::pair<nodes::Import, ModuleTablePtr>>, const nodes::DeclarationSequence&, nodes::StatementSequence, MessageContainer&);

    Maybe<SymbolToken> get_symbol_out(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const;
    virtual Maybe<SymbolToken> get_symbol(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const override;
    virtual Maybe<nodes::ExpressionPtr> get_value(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const override;
    virtual Maybe<TablePtr> get_table(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const override;

    virtual bool type_extends_base(const nodes::Type* extension, nodes::QualIdent base) const override;

    virtual bool add_symbol(MessageContainer&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) override;
private:
    ModuleTable() {}
    bool add_import(MessageContainer&, nodes::Import import, ModuleTablePtr module);
    nodes::Ident m_name;
    SymbolMap<Import> m_imports;
    SymbolSet m_exports;
};
