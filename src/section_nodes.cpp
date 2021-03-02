#include "section_nodes.hpp"
#include "node_formatters.hpp"

using namespace nodes;

std::string ProcedureDeclaration::to_string() const {
    std::string statements;
    if (!body.empty())
        statements = fmt::format("BEGIN\n{}\n", body);
    std::string ret_str;
    if (ret)
        ret_str = fmt::format("RETURN {}\n", *ret);
    return fmt::format("PROCEDURE {} {};\n{}{}{}END {}", name, type.params, decls, statements, ret_str, name);
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
