#include "section_nodes.hpp"
#include "node_formatters.hpp"

using namespace nodes;

std::string ProcedureDeclaration::to_string() const {
    if (body) {
        std::string statements;
        if (!body->statements.empty())
            statements = fmt::format("BEGIN\n{}\n", body->statements);
        std::string ret_str;
        if (body->ret)
            ret_str = fmt::format("RETURN {}\n", *body->ret);
        return fmt::format("PROCEDURE {} {};\n{}{}{}END {}", name, type.params, body->decls, statements, ret_str, name);
    } else {
        return fmt::format("PROCEDURE {} {} := 0", name, type.params);
    }
}

Import::Import(Ident first, std::optional<Ident> second) : name(first) {
    if (second) {
        real_name = *second;
    } else {
        real_name = name;
    }
}

std::string Module::to_string() const {
    return fmt::format("MODULE {}; IMPORT {}\n{}\nBEGIN\n{}\nEND {}.", name, fmt::join(imports, ", "), declarations,
                       body, name);
}

std::string ProcedureDefinition::to_string() const {
    return fmt::format("PROCEDURE {} {};", name, type.to_string());
}

std::string Definition::to_string() const {
    return fmt::format("DEFINITION {}; IMPORT {}\n definitions END {}", name, fmt::join(imports, ", "), name);
}
