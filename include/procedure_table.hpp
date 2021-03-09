#pragma once

#include "symbol_table.hpp"

class ProcedureTable : public SymbolTable {
public:
    ProcedureTable(nodes::Ident name, nodes::ProcedureType type, std::optional<nodes::ExpressionPtr> ret,
                   nodes::StatementSequence body, SymbolTable* parent);
    virtual Maybe<SymbolToken> get_symbol(MessageManager&, const nodes::QualIdent& ident, bool secretly = false) const override;
    virtual Maybe<nodes::ExpressionPtr> get_value(MessageManager&, const nodes::QualIdent& ident, bool secretly = false) const override;
    virtual Maybe<TablePtr> get_table(MessageManager&, const nodes::QualIdent& ident, bool secretly = false) const override;

    virtual bool type_extends_base(const nodes::Type* extension, nodes::QualIdent base) const override;

    virtual bool add_symbol(MessageManager&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) override;

private:
    nodes::Ident m_name;
    nodes::ProcedureType m_type;
    std::optional<nodes::ExpressionPtr> m_ret;
    SymbolTable* m_parent;
};
