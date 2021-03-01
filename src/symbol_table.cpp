#include "symbol_table.hpp"
#include "procedure_table.hpp"

SymbolTable::SymbolTable(nodes::StatementSequence b) : body(b) {}

Error SymbolTable::parse(const nodes::DeclarationSequence& seq) {
    for (auto& decl : seq.constDecls) {
        auto exprError = decl.expression->eval(*this);
        if (!exprError) return exprError.get_err();
        auto expr = exprError.get_ok();
        if (auto type = expr->get_type(*this); type) {
            auto error = add_value(decl.ident, SymbolGroup::CONST, type.get_ok(), expr);
            if (error) return error;
        } else {
            return type.get_err();
        }
    }
    std::vector<nodes::TypeDecl> unchecked_types;
    for (auto& decl : seq.typeDecls) {
        Error typeError;
        if (auto pointer = dynamic_cast<nodes::PointerType*>(decl.type.get()); pointer) {
            unchecked_types.push_back(decl);
            typeError = {};
        } else {
            typeError = decl.type->check(*this);
        }
        if (typeError) {
            return typeError;
        } else {
            auto error = add_symbol(decl.ident, SymbolGroup::TYPE, decl.type);
            if (error) return error;
        }
    }
    for (auto& decl : unchecked_types) {
        if (auto typeError = symbols[decl.ident.ident].type->check(*this); typeError) {
            return typeError;
        }
    }
    for (auto& decl : seq.variableDecls) {
        if (auto typeError = decl.type->check(*this); typeError) {
            return typeError;
        } else {
            for (auto& var : decl.list) {
                auto error = add_symbol(var, SymbolGroup::VAR, decl.type);
                if (error) return error;
            }
        }
    }
    for (auto& _decl : seq.procedureDecls) {
        auto& decl = *dynamic_cast<nodes::ProcedureDeclaration*>(_decl.get());
        if (auto typeError = decl.type.check(*this); typeError) {
            return typeError;
        } else {
            auto table = std::make_shared<ProcedureTable>(decl.name.ident, decl.type, decl.ret, decl.body, this);
            if (auto tableError = table->parse(decl.decls); tableError) {
                return tableError;
            }
            nodes::TypePtr typePtr = std::make_shared<nodes::ProcedureType>(decl.type);
            auto error = add_table(decl.name, SymbolGroup::CONST, typePtr, TablePtr(table));
            if (error) return error;
        }
    }
    for (auto& statement : body) {
        if (auto error = statement->check(*this); error) return error;
    }
    return {};
}

Error SymbolTable::add_symbol(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) {
    if (symbols.contains(ident.ident)) {
        auto symbol = symbols[ident.ident];
        return ErrorBuilder(ident.ident.place).format("Redefinition of symbol {}", ident.ident)
            .format("{} First definition here", symbol.name.ident.place)
            .format("{} Second definition here", ident.ident.place).build();
    } else {
        SymbolToken symbol;
        symbol.name = nodes::QualIdent({}, ident.ident);
        symbol.group = group;
        symbol.type = type;
        symbol.count = 0;
        symbols[ident.ident] = symbol;
        return {};
    }
}

Error SymbolTable::add_value(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, nodes::ExpressionPtr value) {
    if (auto error = add_symbol(ident, group, type); error) {
        return error;
    } else {
        values[ident.ident] = value;
        return {};
    }
}

Error SymbolTable::add_table(nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, TablePtr table) {
    if (auto error = add_symbol(ident, group, type); error) {
        return error;
    } else {
        tables[ident.ident] = std::move(table);
        return {};
    }
}

SemResult<SymbolToken> SymbolTable::get_symbol(const nodes::QualIdent& ident, bool secretly) const {
    auto error =  ErrorBuilder(ident.ident.place).format("Symbol {} not found", ident).build();
    if (ident.qual) {
        return error;
    } else {
        if (auto res = symbols.find(ident.ident); res != symbols.end()) {
            if (!secretly) const_cast<SymbolTable*>(this)->symbols[ident.ident].count++;
            return SymbolToken(res->second);
        } else {
            return error;
        }
    }
}

SemResult<nodes::ExpressionPtr> SymbolTable::get_value(const nodes::QualIdent& ident, bool secretly) const {
    auto error =  ErrorBuilder(ident.ident.place).format("Symbol {} not found", ident).build();
    if (ident.qual) {
        return error;
    } else {
        if (auto res = values.find(ident.ident); res != values.end()) {
            if (!secretly) const_cast<SymbolTable*>(this)->symbols[ident.ident].count++;
            return nodes::ExpressionPtr(res->second);
        } else {
            return error;
        }
    }
}
SemResult<TablePtr> SymbolTable::get_table(const nodes::QualIdent& ident, bool secretly) const {
    auto error =  ErrorBuilder(ident.ident.place).format("Symbol {} not found", ident).build();
    if (ident.qual) {
        return error;
    } else {
        if (auto res = tables.find(ident.ident); res != tables.end()) {
            if (!secretly) const_cast<SymbolTable*>(this)->symbols[ident.ident].count++;
            return TablePtr(res->second);
        } else {
            return error;
        }
    }
}

bool SymbolTable::type_extends_base(const nodes::Type* extension, nodes::QualIdent base) const {
    if (auto isRecord = dynamic_cast<const nodes::RecordType*>(extension); isRecord && isRecord->basetype) {
        return type_hierarchy.extends(*isRecord->basetype, base);
    } else if (auto isName = dynamic_cast<const nodes::TypeName*>(extension); isName) {
        return type_hierarchy.extends(isName->ident, base);
    } else return false;
}

std::string SymbolTable::to_string() const {
    std::string result;
    result += fmt::format("Symbols ({}):\n", symbols.size());
    for (auto [name, symbol]: symbols) {
        result += fmt::format("{}: {}\n", name, symbol);
    }
    result += fmt::format("Values ({}):\n", values.size());
    for (auto [name, value]: values) {
        result += fmt::format("{}: {}\n", name, value->to_string());
    }
    result += fmt::format("Tables ({}):\n", tables.size());
    for (auto [name, table]: tables) {
        result += fmt::format("{}:\n{}\n", name, table->to_string());
    }
    return result;
}
