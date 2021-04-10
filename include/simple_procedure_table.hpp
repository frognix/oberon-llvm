#pragma once

#include "procedure_table.hpp"

class SimpleProcedureTable : public ProcedureTable {
public:
    static std::unique_ptr<SimpleProcedureTable> parse(const nodes::ProcedureDeclaration& proc, const SymbolTable* parent, MessageContainer& mm);
    static bool suitable_declaration(const nodes::ProcedureDeclaration&);

    const SymbolContainer& get_symbols() const override { return m_symbols; }
    SymbolContainer& get_symbols() override { return m_symbols; }
    bool can_overload(MessageContainer&, const ProcedureTable&) const override { return false; }
    bool overload(MessageContainer&, std::shared_ptr<ProcedureTable>) override;
    const SymbolTable& parent() const override { return *m_parent; }
private:
    SimpleProcedureTable() {};
    nodes::Ident m_name;
    SymbolContainer m_symbols;
    const SymbolTable* m_parent;
};
