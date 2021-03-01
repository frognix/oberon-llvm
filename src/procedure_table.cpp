#include "procedure_table.hpp"

ProcedureTable::ProcedureTable(nodes::Ident name, nodes::ProcedureType type, std::optional<nodes::ExpressionPtr> ret,
               nodes::StatementSequence body, SymbolTable* parent)
    : SymbolTable(body), m_name(name), m_type(type), m_ret(ret), m_parent(parent) {
    for (auto section : m_type.params.sections) {
        auto group = section.var ? SymbolGroup::VAR : SymbolGroup::CONST;
        for (auto ident : section.idents) {
            auto res = SymbolTable::add_symbol(nodes::IdentDef{ident, false}, group, section.type);
        }
    }
}

SemResult<SymbolToken> ProcedureTable::get_symbol(const nodes::QualIdent& ident, bool secretly) const {
    if (ident.qual) {
        return m_parent->get_symbol(ident, secretly);
    } else {
        if (auto inner = SymbolTable::get_symbol(ident, secretly); !inner) {
            return m_parent->get_symbol(ident, secretly);
        } else {
            return inner;
        }
    }
}

SemResult<nodes::ExpressionPtr> ProcedureTable::get_value(const nodes::QualIdent& ident, bool secretly) const {
    if (ident.qual) {
        return m_parent->get_value(ident, secretly);
    } else {
        if (auto inner = SymbolTable::get_value(ident, secretly); !inner) {
            return m_parent->get_value(ident, secretly);
        } else {
            return inner;
        }
    }
}

SemResult<TablePtr> ProcedureTable::get_table(const nodes::QualIdent& ident, bool secretly) const {
    if (ident.qual) {
        return m_parent->get_table(ident, secretly);
    } else {
        if (auto inner = SymbolTable::get_table(ident, secretly); !inner) {
            return m_parent->get_table(ident, secretly);
        } else {
            return inner;
        }
    }
}

Error ProcedureTable::add_symbol(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) {
    if (ident.def) {
        return ErrorBuilder(ident.ident.place).format("Cannot export local variable {}", ident.ident).build();
    } else {
        return SymbolTable::add_symbol(ident, group, type);
    }
}

bool ProcedureTable::type_extends_base(const nodes::Type* extension, nodes::QualIdent base) const {
    if (SymbolTable::type_extends_base(extension, base)) return true;
    return m_parent->type_extends_base(extension, base);
}
