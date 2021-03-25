#pragma once

#include "expression.hpp"
#include "expression_nodes.hpp"

class ConstValue {
public:
    ConstValue() {}
    ConstValue(nodes::ExpressionPtr expr) : m_value(expr) {}
    Maybe<nodes::ValuePtr> get(nodes::Context& context) const {
        if (!m_value->is<nodes::Value>()) {
            auto res = m_value->eval_constant(context);
            if (!res) return error;
            res.value()->place = m_value->place;
            m_value = res.value();
        }
        return std::static_pointer_cast<nodes::Value>(m_value);
    }
    nodes::ExpressionPtr get_expression() const {
        return m_value;
    }
private:
    mutable nodes::ExpressionPtr m_value;
};

template <>
struct fmt::formatter<ConstValue> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(ConstValue const& value, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", value.get_expression()->to_string());
    }
};
