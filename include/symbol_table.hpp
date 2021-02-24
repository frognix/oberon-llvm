#pragma once

#include "parser.hpp"
#include <map>

class Procedure;

class SymbolTable {
private:
    std::map<nodes::Ident, nodes::Expression> consts;
    std::map<nodes::Ident, nodes::Type> types;
    std::map<nodes::Ident, nodes::Type> variables;
    std::map<nodes::Ident, Procedure> procedures;
};

class Procedure {
    nodes::Ident name;
    nodes::ProcedureType type;
    SymbolTable symbols;
    nodes::StatementSequence body;
    std::optional<nodes::ExpressionPtr> ret;
};

class Module;

struct Import {
    nodes::Ident name;
    Module* ptr;
};

class ModuleTable : private SymbolTable {
    std::map<nodes::Ident, Import> imports;
};

class Module {};
