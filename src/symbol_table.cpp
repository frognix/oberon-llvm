#include "symbol_table.hpp"
#include "procedure_table.hpp"

void TypeHierarchy::add_extension(nodes::QualIdent extension, nodes::QualIdent base) {
    m_extended_types[extension] = base;
}

bool TypeHierarchy::extends(nodes::QualIdent extension, nodes::QualIdent base) const {
    if (extension == base)
        return true;
    while (true) {
        if (auto res = m_extended_types.find(extension); res != m_extended_types.end()) {
            if (res->second == base)
                return true;
            else
                extension = res->second;
        } else {
            return false;
        }
    }
}

SymbolTable::SymbolTable(nodes::StatementSequence b) : body(b) {}

bool SymbolTable::parse(const nodes::DeclarationSequence& seq, MessageManager& mm) {
    auto context = nodes::Context(mm, *this);
    for (auto& decl : seq.constDecls) {
        auto res = decl.expression->eval(context);
        if (!res) return berror;
        auto expr = *res;
        if (auto type = expr->get_type(context); type) {
            auto res = add_value(mm, decl.ident, SymbolGroup::CONST, *type, expr);
            if (!res) return berror;
        } else {
            return berror;
        }
    }
    std::vector<nodes::TypeDecl> unchecked_types;
    for (auto& decl : seq.typeDecls) {
        nodes::TypePtr type;
        if (auto pointer = dynamic_cast<nodes::PointerType*>(decl.type.get()); pointer) {
            unchecked_types.push_back(decl);
            type = decl.type;
        } else {
            auto res = decl.type->normalize(context, false);
            if (!res) return berror;
            type = *res;
        }
        auto res = add_symbol(mm, decl.ident, SymbolGroup::TYPE, type);
        if (!res) return berror;
        if (auto isRecord = type->is<nodes::RecordType>(); isRecord && isRecord->basetype) {
            type_hierarchy.add_extension(nodes::QualIdent{{}, decl.ident.ident}, *isRecord->basetype);
        }
    }
    for (auto& decl : unchecked_types) {
        auto type = symbols[decl.ident.ident].type;
        auto res = dynamic_cast<nodes::PointerType*>(type.get())->check_type(context);
        if (!res) return berror;
    }
    for (auto& decl : seq.variableDecls) {
        if (auto type = decl.type->normalize(context, false); !type) {
            return berror;
        } else {
            for (auto& var : decl.list) {
                auto res = add_symbol(mm, var, SymbolGroup::VAR, *type);
                if (!res) return berror;
            }
        }
    }
    for (auto& _decl : seq.procedureDecls) {
        auto& decl = *dynamic_cast<nodes::ProcedureDeclaration*>(_decl.get());
        if (auto type = decl.type.normalize(context, false); !type) {
            return berror;
        } else {
            auto table = std::make_shared<ProcedureTable>(decl.name.ident, *type.value()->is<nodes::ProcedureType>(),
                                                          decl.ret, decl.body, this);
            if (auto res = table->parse(decl.decls, mm); !res) {
                return berror;
            }
            auto res = add_table(mm, decl.name, SymbolGroup::CONST, *type, TablePtr(table));
            if (!res) return berror;
        }
    }
    for (auto& statement : body) {
        if (auto res = statement->check(context); !res)
            return berror;
    }
    return bsuccess;
}

bool SymbolTable::add_symbol(MessageManager& messages, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) {
    if (symbols.contains(ident.ident)) {
        auto symbol = symbols[ident.ident];
        messages.addErr(ident.ident.place, "Redefinition of symbol {}", ident.ident);
        messages.addFormat(MPriority::W1, ident.ident.place, "{} First definition here", symbol.name.ident.place);
        messages.addFormat(MPriority::W1, ident.ident.place, "{} Second definition here", ident.ident.place);
        return berror;
    } else {
        SymbolToken symbol;
        symbol.name = nodes::QualIdent{{}, ident.ident};
        symbol.group = group;
        symbol.type = type;
        symbol.count = 0;
        symbols[ident.ident] = symbol;
        return bsuccess;
    }
}

bool SymbolTable::add_value(MessageManager& messages, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type,
                             nodes::ExpressionPtr value) {
    if (auto res = add_symbol(messages, ident, group, type); !res) {
        return berror;
    } else {
        values[ident.ident] = value;
        return bsuccess;
    }
}

bool SymbolTable::add_table(MessageManager& messages, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, TablePtr table) {
    if (auto res = add_symbol(messages, ident, group, type); !res) {
        return berror;
    } else {
        tables[ident.ident] = std::move(table);
        return bsuccess;
    }
}

Maybe<SymbolToken> SymbolTable::get_symbol(MessageManager& messages, const nodes::QualIdent& ident, bool secretly) const {
    auto msg = Message(MPriority::ERR, ident.ident.place, fmt::format("Symbol {} not found", ident));
    if (ident.qual) {
        messages.addMessage(msg);
        return error;
    } else {
        if (auto res = symbols.find(ident.ident); res != symbols.end()) {
            if (!secretly)
                const_cast<SymbolTable*>(this)->symbols[ident.ident].count++;
            return SymbolToken(res->second);
        } else {
            messages.addMessage(msg);
            return error;
        }
    }
}

Maybe<nodes::ExpressionPtr> SymbolTable::get_value(MessageManager& messages, const nodes::QualIdent& ident, bool secretly) const {
    auto msg = Message(MPriority::ERR, ident.ident.place, fmt::format("Symbol {} not found", ident));
    if (ident.qual) {
        messages.addMessage(msg);
        return error;
    } else {
        if (auto res = values.find(ident.ident); res != values.end()) {
            if (!secretly)
                const_cast<SymbolTable*>(this)->symbols[ident.ident].count++;
            return nodes::ExpressionPtr(res->second);
        } else {
            messages.addMessage(msg);
            return error;
        }
    }
}
Maybe<TablePtr> SymbolTable::get_table(MessageManager& messages, const nodes::QualIdent& ident, bool secretly) const {
    auto msg = Message(MPriority::ERR, ident.ident.place, fmt::format("Symbol {} not found", ident));
    if (ident.qual) {
        messages.addMessage(msg);
        return error;
    } else {
        if (auto res = tables.find(ident.ident); res != tables.end()) {
            if (!secretly)
                const_cast<SymbolTable*>(this)->symbols[ident.ident].count++;
            return TablePtr(res->second);
        } else {
            messages.addMessage(msg);
            return error;
        }
    }
}

bool SymbolTable::type_extends_base(const nodes::Type* extension, nodes::QualIdent base) const {
    if (auto isRecord = dynamic_cast<const nodes::RecordType*>(extension); isRecord && isRecord->basetype) {
        return type_hierarchy.extends(*isRecord->basetype, base);
    } else if (auto isName = dynamic_cast<const nodes::TypeName*>(extension); isName) {
        return type_hierarchy.extends(isName->ident, base);
    } else
        return false;
}

std::string SymbolTable::to_string() const {
    std::string result;
    result += fmt::format("Symbols ({}):\n", symbols.size());
    for (auto [name, symbol] : symbols) {
        result += fmt::format("{}: {}\n", name, symbol);
    }
    result += fmt::format("Values ({}):\n", values.size());
    for (auto [name, value] : values) {
        result += fmt::format("{}: {}\n", name, value->to_string());
    }
    result += fmt::format("Tables ({}):\n", tables.size());
    for (auto [name, table] : tables) {
        result += fmt::format("{}:\n{}\n", name, table->to_string());
    }
    return result;
}
