#pragma once

#include "symbol_table.hpp"
#include "nodes.hpp"
#include <functional>

class ProcedureTable;

class MessageContainer;

using ParseReturnType = std::optional<std::unique_ptr<SemanticUnit>>;

class SymbolContainer {
public:
    SymbolContainer() {}
    static bool parse(SymbolContainer& table, nodes::Context& context, const nodes::DeclarationSequence& seq, nodes::StatementSequence body, std::function<bool(nodes::IdentDef,nodes::Context&)> func);

    //SymbolTableI
    Maybe<SymbolToken> get_symbol(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const;
    Maybe<nodes::ValuePtr> get_value(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const;
    Maybe<TablePtr> get_table(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly = false) const;
    bool add_symbol(MessageContainer&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type);
    bool add_value(MessageContainer&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, nodes::ValuePtr value);
    bool add_table(MessageContainer&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, TablePtr table);

    //CodeSectionI
    bool analyze_code(nodes::Context& messages) const;

    bool has_symbol(const nodes::QualIdent& ident) const { return symbols.contains(ident.ident); }

    std::string to_string() const;
private:
    SymbolMap<SymbolToken> symbols;
    SymbolMap<nodes::ValuePtr> values;
    SymbolMap<TablePtr> tables;
    nodes::StatementSequence body;
};
