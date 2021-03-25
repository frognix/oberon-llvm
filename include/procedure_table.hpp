#pragma once

#include "symbol_table.hpp"

class ProcedureTable : public SemanticUnitI {
public:
    static std::unique_ptr<ProcedureTable> parse(const nodes::ProcedureDeclaration& proc, const nodes::ProcedureType& type, const SymbolTableI* parent, MessageContainer& mm);
    virtual Maybe<SymbolToken> get_symbol(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const override;
    virtual Maybe<nodes::ValuePtr> get_value(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const override;

    virtual std::string to_string() const override;
    bool analyze_code(MessageContainer& messages) const override;
private:
    ProcedureTable() {};
    nodes::Ident m_name;
    SymbolTable symbols;
    const SymbolTableI* m_parent;
};
