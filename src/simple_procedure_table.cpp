#include "simple_procedure_table.hpp"
#include "procedure_table.hpp"

std::unique_ptr<SimpleProcedureTable> SimpleProcedureTable::parse(const nodes::ProcedureDeclaration& proc, const SymbolTable* parent, MessageContainer& messages) {
    if (!proc.body)
        internal::compiler_error("Bad ProcedureDeclaration structure");
    std::unique_ptr<SimpleProcedureTable> table(new SimpleProcedureTable());
    table->m_name = proc.name.ident;
    table->m_parent = parent;
    auto context = nodes::Context(messages, *table);
    auto func = [](auto ident, auto& context) {
        if (ident.def) {
            context.messages.addErr(ident.ident.place, "Cannot export local variable {}", ident.ident);
            return false;
        }
        return true;
    };
    auto success = parseProcedureType(messages, *table, proc.type);
    if (!success) return nullptr;
    SymbolContainer::parse(table->m_symbols, context, proc.body->decls, proc.body->statements, func);
    return table;
}

bool SimpleProcedureTable::overload(MessageContainer& messages, std::shared_ptr<ProcedureTable>) {
    messages.addErr(m_name.place, "Attempt to overload procedure: {}", m_name);
    return false;
}

bool SimpleProcedureTable::suitable_declaration(const nodes::ProcedureDeclaration& proc) {
    return proc.type.params.common.size() == 0 && proc.body;
}
