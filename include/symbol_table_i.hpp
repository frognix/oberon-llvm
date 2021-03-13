#pragma once

#include "plib/result.hpp"
#include "symbol_token.hpp"

class SymbolTableI {
public:
    virtual ~SymbolTableI() {}

    virtual Maybe<SymbolToken> get_symbol(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const = 0;
    virtual Maybe<nodes::ExpressionPtr> get_value(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const = 0;

    virtual std::string to_string() const = 0;
};

class CodeSectionI {
public:
    virtual bool analyze_code(MessageContainer& messages) const = 0;
    virtual ~CodeSectionI() {}
};

struct SemanticUnitI : public SymbolTableI, public CodeSectionI {};
