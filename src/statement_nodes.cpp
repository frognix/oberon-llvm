#include "statement_nodes.hpp"
#include "expression_nodes.hpp"
#include "message_container.hpp"
#include "node_formatters.hpp"

#include "semantic_context.hpp"
#include "symbol_table.hpp"
#include "type.hpp"

using namespace nodes;

inline bool check_statements(Context& context, const StatementSequence& seq) {
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
    if (lltype.assignment_compatible(context, *rrtype)) {
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

bool CaseStatement::check(Context& context) const {
    if (cases.size() == 0) return true;
    auto label_index = std::get<0>(cases[0]).front().first.index();
    auto expr_res = expression->get_type(context);
    if (!expr_res) return berror;
    auto [expr_group, expr_type] = expr_res.value();
    auto expr_basetype = expr_type->is<BuiltInType>();

    const Type* expr_record = nullptr;
    if (label_index == 2) {
        if (auto pointer_type = expr_type->is<PointerType>(); pointer_type) {
            expr_record = &pointer_type->get_type(context);
        } else if (auto record_type = expr_type->is<RecordType>(); record_type) {
            expr_record = record_type;
        } else {
            context.messages.addErr(expression->place, "Expected Record or Pointer type for type labels");
            return berror;
        }
    }

    auto result = bsuccess;
    for (auto& [label_list, statements] : cases) {
        for (auto& label : label_list) {
            if (label.first.index() != label_index || (label.second && label.second->index() != label_index)) {
                context.messages.addErr(place, "Different label types in case statement");
                result = berror;
            } else if (label_index == 0 && (!expr_basetype || expr_basetype->type != BaseType::INTEGER)) {
                context.messages.addErr(place, "Expected expression of INTEGER type");
                result = berror;
            } else if (label_index == 1 && (!expr_basetype || expr_basetype->type != BaseType::CHAR)) {
                context.messages.addErr(place, "Expected expression of CHAR type");
                result = berror;
            } else if (label_index == 2) {
                if (label.second) {
                    context.messages.addErr(place, "Unexpected label range for type labels");
                    result = berror;
                    continue;
                }
                auto ident = std::get<2>(label.first);
                auto symbol = context.symbols.get_symbol(context.messages, ident);
                if (!symbol) {
                    result = berror;
                    continue;
                }
                if (!expr_record->assignment_compatible(context, *symbol->type)) {
                    context.messages.addErr(ident.ident.place, "Expected extension of type: {}", expr_type->to_string());
                    result = berror;
                }
            }
        }
        if (!check_statements(context, statements)) result = berror;
    }
    return result;
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

bool RepeatStatement::check(Context& context) const {
    return check_if_block(context, if_block);
}

std::string ForStatement::to_string() const {
    if (by_expr)
        return fmt::format("FOR {} := {} TO {} BY {} DO\n{}\nEND", ident, for_expr, to_expr, *by_expr, block);
    else
        return fmt::format("FOR {} := {} TO {} DO\n{}\nEND", ident, for_expr, to_expr, block);
}

bool ForStatement::check(Context& context) const {
    if (by_expr) {
        auto value = by_expr->get(context);
        if (!value) return berror;
        if (!value.value()->is<IntegerValue>()) {
            context.messages.addErr(value.value()->place, "Expected integer");
            return berror;
        }
    }
    auto symbol_res = context.symbols.get_symbol(context.messages, QualIdent{{}, ident});
    if (!symbol_res) return berror;
    if (auto type = symbol_res->type->is<BuiltInType>(); type && type->equal_to(BaseType::INTEGER)) {
        bool error = false;
        for (auto statement : block) {
            if (!statement->check(context)) error = true;
        }
        return !error;
    } else {
        context.messages.addErr(ident.place, "Expected variable of INTEGER type");
        return berror;
    }
}

std::string CallStatement::to_string() const {
    return fmt::format("{}", call->to_string());
}

bool CallStatement::check(Context& context) const {
    auto pair = call->get_type(context);
    if (!pair) return berror;
    auto [group, type] = *pair;
    if (auto base = type->is<BuiltInType>(); !base || base->type != BaseType::VOID)
        context.messages.addFormat(MPriority::W3, place, "Unused built-in procedure result value");
    return bsuccess;
}
