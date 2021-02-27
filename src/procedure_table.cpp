#include "procedure_table.hpp"

ProcedureTable::ProcedureTable(nodes::Ident name, nodes::ProcedureType type, std::optional<nodes::ExpressionPtr> ret,
               nodes::StatementSequence body, SymbolTable* parent)
    : SymbolTable(), m_name(name), m_type(type), m_ret(ret), m_body(body), m_parent(parent) {}

SemResult<Symbol> ProcedureTable::get_symbol(const nodes::QualIdent& ident) const {
    if (ident.qual) {
        return m_parent->get_symbol(ident);
    } else {
        return SymbolTable::get_symbol(ident);
    }
}

SemResult<nodes::ExpressionPtr> ProcedureTable::get_value(const nodes::QualIdent& ident) const {
    if (ident.qual) {
        return m_parent->get_value(ident);
    } else {
        return SymbolTable::get_value(ident);
    }
}

SemResult<TablePtr> ProcedureTable::get_table(const nodes::QualIdent& ident) const {
    if (ident.qual) {
        return m_parent->get_table(ident);
    } else {
        return SymbolTable::get_table(ident);
    }
}

Error ProcedureTable::add_symbol(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) {
    if (ident.def) {
        return ErrorBuilder(ident.ident.place).format("Cannot export local variable {}", ident.ident).build();
    } else {
        return SymbolTable::add_symbol(ident, group, type);
    }
}

bool ProcedureTable::type_extends_base(nodes::QualIdent extension, nodes::QualIdent base) const {
    if (SymbolTable::type_extends_base(extension, base)) return true;
    return m_parent->type_extends_base(extension, base);
}
