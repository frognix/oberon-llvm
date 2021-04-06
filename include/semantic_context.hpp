#pragma once

#include "message_container.hpp"

class SymbolTable;

struct SemanticContext {
    SemanticContext(MessageContainer& m, const SymbolTable& s) : messages(m), symbols(s) {}
    MessageContainer& messages;
    const SymbolTable& symbols;
};

template <class T>
using Maybe = std::optional<T>;

const auto error = std::nullopt;
const auto berror = false;
const auto bsuccess = true;
