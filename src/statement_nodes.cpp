#include "statement_nodes.hpp"
#include "expression_nodes.hpp"
#include "node_formatters.hpp"

#include "semantic_context.hpp"
#include "symbol_table.hpp"
#include "type.hpp"

using namespace nodes;

bool nodes::check_statements(Context& context, const StatementSequence& seq) {
    bool result = true;
    for (auto& statement : seq) {
        if (!statement->check(context))
            result = false;
    }
    return result;
}

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
    auto& lltype = *lsymbol->type;
    auto& [group, rrtype] = *rtype;
    if (assignment_compatible_types(context, lltype, *rrtype)) {
        return bsuccess;
    } else {
        context.messages.addErr(place, "Incompatible types in assignment: {} and {}", lsymbol->type->to_string(),
                                rrtype->to_string());
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

bool check_if_block(Context& context, const IfBlock& if_block) {
    auto [expr, statements] = if_block;
    auto exprRes = expr->get_type(context);
    if (!exprRes) {
        return false;
    }
    auto [group, type] = *exprRes;
    bool result = true;
    if (auto base = type->is<BuiltInType>(); base && base->equal_to(BaseType::BOOL)) {
        if (!check_statements(context, statements)) {
            result = false;
        }
    } else {
        context.messages.addErr(expr->place, "Expected expression fo BOOLEAN type");
        result = false;
    }
    return result;
}

bool IfStatement::check(Context& context) const {
    bool result = true;
    for (auto& if_block : if_blocks) {
        if (!check_if_block(context, if_block))
            result = false;
    }
    if (else_block && !check_statements(context, *else_block)) {
        result = false;
    }
    return result;
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

bool WhileStatement::check(Context& context) const {
    bool result = true;
    for (auto& if_block : if_blocks) {
        if (!check_if_block(context, if_block))
            result = false;
    }
    return result;
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
    if (!info) return berror;
    auto [group, type] = *info;
    if (type != nullptr) {
        context.messages.addErr(place, "Expected procedure call without return type");
        return berror;
    }
    return bsuccess;
}
