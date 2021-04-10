#include "symbol_container.hpp"
#include "node.hpp"
#include "procedure_table.hpp"

bool SymbolContainer::parse(SymbolContainer& table, nodes::Context& context, const nodes::DeclarationSequence& seq, nodes::StatementSequence body, std::function<bool(nodes::IdentDef,nodes::Context&)> func)  {
    table.body = body;
    for (auto& decl : seq.constDecls) {
        auto res = decl.expression.get(context);
        if (!res) return berror;
        auto expr = *res;
        if (auto exprRes = expr->get_type(context); exprRes) {
            auto [group, type] = *exprRes;
            if (!func(decl.ident, context)) return berror;
            auto res = table.add_value(context.messages, decl.ident, group, type, expr);
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
        if (!func(decl.ident, context)) return berror;
        auto res = table.add_symbol(context.messages, decl.ident, SymbolGroup::TYPE, type);
        if (!res) return berror;
    }
    for (auto& decl : unchecked_types) {
        auto type = table.symbols[decl.ident.ident].type;
        auto res = dynamic_cast<nodes::PointerType*>(type.get())->check_type(context);
        if (!res) return berror;
    }
    for (auto& decl : seq.variableDecls) {
        if (auto type = decl.type->normalize(context, false); !type) {
            return berror;
        } else {
            for (auto& var : decl.list) {
                if (!func(var, context)) return berror;
                auto res = table.add_symbol(context.messages, var, SymbolGroup::VAR, *type);
                if (!res) return berror;
            }
        }
    }
    for (auto& _decl : seq.procedureDecls) {
        auto decl = *dynamic_cast<nodes::ProcedureDeclaration*>(_decl.get());
        if (auto type = decl.type.normalize(context, false); !type) {
            return berror;
        } else {
            decl.type = *type.value()->is<nodes::ProcedureType>();
            auto res = build_procedure_table(decl, &context.symbols, context.messages);
            if (!res.get()) return berror;
            if (!func(decl.name, context)) return berror;
            auto res1 = table.add_table(context.messages, decl.name, SymbolGroup::CONST, *type, TablePtr(res.release()));
            if (!res1) return berror;
        }
    }
    return bsuccess;
}

bool SymbolContainer::analyze_code(nodes::Context& context) const {
    auto serror = false;
    for (auto& [name, table] : tables) {
        auto res = table->analyze_code(context.messages);
        if (!res) serror = true;
    }
    for (auto& statement : body) {
        auto res = statement->check(context);
        if (!res) serror = true;
    }
    // for (auto [name, symbol] : symbols) {
    //     if (symbol.count == 0)
    //         context.messages.addFormat(MPriority::W4, symbol.name.ident.place, "Unused symbol: {}", symbol.name);
    // }
    if (serror) return berror;
    return bsuccess;
}

bool SymbolContainer::add_symbol(MessageContainer& messages, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) {
    if (symbols.contains(ident.ident)) {
        auto symbol = symbols[ident.ident];
        messages.addErr(ident.ident.place, "Redefinition of symbol {}", ident.ident);
        // messages.addFormat(MPriority::W1, ident.ident.place, "{} First definition here", symbol.name.ident.place);
        // messages.addFormat(MPriority::W1, ident.ident.place, "{} Second definition here", ident.ident.place);
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

bool SymbolContainer::add_value(MessageContainer& messages, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type,
                             nodes::ValuePtr value) {
    if (auto res = add_symbol(messages, ident, group, type); !res) {
        return berror;
    } else {
        values[ident.ident] = value;
        return bsuccess;
    }
}

bool SymbolContainer::add_table(MessageContainer& messages, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, TablePtr table) {
    if (auto tableRes = get_table(messages, nodes::QualIdent{{}, ident.ident}, true); tableRes) {
        if (tableRes.value()->can_overload(messages, *table)) {
            if (&tableRes.value()->parent().get_symbols() == this) {
                return tableRes.value()->overload(messages, table);
            } else {
                tables[ident.ident] = std::move(table);
                return bsuccess;
            }
        }
        return tableRes.value()->overload(messages, table);
    }
    if (auto res = add_symbol(messages, ident, group, type); !res) {
        return berror;
    } else {
        tables[ident.ident] = std::move(table);
        return bsuccess;
    }
}

Maybe<SymbolToken> SymbolContainer::get_symbol(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const {
    auto msg = Message(MPriority::ERR, ident.ident.place, fmt::format("Symbol {} not found", ident));
    if (ident.qual) {
        if (!secretly)
            messages.addMessage(msg);
        return error;
    } else {
        if (auto res = symbols.find(ident.ident); res != symbols.end()) {
            if (!secretly)
                const_cast<SymbolContainer*>(this)->symbols[ident.ident].count++;
            return SymbolToken(res->second);
        } else {
            if (!secretly)
                messages.addMessage(msg);
            return error;
        }
    }
}

Maybe<nodes::ValuePtr> SymbolContainer::get_value(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const {
    auto msg = Message(MPriority::ERR, ident.ident.place, fmt::format("Symbol {} not found", ident));
    if (ident.qual) {
        if (!secretly)
            messages.addMessage(msg);
        return error;
    } else {
        if (auto res = values.find(ident.ident); res != values.end()) {
            if (!secretly)
                const_cast<SymbolContainer*>(this)->symbols[ident.ident].count++;
            return nodes::ValuePtr(res->second);
        } else {
            if (!secretly)
                messages.addMessage(msg);
            return error;
        }
    }
}

Maybe<TablePtr> SymbolContainer::get_table(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const {
    auto msg = Message(MPriority::ERR, ident.ident.place, fmt::format("Symbol {} not found", ident));
    if (ident.qual) {
        if (!secretly)
            messages.addMessage(msg);
        return error;
    } else {
        if (auto res = tables.find(ident.ident); res != tables.end()) {
            if (!secretly)
                const_cast<SymbolContainer*>(this)->symbols[ident.ident].count++;
            return TablePtr(res->second);
        } else {
            if (!secretly)
                messages.addMessage(msg);
            return error;
        }
    }
}

std::string SymbolContainer::to_string() const {
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
