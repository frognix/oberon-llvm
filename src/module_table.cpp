#include "module_table.hpp"

ParseReturnType ModuleTable::parse(nodes::Ident name, std::vector<std::pair<nodes::Import, ModuleTablePtr>> imports,
                                   const nodes::DeclarationSequence& seq, nodes::StatementSequence body, MessageContainer& mm) {
    std::unique_ptr<ModuleTable> table(new ModuleTable());
    table->m_name = name;
    for (auto [import, ptr] : imports) {
        auto res = table->add_import(mm, import, ptr);
        if (!res) return error;
    }
    return SymbolTable::base_parse(std::unique_ptr<SymbolTable>(table.release()), seq, body, mm);
}

bool ModuleTable::add_import(MessageContainer& messages, nodes::Import import, ModuleTablePtr ptr) {
    auto res = SymbolTable::add_symbol(messages, nodes::IdentDef{import.name, false},
                                       SymbolGroup::MODULE, nodes::make_type<nodes::TypeName>(nodes::QualIdent{{}, import.name}));
    if (res) {
        m_imports[import.name] = Import{import.real_name, ptr};
    } else {
        return berror;
    }
    return bsuccess;
}

Maybe<SymbolToken> ModuleTable::get_symbol_out(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const {
    if (m_exports.contains(ident.ident)) {
        nodes::QualIdent name{{}, ident.ident};
        auto res = SymbolTable::get_symbol(messages, name, secretly);
        if (res) {
            auto symbol = *res;
            symbol.group = SymbolGroup::CONST;
            return symbol;
        } else {
            messages.addErr(ident.ident.place, "Symbol {} not found", ident);
            return error;
        }
    } else {
        messages.addErr(ident.ident.place, "Attempting to access a non-exported symbol {}", ident);
        return error;
    }
}

Maybe<SymbolToken> ModuleTable::get_symbol(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const {
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

Maybe<nodes::ExpressionPtr> ModuleTable::get_value(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const {
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

Maybe<TablePtr> ModuleTable::get_table(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const {
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

bool ModuleTable::add_symbol(MessageContainer& messages, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) {
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
