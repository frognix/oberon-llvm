#pragma once

#include "symbol_table_i.hpp"
#include "nodes.hpp"

class ProcedureTable;

class MessageContainer;

using ParseReturnType = std::optional<std::unique_ptr<SemanticUnitI>>;

class SymbolTable;

using TablePtr = std::shared_ptr<SemanticUnitI>;

class SymbolTable {
public:
    // SymbolTable(nodes::StatementSequence b);
    SymbolTable() {}
    static bool parse(SymbolTable& table, nodes::Context& context, const nodes::DeclarationSequence& seq, nodes::StatementSequence body, std::function<bool(nodes::IdentDef,nodes::Context&)> func);

    //SymbolTableI
    Maybe<SymbolToken> get_symbol(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const;
    Maybe<nodes::ValuePtr> get_value(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const;
    bool add_symbol(MessageContainer&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type);
    bool add_value(MessageContainer&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, nodes::ValuePtr value);
    bool add_table(MessageContainer&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, TablePtr table);

    //CodeSectionI
    bool analyze_code(nodes::Context& messages) const;

    bool has_symbol(const nodes::QualIdent& ident) const { return symbols.contains(ident.ident); }

    std::string to_string() const;
protected:
private:
    SymbolMap<SymbolToken> symbols;
    SymbolMap<nodes::ValuePtr> values;
    SymbolMap<TablePtr> tables;
    nodes::StatementSequence body;
};
