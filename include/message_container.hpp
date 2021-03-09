#pragma once

#include "plib/code_stream.hpp"
#include <cstddef>
#include "plib/format.hpp"

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
    MessageContainer() {}
    template <class... Args>
    void addFormat(MPriority priority, CodePlace place, const char* str, const Args&... args) noexcept {
        addMessage({priority, place, fmt::format(str, args...)});
    }
    template <class... Args>
    void addErr(CodePlace place, const char* str, const Args&... args) noexcept {
        addMessage({MPriority::ERR, place, fmt::format(str, args...)});
    }
    void addMessage(Message message) {
        messages.push_back(message);
    }
    const std::vector<Message>& get_messages() { return messages; };
private:
    std::vector<Message> messages;
};
