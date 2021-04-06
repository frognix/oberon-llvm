#pragma once

#include "procedure_table.hpp"

class SimpleProcedureTable : public ProcedureTable {
public:
    static std::unique_ptr<SimpleProcedureTable> parse(const nodes::ProcedureDeclaration& proc, const nodes::ProcedureType& type, const SymbolTable* parent, MessageContainer& mm);

    const SymbolContainer& get_symbols() const override { return symbols; }
    bool can_overload(const ProcedureTable&) const override { return false; }
    bool overload(MessageContainer&, std::shared_ptr<ProcedureTable>) override;
    const SymbolTable& parent() const override { return *m_parent; }
private:
    SimpleProcedureTable() {};
    nodes::Ident m_name;
    SymbolContainer symbols;
    const SymbolTable* m_parent;
};
