#include "procedure_table.hpp"

std::unique_ptr<ProcedureTable> ProcedureTable::parse(const nodes::ProcedureDeclaration& proc, const nodes::ProcedureType& type, const SymbolTableI* parent, MessageContainer& messages) {
    std::unique_ptr<ProcedureTable> table(new ProcedureTable());
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
    SymbolTable::parse(table->symbols, context, proc.decls, proc.body, func);
    return table;
}

Maybe<SymbolToken> ProcedureTable::get_symbol(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const {
    if (ident.qual) {
        return m_parent->get_symbol(messages, ident, secretly);
    } else {
        if (!symbols.has_symbol(ident)) {
            return m_parent->get_symbol(messages, ident, secretly);
        } else {
            auto res = symbols.get_symbol(messages, ident, secretly);
            if (!res) return error;
            return res;
        }
    }
}

Maybe<nodes::ExpressionPtr> ProcedureTable::get_value(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const {
    if (ident.qual) {
        return m_parent->get_value(messages, ident, secretly);
    } else {
        if (!symbols.has_symbol(ident)) {
            return m_parent->get_value(messages, ident, secretly);
        } else {
            auto res = symbols.get_value(messages, ident, secretly);
            if (!res) return error;
            return res;
        }
    }
}

std::string ProcedureTable::to_string() const {
    return symbols.to_string();
}

bool ProcedureTable::analyze_code(MessageContainer& messages) const {
    auto context = nodes::Context(messages, *this);
     return symbols.analyze_code(context);
}
