#include "statement_nodes.hpp"
#include "expression_nodes.hpp"
#include "node_formatters.hpp"

#include "semantic_context.hpp"
#include "symbol_table.hpp"

using namespace nodes;

std::string Assignment::to_string() const {
    return fmt::format("{} := {}", variable.get().to_string(), value);
}

bool Assignment::check(Context& context) const {
    if (!variable.repair(context))
        return berror;
    auto validIdent = variable.get();
    auto lsymbol = validIdent.get_symbol(context, place);
    if (!lsymbol)
        return berror;
    if (lsymbol->group != SymbolGroup::VAR) {
        context.messages.addErr(place, "Expected variable");
        return berror;
    }
    auto rtype = value->get_type(context);
    if (!rtype)
        return berror;
    auto lltype = lsymbol->type;
    auto rrtype = *rtype;
    auto rbuiltin = rrtype->is<BuiltInType>();
    if (*lltype == *rrtype || (lltype->is<PointerType>() && rbuiltin && rbuiltin->type == BaseType::NIL)) {
        return bsuccess;
    } else {
        context.messages.addErr(place, "Incompatible types in assignment: {} and {}", lsymbol->type->to_string(),
                                rtype.value()->to_string());
        return berror;
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

bool IfStatement::check(Context&) const {
    return bsuccess;
}

std::string CaseStatement::to_string() const {
    return fmt::format("CASE {} OF {} END", expression, fmt::join(cases, " |\n"));
}

bool CaseStatement::check(Context&) const {
    return bsuccess;
}

std::string WhileStatement::to_string() const {
    auto [cond, block] = if_blocks[0];
    std::vector<IfBlock> elsif_blocks = {if_blocks.begin() + 1, if_blocks.end()};
    return fmt::format("WHILE {} DO\n{}\n{}END", cond, block, fmt::join(elsif_blocks, "\n"));
}

bool WhileStatement::check(Context&) const {
    return bsuccess;
}

std::string RepeatStatement::to_string() const {
    auto [cond, block] = if_block;
    return fmt::format("REPEAT {} UNTIL\n{}", block, cond);
}

bool RepeatStatement::check(Context&) const {
    return bsuccess;
}

std::string ForStatement::to_string() const {
    if (by_expr)
        return fmt::format("FOR {} := {} TO {} BY {} DO\n{}\nEND", ident, for_expr, to_expr, *by_expr, block);
    else
        return fmt::format("FOR {} := {} TO {} DO\n{}\nEND", ident, for_expr, to_expr, block);
}

bool ForStatement::check(Context&) const {
    return bsuccess;
}

std::string CallStatement::to_string() const {
    return fmt::format("{}", call.to_string());
}

bool CallStatement::check(Context& context) const {
    auto info = call.get_info(context);
    if (!info)
        return berror;
    if (info->type != nullptr) {
        context.messages.addErr(place, "Expected procedure call without return type");
        return berror;
    }
    return bsuccess;
}
