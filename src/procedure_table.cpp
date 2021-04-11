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

bool parseProcedureType(MessageContainer& messages, SymbolTable& table, const nodes::ProcedureDeclaration& proc) {
    for (auto& section : proc.type.params.common) {
        auto group = section.var ? SymbolGroup::VAR : SymbolGroup::CONST;
        auto res = table.get_symbols().add_symbol(messages, nodes::IdentDef{section.ident, false}, group, section.type);
        if (!res) return false;
    }
    for (auto& section : proc.type.params.formal) {
        auto group = section.var ? SymbolGroup::VAR : SymbolGroup::CONST;
        auto res = table.get_symbols().add_symbol(messages, nodes::IdentDef{section.ident, false}, group, section.type);
        if (!res) return false;
    }
    auto func = [](auto ident, auto& context) {
        if (ident.def) {
            context.messages.addErr(ident.ident.place, "Cannot export local variable {}", ident.ident);
            return false;
        }
        return true;
    };
    auto context = nodes::Context(messages, table);
    if (proc.body) {
        SymbolContainer::parse(table.get_symbols(), context, proc.body->decls, proc.body->statements, func);
        if (proc.type.params.rettype) {
            auto return_res = proc.body->ret.value()->get_type(context);
            auto [ret_group, ret_type] = return_res.value();
            if (!proc.type.params.rettype.value()->assignment_compatible(context, *ret_type)) {
                messages.addErr(proc.body->ret.value()->place, "Incompatible formal and actual procedure return types");
                return false;
            }
        }
    }
    return true;
}
