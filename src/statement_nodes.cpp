#include "statement_nodes.hpp"
#include "expression_nodes.hpp"
#include "node_formatters.hpp"

#include "symbol_table.hpp"

using namespace nodes;

std::string Assignment::to_string() const {
    return fmt::format("{} := {}", variable.get().to_string(), value);
}

Error Assignment::check(const SymbolTable& table) const {
    auto err = variable.repair(table);
    if (err) return err;
    auto validIdent = variable.get();
    auto lsymbol = validIdent.get_symbol(table, place);
    if (!lsymbol)
        return lsymbol.get_err();
    if (lsymbol.get_ok().group != SymbolGroup::VAR)
        return ErrorBuilder(this->place).format("Expected variable").build();
    auto rtype = value->get_type(table);
    if (!rtype)
        return rtype.get_err();
    auto lltype = lsymbol.get_ok().type;
    auto rrtype = rtype.get_ok();
    auto rbuiltin = rrtype->is<BuiltInType>();
    if (*lltype == *rrtype || (lltype->is<PointerType>() && rbuiltin && rbuiltin->type == BaseType::NIL)) {
        return {};
    } else {
        return ErrorBuilder(place)
            .format("Incompatible types in assignment: {} and {}", lsymbol.get_ok().type->to_string(),
                    rtype.get_ok()->to_string())
            .build();
    }
}

std::string IfStatement::to_string() const {
    auto [cond, block] = if_blocks[0];
    std::vector<IfBlock> elsif_blocks = {if_blocks.begin() + 1, if_blocks.end()};
    if (else_block)
        return fmt::format("IF {} THEN\n{}\n{}ELSE {}\nEND", cond, block, fmt::join(elsif_blocks, "\n"), *else_block);
    else
        return fmt::format("IF {} THEN\n{}\n{}END", cond, block, fmt::join(elsif_blocks, "\n"));
}

Error IfStatement::check(const SymbolTable&) const {
    return {};
}

std::string CaseStatement::to_string() const {
    return fmt::format("CASE {} OF {} END", expression, fmt::join(cases, " |\n"));
}

Error CaseStatement::check(const SymbolTable&) const {
    return {};
}

std::string WhileStatement::to_string() const {
    auto [cond, block] = if_blocks[0];
    std::vector<IfBlock> elsif_blocks = {if_blocks.begin() + 1, if_blocks.end()};
    return fmt::format("WHILE {} DO\n{}\n{}END", cond, block, fmt::join(elsif_blocks, "\n"));
}

Error WhileStatement::check(const SymbolTable&) const {
    return {};
}

std::string RepeatStatement::to_string() const {
    auto [cond, block] = if_block;
    return fmt::format("REPEAT {} UNTIL\n{}", block, cond);
}

Error RepeatStatement::check(const SymbolTable&) const {
    return {};
}

std::string ForStatement::to_string() const {
    if (by_expr)
        return fmt::format("FOR {} := {} TO {} BY {} DO\n{}\nEND", ident, for_expr, to_expr, *by_expr, block);
    else
        return fmt::format("FOR {} := {} TO {} DO\n{}\nEND", ident, for_expr, to_expr, block);
}

Error ForStatement::check(const SymbolTable&) const {
    return {};
}

std::string CallStatement::to_string() const {
    return fmt::format("{}", call.to_string());
}

Error CallStatement::check(const SymbolTable& table) const {
    auto info = call.get_info(table);
    if (!info)
        return info.get_err();
    if (info.get_ok().type != nullptr)
        return ErrorBuilder(place).text("Expected procedure call without return type").build();
    return {};
}
