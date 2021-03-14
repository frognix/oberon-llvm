#pragma once

#include "symbol_table_i.hpp"

class ModuleTableI : public SemanticUnitI {
public:
    virtual Maybe<SymbolToken> get_symbol_out(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const = 0;
};

using ModuleTablePtr = ModuleTableI const*;

struct Import {
    nodes::Ident name;
    ModuleTablePtr module;
};
