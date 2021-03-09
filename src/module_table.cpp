#include "module_table.hpp"

ModuleTable::ModuleTable(nodes::Ident name, nodes::StatementSequence body)
    : SymbolTable(body), m_name(name) {}

bool ModuleTable::add_imports(MessageManager& messages, nodes::ImportList imports) {
    for (auto& import : imports) {
        auto res = SymbolTable::add_symbol(messages, nodes::IdentDef{import.name, false},
                                           SymbolGroup::MODULE, nodes::make_type<nodes::TypeName>(nodes::QualIdent{{}, import.name}));
        if (res) {
            m_imports[import.name] = Import{import.real_name, nullptr};
        } else {
            return berror;
        }
    }
    return bsuccess;
}

bool ModuleTable::set_module(MessageManager& messages, ModuleTablePtr module_ptr) {
    auto res = std::find_if(m_imports.begin(), m_imports.end(), [module_ptr](auto pair){
        auto [name, module] = pair;
        return module.name == module_ptr->m_name;
    });
    if (res != m_imports.end()) {
        res->second.module = module_ptr;
        return bsuccess;
    } else {
        messages.addErr(module_ptr->m_name.place, "Module {} not imported", module_ptr->m_name);
        return berror;
    }
}

Maybe<SymbolToken> ModuleTable::get_symbol_out(MessageManager& messages, const nodes::QualIdent& ident, bool secretly) const {
    if (m_exports.contains(ident.ident)) {
        nodes::QualIdent name{{}, ident.ident};
        auto res = SymbolTable::get_symbol(messages, name, secretly);
        if (res) {
            auto symbol = *res;
            symbol.group = SymbolGroup::CONST;
            return symbol;
        } else return error;
    } else {
        messages.addErr(ident.ident.place, "Attempting to access a non-exported symbol {}", ident);
        return error;
    }
}

Maybe<SymbolToken> ModuleTable::get_symbol(MessageManager& messages, const nodes::QualIdent& ident, bool secretly) const {
    if (!ident.qual) {
        return SymbolTable::get_symbol(messages, ident, secretly);
    } else {
        if (auto res = m_imports.find(*ident.qual); res != m_imports.end()) {
            auto& import = res->second;
            if (import.module == nullptr) {
                messages.addErr(ident.qual->place, "Module {} not found", import.name);
                return error;
            } else {
                return import.module->get_symbol_out(messages, ident, secretly);
            }
        } else {
            messages.addErr(ident.qual->place, "Import '{}' does not exist", *ident.qual);
            return error;
        }
    }
}

Maybe<nodes::ExpressionPtr> ModuleTable::get_value(MessageManager& messages, const nodes::QualIdent& ident, bool secretly) const {
    if (!ident.qual) {
        return SymbolTable::get_value(messages, ident, secretly);
    } else {
        if (auto res = m_imports.find(*ident.qual); res != m_imports.end()) {
            messages.addErr(ident.ident.place, "Attempting to access outer value {}", ident);
            return error;
        } else {
            messages.addErr(m_name.place, "Import '{}' does not exist", res->second.name);
            return error;
        }
    }
}

Maybe<TablePtr> ModuleTable::get_table(MessageManager& messages, const nodes::QualIdent& ident, bool secretly) const {
    if (!ident.qual) {
        return SymbolTable::get_table(messages, ident, secretly);
    } else {
        if (auto res = m_imports.find(*ident.qual); res != m_imports.end()) {
            messages.addErr(ident.ident.place, "Attempting to access outer table {}", ident);
            return error;
        } else {
            messages.addErr(m_name.place, "Import '{}' does not exist", res->second.name);
            return error;
        }
    }
}

bool ModuleTable::add_symbol(MessageManager& messages, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) {
    if (ident.def) {
        auto res = SymbolTable::add_symbol(messages, ident, group, type);
        if (res) {
            m_exports.insert(ident.ident);
            return bsuccess;
        }
        return berror;
    } else {
        return SymbolTable::add_symbol(messages, ident, group, type);
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
