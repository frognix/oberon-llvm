#pragma once

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "nodes.hpp"

template <>
struct fmt::formatter<std::vector<char>> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(std::vector<char> const& vec, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", fmt::join(vec, ""));
    }
};

template <>
struct fmt::formatter<nodes::TypePtr> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::TypePtr const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", id->to_string());
    }
};

template <>
struct fmt::formatter<nodes::ExpressionPtr> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::ExpressionPtr const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", id->to_string());
    }
};

template <>
struct fmt::formatter<nodes::StatementPtr> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::StatementPtr const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", id->to_string());
    }
};

template <>
struct fmt::formatter<nodes::SectionPtr> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::SectionPtr const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", id->to_string());
    }
};

template <>
struct fmt::formatter<nodes::SetElement> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::SetElement const& id, FormatContext& ctx) {
        if (id.second)
            return fmt::format_to(ctx.out(), "{}..{}", id.first, *id.second);
        else
            return fmt::format_to(ctx.out(), "{}", id.first);
    }
};

template <>
struct fmt::formatter<nodes::QualIdent> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::QualIdent const& id, FormatContext& ctx) {
        if (id.qual)
            return fmt::format_to(ctx.out(), "{}.{}", *id.qual, id.ident);
        else
            return fmt::format_to(ctx.out(), "{}", id.ident);
    }
};

template <>
struct fmt::formatter<nodes::IdentDef> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::IdentDef const& id, FormatContext& ctx) {
        if (id.def)
            return fmt::format_to(ctx.out(), "{}*", id.ident);
        else
            return fmt::format_to(ctx.out(), "{}", id.ident);
    }
};

template <>
struct fmt::formatter<nodes::IdentList> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::IdentList const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", fmt::join(id, ", "));
    }
};

template <>
struct fmt::formatter<nodes::FieldList> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::FieldList const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{} : {}", id.list, id.type);
    }
};

template <>
struct fmt::formatter<nodes::FieldListSequence> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::FieldListSequence const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", fmt::join(id, "; "));
    }
};

template <>
struct fmt::formatter<nodes::Selector> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::Selector const& id, FormatContext& ctx) {
        if (auto sel = std::get_if<nodes::Ident>(&id); sel) {
            return fmt::format_to(ctx.out(), ".{}", *sel);
        } else if (auto sel = std::get_if<nodes::ExpList>(&id); sel) {
            return fmt::format_to(ctx.out(), "[{}]", fmt::join(*sel, ", "));
        } else if (auto sel = std::get_if<char>(&id); sel) {
            return fmt::format_to(ctx.out(), "{}", *sel);
        } else if (auto sel = std::get_if<nodes::QualIdent>(&id); sel) {
            return fmt::format_to(ctx.out(), "({})", *sel);
        } else
            throw std::runtime_error("Bad variant");
    }
};

template <>
struct fmt::formatter<nodes::Designator> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::Designator const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}{}", id.ident, fmt::join(id.selector, ""));
    }
};

template <>
struct fmt::formatter<nodes::ConstDecl> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::ConstDecl const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{} = {}", id.ident, id.expression);
    }
};

template <>
struct fmt::formatter<nodes::TypeDecl> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::TypeDecl const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{} = {}", id.ident, id.type);
    }
};

template <>
struct fmt::formatter<nodes::StatementSequence> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::StatementSequence const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", fmt::join(id, ";\n"));
    }
};

template <>
struct fmt::formatter<nodes::IfBlock> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::IfBlock const& id, FormatContext& ctx) {
        auto [cond, block] = id;
        return fmt::format_to(ctx.out(), "ELSIF {} THEN\n{}\n", cond, block);
    }
};

template <>
struct fmt::formatter<nodes::CaseLabel> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::CaseLabel const& id, FormatContext& ctx) {
        if (id.second)
            return fmt::format_to(ctx.out(), "{}..{}", id.first, *id.second);
        return fmt::format_to(ctx.out(), "{}", id.first);
    }
};

template <>
struct fmt::formatter<nodes::CaseLabelList> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::CaseLabelList const& id, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}", fmt::join(id, ", "));
    }
};

template <>
struct fmt::formatter<nodes::Case> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::Case const& id, FormatContext& ctx) {
        auto [label, state] = id;
        return fmt::format_to(ctx.out(), "{} : {}", label, state);
    }
};

template <>
struct fmt::formatter<nodes::DeclarationSequence> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::DeclarationSequence const& id, FormatContext& ctx) {
        std::string constDecl, typeDecl, variableDecl, procDecl;
        if (!id.constDecls.empty())
            constDecl = fmt::format("CONST {};\n", fmt::join(id.constDecls, ";\n"));
        if (!id.typeDecls.empty())
            typeDecl = fmt::format("TYPE {};\n", fmt::join(id.typeDecls, ";\n"));
        if (!id.variableDecls.empty())
            variableDecl = fmt::format("VAR {};\n", fmt::join(id.variableDecls, ";\n"));
        if (!id.procedureDecls.empty())
            procDecl = fmt::format("{};\n", fmt::join(id.procedureDecls, ";\n"));
        return fmt::format_to(ctx.out(), "{}{}{}{}", constDecl, typeDecl, variableDecl, procDecl);
    }
};

template <>
struct fmt::formatter<nodes::FormalType> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::FormalType const& id, FormatContext& ctx) {
        if (id.array)
            return fmt::format_to(ctx.out(), "ARRAY OF {}", id.ident);
        else
            return fmt::format_to(ctx.out(), "{}", id.ident);
    }
};

template <>
struct fmt::formatter<nodes::FPSection> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::FPSection const& id, FormatContext& ctx) {
        if (id.var)
            return fmt::format_to(ctx.out(), "{} {} : {}", *id.var, fmt::join(id.idents, ", "), id.type);
        else
            return fmt::format_to(ctx.out(), "{} : {}", fmt::join(id.idents, ", "), id.type);
    }
};

template <>
struct fmt::formatter<nodes::FormalParameters> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::FormalParameters const& id, FormatContext& ctx) {
        if (id.rettype)
            return fmt::format_to(ctx.out(), "({}) : {}", fmt::join(id.sections, "; "), *id.rettype);
        else
            return fmt::format_to(ctx.out(), "({})", fmt::join(id.sections, "; "));
    }
};

template <>
struct fmt::formatter<nodes::Import> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(nodes::Import const& id, FormatContext& ctx) {
        if (id.real_name == id.name)
            return fmt::format_to(ctx.out(), "{}", id.name);
        else
            return fmt::format_to(ctx.out(), "{} := {}", id.name, id.real_name);
    }
};
