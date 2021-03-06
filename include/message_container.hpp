#pragma once

#include "libparser/code_stream.hpp"
#include <cstddef>
#include <ostream>

class IOManager;

enum MPriority : char {
    ERR, W1, W2, W3, W4
};

struct Message {
    Message(MPriority p, CodePlace pl, std::string t) : priority(p), place(pl), text(t) {}
    MPriority priority;
    CodePlace place;
    std::string text;
};

class MessageContainer {
public:
    MessageContainer(std::ostream& s, CodeStream& c) : m_stream(s), m_code(c) {}
    template <class... Args>
    void addFormat(MPriority priority, CodePlace place, const char* str, const Args&... args) noexcept {
        addMessage({priority, place, fmt::format(str, args...)});
    }
    template <class... Args>
    void addErr(CodePlace place, const char* str, Args&&... args) noexcept {
        addMessage({MPriority::ERR, place, fmt::format(str, std::forward<Args>(args)...)});
    }
    void addMessage(Message message);
private:
    std::ostream& m_stream;
    CodeStream& m_code;
};
