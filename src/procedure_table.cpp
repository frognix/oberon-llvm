#include "procedure_table.hpp"

ProcedureTable::ProcedureTable(nodes::Ident name, nodes::ProcedureType type, std::optional<nodes::ExpressionPtr> ret,
               nodes::StatementSequence body, SymbolTable* parent)
    : SymbolTable(body), m_name(name), m_type(type), m_ret(ret), m_parent(parent) {
    for (auto section : m_type.params.sections) {
        auto group = section.var ? SymbolGroup::VAR : SymbolGroup::CONST;
        for (auto ident : section.idents) {
            MessageManager messages;
            auto res = SymbolTable::add_symbol(messages, nodes::IdentDef{ident, false}, group, section.type);
            if (!res) throw std::runtime_error("Internal error, can't add symbol to table");
        }
    }
}

Maybe<SymbolToken> ProcedureTable::get_symbol(MessageManager& messages, const nodes::QualIdent& ident, bool secretly) const {
    if (ident.qual) {
        return m_parent->get_symbol(messages, ident, secretly);
    } else {
        if (!SymbolTable::has_symbol(ident)) {
            return m_parent->get_symbol(messages, ident, secretly);
        } else {
            auto res = SymbolTable::get_symbol(messages, ident, secretly);
            if (!res) return error;
            return res;
        }
    }
}

Maybe<nodes::ExpressionPtr> ProcedureTable::get_value(MessageManager& messages, const nodes::QualIdent& ident, bool secretly) const {
    if (ident.qual) {
        return m_parent->get_value(messages, ident, secretly);
    } else {
        if (!SymbolTable::has_symbol(ident)) {
            return m_parent->get_value(messages, ident, secretly);
        } else {
            auto res = SymbolTable::get_value(messages, ident, secretly);
            if (!res) return error;
            return res;
        }
    }
}

Maybe<TablePtr> ProcedureTable::get_table(MessageManager& messages, const nodes::QualIdent& ident, bool secretly) const {
    if (ident.qual) {
        return m_parent->get_table(messages, ident, secretly);
    } else {
        if (!SymbolTable::has_symbol(ident)) {
            return m_parent->get_table(messages, ident, secretly);
        } else {
            auto res = SymbolTable::get_table(messages, ident, secretly);
            if (!res) return error;
            return res;
        }
    }
}

bool ProcedureTable::add_symbol(MessageManager& messages, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) {
    if (ident.def) {
        messages.addErr(ident.ident.place, "Cannot export local variable {}", ident.ident);
        return berror;
    } else {
        return SymbolTable::add_symbol(messages, ident, group, type);
    }
}

bool ProcedureTable::type_extends_base(const nodes::Type* extension, nodes::QualIdent base) const {
    if (SymbolTable::type_extends_base(extension, base)) return true;
    return m_parent->type_extends_base(extension, base);
}
