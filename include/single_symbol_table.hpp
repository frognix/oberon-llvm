#pragma once

#include "symbol_table_i.hpp"
#include "symbol_token.hpp"

class SingleSymbolTable : public SymbolTableI {
public:
    SingleSymbolTable(SymbolToken symbol, const SymbolTableI& parent) : m_symbol(symbol), m_parent(parent) {}

    Maybe<SymbolToken> get_symbol(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly = false) const {
        if (ident == m_symbol.name) return m_symbol;
        else return m_parent.get_symbol(messages, ident, secretly);
    }

    Maybe<nodes::ValuePtr> get_value(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly = false) const {
        return m_parent.get_value(messages, ident, secretly);
    }

    std::string to_string() const { return m_parent.to_string(); }

private:
    SymbolToken m_symbol;
    const SymbolTableI& m_parent;
};
