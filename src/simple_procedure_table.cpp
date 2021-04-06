#include "simple_procedure_table.hpp"

std::unique_ptr<SimpleProcedureTable> SimpleProcedureTable::parse(const nodes::ProcedureDeclaration& proc, const nodes::ProcedureType& type, const SymbolTable* parent, MessageContainer& messages) {
    std::unique_ptr<SimpleProcedureTable> table(new SimpleProcedureTable());
    table->m_name = proc.name.ident;
    table->m_parent = parent;
    for (auto section : type.params.params) {
        auto group = section.var ? SymbolGroup::VAR : SymbolGroup::CONST;
        auto res = table->symbols.add_symbol(messages, nodes::IdentDef{section.ident, false}, group, section.type);
        if (!res) return nullptr;
    }
    auto context = nodes::Context(messages, *table);
    auto func = [](auto ident, auto& context) {
        if (ident.def) {
            context.messages.addErr(ident.ident.place, "Cannot export local variable {}", ident.ident);
            return false;
        }
        return true;
    };
    SymbolContainer::parse(table->symbols, context, proc.decls, proc.body, func);
    return table;
}

bool SimpleProcedureTable::overload(MessageContainer& messages, std::shared_ptr<ProcedureTable>) {
    messages.addErr(m_name.place, "Attempt to overoload procedure: {}", m_name);
    return false;
}
