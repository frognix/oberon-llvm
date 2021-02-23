#include "reassign_core.hpp"

std::unique_ptr<reassign_core> reassign_core::instance{};

size_t reassign_core::copy_count = 0;

reassign_core::reassign_core() : counters() {}

reassign_core::~reassign_core() {
    std::cout << "all copy count = " << copy_count << std::endl;
    for (auto elem : counters) {
        delete elem;
    }
}

void reassign_core::print_counters() {
    for (auto elem : counters) {
        for (auto counter : *elem) {
            std::cout << counter.count << " : " << counter.ptr << ", ";
        }
        std::cout << "\nend\n";
    }
}

counter* reassign_core::get_new_counter() noexcept {
    return get_empty();
}

counter* reassign_core::copy(counter* counter) noexcept {
    if (counter == nullptr)
        return nullptr;
    counter->count++;
    copy_count++;
    return counter;
}

void reassign_core::set(counter* counter, class counter* other) noexcept {
    if (counter == other)
        return;
    counter->ptr = other->ptr;
    count[counter->ptr]++;
}

reassign_core& reassign_core::get_instance() noexcept {
    if (instance == nullptr) {
        instance.reset(new reassign_core());
    }
    return *instance;
}

counter* reassign_core::get_empty() noexcept {
    if (empty.empty()) {
        for (size_t x = 0; x < counters.size(); x++) {
            for (size_t y = 0; y < 256; y++) {
                if (counters[x]->at(y).count == 0) {
                    empty.push({x, y});
                }
            }
        }
        if (empty.empty()) {
            counters.push_back(new std::array<counter, 256>());
            for (size_t i = 0; i < 30; ++i) {
                empty.push({counters.size() - 1, i});
            }
        }
    }
    auto [x, y] = empty.front();
    empty.pop();
    auto& it = counters[x]->at(y);
    it.count = 1;
    return &it;
}
