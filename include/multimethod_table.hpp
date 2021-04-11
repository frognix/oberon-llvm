#pragma once

#include "internal_error.hpp"
#include "message_container.hpp"
#include "procedure_table.hpp"
#include "symbol_container.hpp"
#include "symbol_table.hpp"

class MultimethodInstanceTable;

class MultimethodTable : public ProcedureTable {
public:
    static std::unique_ptr<MultimethodTable> parse(const nodes::ProcedureDeclaration& proc, const SymbolTable* parent, MessageContainer& mm);
    static bool suitable_declaration(const nodes::ProcedureDeclaration&);

    const SymbolContainer& get_symbols() const override { return m_symbols; }
    SymbolContainer& get_symbols() override { return m_symbols; }
    bool can_overload(MessageContainer&, const ProcedureTable&) const override;
    bool overload(MessageContainer&, std::shared_ptr<ProcedureTable>) override;
    const SymbolTable& parent() const override { return *m_parent; }

    bool analyze_code(MessageContainer& messages) const override;

    bool instance_compatible(MessageContainer& messages, bool with_messages, const ProcedureTable& instance) const;
private:
    MultimethodTable() {}
    nodes::Ident m_name;
    nodes::ProcedureType m_type;
    SymbolContainer m_symbols;
    std::vector<std::shared_ptr<MultimethodInstanceTable>> m_instances;
    const SymbolTable* m_parent;
};

class MultimethodInstanceTable : public ProcedureTable {
public:
    static std::unique_ptr<MultimethodInstanceTable> parse(const nodes::ProcedureDeclaration& proc, const SymbolTable* parent, MessageContainer& mm);
    static bool suitable_declaration(const nodes::ProcedureDeclaration&);

    const SymbolContainer& get_symbols() const override { return m_symbols; }
    SymbolContainer& get_symbols() override { return m_symbols; }
    bool can_overload(MessageContainer&, const ProcedureTable&) const override { return false; }
    bool overload(MessageContainer& messages, std::shared_ptr<ProcedureTable>) override {
        messages.addErr(m_name.place, "Attempt to overoload multimethod instance: {}", m_name);
        return false;
    }
    const SymbolTable& parent() const override { return *m_parent; }
private:
    friend class MultimethodTable;
    MultimethodInstanceTable() {}
    nodes::Ident m_name;
    nodes::ProcedureType m_type;
    SymbolContainer m_symbols;
    const SymbolTable* m_parent;
};
