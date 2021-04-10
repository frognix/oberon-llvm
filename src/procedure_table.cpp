#include "multimethod_table.hpp"
#include "section_nodes.hpp"
#include "simple_procedure_table.hpp"
#include "type_nodes.hpp"

std::unique_ptr<ProcedureTable> build_procedure_table(const nodes::ProcedureDeclaration& proc,
                                                       const SymbolTable* parent, MessageContainer& messages) {
    if (MultimethodTable::suitable_declaration(proc))
        return std::unique_ptr<ProcedureTable>(MultimethodTable::parse(proc, parent, messages).release());
    if (MultimethodInstanceTable::suitable_declaration(proc))
        return std::unique_ptr<ProcedureTable>(MultimethodInstanceTable::parse(proc, parent, messages).release());
    if (SimpleProcedureTable::suitable_declaration(proc))
        return std::unique_ptr<ProcedureTable>(SimpleProcedureTable::parse(proc, parent, messages).release());
    messages.addErr(proc.name.ident.place, "Unexpected procedure structure");
    return nullptr;
}

bool parseProcedureType(MessageContainer& messages, SymbolTable& table, const nodes::ProcedureType& type) {
    for (auto& section : type.params.common) {
        auto group = section.var ? SymbolGroup::VAR : SymbolGroup::CONST;
        auto res = table.get_symbols().add_symbol(messages, nodes::IdentDef{section.ident, false}, group, section.type);
        if (!res) return false;
    }
    for (auto& section : type.params.formal) {
        auto group = section.var ? SymbolGroup::VAR : SymbolGroup::CONST;
        auto res = table.get_symbols().add_symbol(messages, nodes::IdentDef{section.ident, false}, group, section.type);
        if (!res) return false;
    }
    return true;
}
