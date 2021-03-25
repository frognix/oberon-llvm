#include "module_table.hpp"

std::unique_ptr<ModuleTable> ModuleTable::parse(const nodes::Module& module, std::vector<std::pair<nodes::Import, ModuleTablePtr>> imports, MessageContainer& mm) {
    std::unique_ptr<ModuleTable> table(new ModuleTable());
    table->m_name = module.name;
    for (auto [import, ptr] : imports) {
        auto res = table->add_import(mm, import, ptr);
        if (!res) return nullptr;
    }
    auto context = nodes::Context(mm, *table);
    auto func = [&table](auto ident, auto) {
        if (ident.def) table->add_export(ident.ident);
        return true;
    };
    SymbolTable::parse(table->symbols, context, module.declarations, module.body, func);
    return table;
}

std::unique_ptr<ModuleTable> ModuleTable::parse(const nodes::Definition& def, std::vector<std::pair<nodes::Import, ModuleTablePtr>> imports, MessageContainer& mm) {
    std::unique_ptr<ModuleTable> table(new ModuleTable());
    table->m_name = def.name;
    for (auto [import, ptr] : imports) {
        auto res = table->add_import(mm, import, ptr);
        if (!res) return nullptr;
    }
    auto context = nodes::Context(mm, *table);
    auto func = [&table](auto ident, auto) {
        table->add_export(ident.ident);
        return true;
    };
    nodes::DeclarationSequence seq;
    seq.constDecls = def.definitions.constDecls;
    seq.typeDecls = def.definitions.typeDecls;
    seq.variableDecls = def.definitions.variableDecls;
    for (auto proc : def.definitions.procedureDecls) {
        seq.variableDecls.push_back(nodes::VariableDecl({proc.name}, make_type<nodes::ProcedureType>(proc.type)));
    }
    SymbolTable::parse(table->symbols, context, seq, {}, func);
    return table;
}

bool ModuleTable::add_import(MessageContainer& messages, nodes::Import import, ModuleTablePtr ptr) {
    auto res = symbols.add_symbol(messages, nodes::IdentDef{import.name, false},
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
        auto res = symbols.get_symbol(messages, name, secretly);
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
        return symbols.get_symbol(messages, ident, secretly);
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

Maybe<nodes::ValuePtr> ModuleTable::get_value(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const {
    if (!ident.qual) {
        return symbols.get_value(messages, ident, secretly);
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

std::string ModuleTable::to_string() const {
    return symbols.to_string();
}

 bool ModuleTable::analyze_code(MessageContainer& messages) const {
     auto context = nodes::Context(messages, *this);
     return symbols.analyze_code(context);
}
