#include "module_table.hpp"

ModuleTable::ModuleTable(nodes::Ident name, nodes::StatementSequence body)
    : SymbolTable(body), m_name(name) {}

Error ModuleTable::add_imports(nodes::ImportList imports) {
    for (auto& import : imports) {
        if (auto err = SymbolTable::add_symbol(nodes::IdentDef{import.name, false},
                                               SymbolGroup::MODULE, nodes::make_type<nodes::TypeName>(nodes::QualIdent{{}, import.name})); !err) {
            m_imports[import.name] = Import{import.real_name, nullptr};
        } else {
            return err;
        }
    }
    return {};
}

SemResult<SymbolToken> ModuleTable::get_symbol_out(const nodes::QualIdent& ident, bool secretly) const {
    if (m_exports.contains(ident.ident)) {
        nodes::QualIdent name{{}, ident.ident};
        auto res = SymbolTable::get_symbol(name, secretly);
        if (res) {
            auto symbol = res.get_ok();
            symbol.group = SymbolGroup::CONST;
            return symbol;
        } else return res.get_err();
    } else {
        return ErrorBuilder(ident.ident.place)
            .format("Attempting to access a non-exported symbol {}", ident)
            .build();
    }
}

SemResult<SymbolToken> ModuleTable::get_symbol(const nodes::QualIdent& ident, bool secretly) const {
    if (!ident.qual) {
        return SymbolTable::get_symbol(ident, secretly);
    } else {
        if (auto res = m_imports.find(*ident.qual); res != m_imports.end()) {
            auto& import = res->second;
            if (import.module == nullptr) {
                SymbolToken symbol{ident, SymbolGroup::ANY, nodes::make_type<nodes::AnyType>(), 0};
                return symbol;
            } else {
                return import.module->get_symbol_out(ident, secretly);
            }
        } else {
            return ErrorBuilder(ident.qual->place).format("Import '{}' does not exist", *ident.qual).build();
        }
    }
}

SemResult<nodes::ExpressionPtr> ModuleTable::get_value(const nodes::QualIdent& ident, bool secretly) const {
    if (!ident.qual) {
        return SymbolTable::get_value(ident, secretly);
    } else {
        if (auto res = m_imports.find(*ident.qual); res != m_imports.end()) {
            return ErrorBuilder(ident.ident.place).format("Attempting to access outer value {}", ident).build();
        } else {
            return ErrorBuilder(m_name.place).format("Import '{}' does not exist", res->second.name).build();
        }
    }
}

SemResult<TablePtr> ModuleTable::get_table(const nodes::QualIdent& ident, bool secretly) const {
    if (!ident.qual) {
        return SymbolTable::get_table(ident, secretly);
    } else {
        if (auto res = m_imports.find(*ident.qual); res != m_imports.end()) {
            return ErrorBuilder(ident.ident.place).format("Attempting to access outer table {}", ident).build();
        } else {
            return ErrorBuilder(m_name.place).format("Import '{}' does not exist", res->second.name).build();
        }
    }
}

Error ModuleTable::add_symbol(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) {
    if (ident.def) {
        auto error = SymbolTable::add_symbol(ident, group, type);
        if (!error) {
            m_exports.insert(ident.ident);
        }
        return error;
    } else {
        return SymbolTable::add_symbol(ident, group, type);
    }
}

bool ModuleTable::type_extends_base(const nodes::Type* extension, nodes::QualIdent base) const {
    if (auto isRecord = extension->is<nodes::RecordType>(); isRecord && isRecord->basetype) {
        if (isRecord->basetype->qual && base.qual) {
            if (auto mod = m_imports.at(*base.qual).module; mod) {
                auto type = nodes::TypeName(nodes::QualIdent{{}, isRecord->basetype->ident});
                return mod->type_extends_base(&type, nodes::QualIdent{{}, base.ident});
            } else return true;
        } else return SymbolTable::type_extends_base(extension, base);
    } else if (auto isTypeName = extension->is<nodes::TypeName>(); isTypeName) {
        if (isTypeName->ident.qual && base.qual) {
            if (auto mod = m_imports.at(*base.qual).module; mod) {
                auto type = nodes::TypeName(nodes::QualIdent{{}, isTypeName->ident.ident});
                return mod->type_extends_base(&type, nodes::QualIdent{{}, base.ident});
            } else return true;
        } else return SymbolTable::type_extends_base(extension, base);
    } else return false;
}
