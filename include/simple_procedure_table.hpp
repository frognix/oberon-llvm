#pragma once

#include "procedure_table.hpp"

class SimpleProcedureTable : public IProcedureTable {
public:
    static std::unique_ptr<SimpleProcedureTable> parse(const nodes::ProcedureDeclaration& proc, const nodes::ProcedureType& type, const SymbolTableI* parent, MessageContainer& mm);

    const SymbolTable& get_symbols() const override { return symbols; }
    bool can_overload(const IProcedureTable&) const override { return false; }
    bool overload(MessageContainer&, std::shared_ptr<IProcedureTable>) override;
    const SymbolTableI& parent() const override { return *m_parent; }
private:
    SimpleProcedureTable() {};
    nodes::Ident m_name;
    SymbolTable symbols;
    const SymbolTableI* m_parent;
};
