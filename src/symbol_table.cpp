#include "symbol_table.hpp"
#include "procedure_table.hpp"

ParseReturnType SymbolTable::parse(const nodes::DeclarationSequence& seq, nodes::StatementSequence body, MessageContainer& mm) {
    std::unique_ptr<SymbolTable> table(new SymbolTable());
    return SymbolTable::base_parse(std::move(table), seq, body, mm);
}

ParseReturnType SymbolTable::base_parse(std::unique_ptr<SymbolTable> table, const nodes::DeclarationSequence& seq, nodes::StatementSequence body, MessageContainer& mm) {
    table->body = body;
    auto context = nodes::Context(mm, *table.get());
    for (auto& decl : seq.constDecls) {
        auto res = decl.expression->eval(context);
        if (!res) return error;
        auto expr = *res;
        if (auto exprRes = expr->get_type(context); exprRes) {
            auto [group, type] = *exprRes;
            auto res = table->add_value(mm, decl.ident, group, type, expr);
            if (!res) return error;
        } else {
            return error;
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
            if (!res) return error;
            type = *res;
        }
        auto res = table->add_symbol(mm, decl.ident, SymbolGroup::TYPE, type);
        if (!res) return error;
    }
    for (auto& decl : unchecked_types) {
        auto type = table->symbols[decl.ident.ident].type;
        auto res = dynamic_cast<nodes::PointerType*>(type.get())->check_type(context);
        if (!res) return error;
    }
    for (auto& decl : seq.variableDecls) {
        if (auto type = decl.type->normalize(context, false); !type) {
            return error;
        } else {
            for (auto& var : decl.list) {
                auto res = table->add_symbol(mm, var, SymbolGroup::VAR, *type);
                if (!res) return error;
            }
        }
    }
    for (auto& _decl : seq.procedureDecls) {
        auto& decl = *dynamic_cast<nodes::ProcedureDeclaration*>(_decl.get());
        if (auto type = decl.type.normalize(context, false); !type) {
            return error;
        } else {
            auto res = ProcedureTable::parse(decl.name.ident, *type.value()->is<typename nodes::ProcedureType>(), decl.ret, decl.body, decl.decls, table.get(), mm);
            if (!res) return error;
            auto res1 = table->add_table(mm, decl.name, SymbolGroup::CONST, *type, TablePtr(dynamic_cast<SymbolTable*>(res->release())));
            if (!res1) return error;
        }
    }
    return std::optional(std::unique_ptr<SemanticUnitI>(table.release()));
}

bool SymbolTable::analyze_code(MessageContainer& messages) const {
    auto context = nodes::Context(messages, *this);
    auto serror = false;
    for (auto& [name, table] : tables) {
        auto res = table->analyze_code(messages);
        if (!res) serror = true;
    }
    for (auto& statement : body) {
        auto res = statement->check(context);
        if (!res) serror = true;
    }
    for (auto [name, symbol] : symbols) {
        if (symbol.count == 0)
            messages.addFormat(MPriority::W4, symbol.name.ident.place, "Unised symbol: {}", symbol.name);
    }
    if (serror) return berror;
    return bsuccess;
}

bool SymbolTable::add_symbol(MessageContainer& messages, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) {
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

bool SymbolTable::add_value(MessageContainer& messages, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type,
                             nodes::ExpressionPtr value) {
    if (auto res = add_symbol(messages, ident, group, type); !res) {
        return berror;
    } else {
        values[ident.ident] = value;
        return bsuccess;
    }
}

bool SymbolTable::add_table(MessageContainer& messages, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, TablePtr table) {
    if (auto res = add_symbol(messages, ident, group, type); !res) {
        return berror;
    } else {
        tables[ident.ident] = std::move(table);
        return bsuccess;
    }
}

Maybe<SymbolToken> SymbolTable::get_symbol(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const {
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

Maybe<nodes::ExpressionPtr> SymbolTable::get_value(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const {
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
Maybe<TablePtr> SymbolTable::get_table(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const {
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
