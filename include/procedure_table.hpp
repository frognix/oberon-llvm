#pragma once

#include "symbol_table.hpp"

class ProcedureTable : public SymbolTable {
public:
    ProcedureTable(nodes::Ident name, nodes::ProcedureType type, std::optional<nodes::ExpressionPtr> ret,
                   nodes::StatementSequence body, SymbolTable* parent);
    virtual SemResult<SymbolToken> get_symbol(const nodes::QualIdent& ident) const override;
    virtual SemResult<nodes::ExpressionPtr> get_value(const nodes::QualIdent& ident) const override;
    virtual SemResult<TablePtr> get_table(const nodes::QualIdent& ident) const override;

    virtual bool type_extends_base(nodes::QualIdent extension, nodes::QualIdent base) const override;

    virtual Error add_symbol(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) override;

private:
    nodes::Ident m_name;
    nodes::ProcedureType m_type;
    std::optional<nodes::ExpressionPtr> m_ret;
    SymbolTable* m_parent;
};
