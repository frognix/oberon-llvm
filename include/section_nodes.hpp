#pragma once

#include "section.hpp"
#include "type.hpp"
#include "expression.hpp"

namespace nodes {

struct ConstDecl {
    IdentDef ident;
    ExpressionPtr expression;
};

struct TypeDecl {
    IdentDef ident;
    TypePtr type;
};

using VariableDecl = FieldList;

struct DeclarationSequence {
    std::vector<ConstDecl> constDecls;
    std::vector<TypeDecl> typeDecls;
    std::vector<VariableDecl> variableDecls;
    std::vector<SectionPtr> procedureDecls;
};

struct ProcedureDeclaration : Section {
    std::string to_string() const {
        std::string statements;
        if (!body.empty())
            statements = fmt::format("BEGIN\n{}\n", body);
        std::string ret_str;
        if (ret)
            ret_str = fmt::format("RETURN {}\n", *ret);
        return fmt::format("PROCEDURE {} {};\n{}{}{}END {}", name, type.params, decls, statements, ret_str, name);
    }
    ProcedureDeclaration() {}
    ProcedureDeclaration(IdentDef n, ProcedureType t, DeclarationSequence d, StatementSequence b,
                         std::optional<ExpressionPtr> r)
        : name(n), type(t), decls(d), body(b), ret(r) {}
    IdentDef name;
    ProcedureType type;
    DeclarationSequence decls;
    StatementSequence body;
    std::optional<ExpressionPtr> ret;
};

struct Import {
    Import() {}
    Import(Ident first, std::optional<Ident> second) : name(first) {
        if (second) {
            real_name = *second;
        } else {
            real_name = name;
        }
    }
    Ident name;
    Ident real_name;
};

using ImportList = std::vector<Import>;

struct Module : Section {
    std::string to_string() const {
        return fmt::format("MODULE {}; IMPORT {}\n{}\nBEGIN\n{}\nEND {}.", name, fmt::join(imports, ", "), declarations,
                           body, name);
    }
    Module() {}
    Module(Ident n, ImportList i, DeclarationSequence d, StatementSequence b)
        : name(n), imports(i), declarations(d), body(b) {}
    Ident name;
    ImportList imports;
    DeclarationSequence declarations;
    StatementSequence body;
};

}
