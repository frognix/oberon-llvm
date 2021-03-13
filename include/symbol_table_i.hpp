#pragma once

#include "plib/result.hpp"
#include "symbol_token.hpp"

class SymbolTableI {
public:
    virtual ~SymbolTableI() {}

    virtual Maybe<SymbolToken> get_symbol(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const = 0;
    virtual Maybe<nodes::ExpressionPtr> get_value(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const = 0;
    // virtual Maybe<SymbolTableI> get_table(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const = 0;

    // virtual bool add_symbol(MessageContainer&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type) = 0;
    // virtual bool add_value(MessageContainer&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, nodes::ExpressionPtr value) = 0;
    // virtual bool add_table(MessageContainer&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, TablePtr table) = 0;

    virtual bool type_extends_base(const nodes::Type* extension, nodes::QualIdent base) const = 0;

    virtual std::string to_string() const = 0;
};

class CodeSectionI {
public:
    virtual bool analyze_code(MessageContainer& messages) const = 0;
    virtual ~CodeSectionI() {}
};

struct SemanticUnitI : public SymbolTableI, public CodeSectionI {};
