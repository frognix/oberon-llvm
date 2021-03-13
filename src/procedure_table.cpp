#include "procedure_table.hpp"

ParseReturnType ProcedureTable::parse(nodes::Ident name, nodes::ProcedureType type,
                                          std::optional<nodes::ExpressionPtr> ret,
                                          nodes::StatementSequence body, nodes::DeclarationSequence seq,
                                          SymbolTable* parent, MessageContainer& mm) {
    std::unique_ptr<ProcedureTable> table(new ProcedureTable());
    table->m_name = name;
    table->m_type = type;
    table->m_ret = ret;
    table->m_parent = parent;
    for (auto section : table->m_type.params.params) {
        auto group = section.var ? SymbolGroup::VAR : SymbolGroup::CONST;
        MessageContainer messages;
        auto res = table->SymbolTable::add_symbol(messages, nodes::IdentDef{section.ident, false}, group, section.type);
        if (!res) return error;
    }
    return SymbolTable::base_parse(std::unique_ptr<SymbolTable>(table.release()), seq, body, mm);
}

Maybe<SymbolToken> ProcedureTable::get_symbol(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const {
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

Maybe<nodes::ExpressionPtr> ProcedureTable::get_value(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const {
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

Maybe<TablePtr> ProcedureTable::get_table(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const {
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

bool ProcedureTable::add_symbol(MessageContainer& messages, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) {
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
