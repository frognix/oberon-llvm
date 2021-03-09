#pragma once

#include "symbol_table.hpp"

class ProcedureTable : public SymbolTable {
public:
    static ParseReturnType parse(nodes::Ident, nodes::ProcedureType,
                                          std::optional<nodes::ExpressionPtr>,
                                          nodes::StatementSequence, nodes::DeclarationSequence,
                                          SymbolTable*, MessageContainer&);
    virtual Maybe<SymbolToken> get_symbol(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const override;
    virtual Maybe<nodes::ExpressionPtr> get_value(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const override;
    virtual Maybe<TablePtr> get_table(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const override;

    virtual bool type_extends_base(const nodes::Type* extension, nodes::QualIdent base) const override;

    virtual bool add_symbol(MessageContainer&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) override;

private:
    ProcedureTable() {}
    nodes::Ident m_name;
    nodes::ProcedureType m_type;
    std::optional<nodes::ExpressionPtr> m_ret;
    SymbolTable* m_parent;
};
