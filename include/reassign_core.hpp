#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <vector>

class counter {
  public:
    counter() noexcept : ptr(nullptr) {}

    void* get() const noexcept { return ptr; }

    counter(const counter&) = default;

  private:
    friend class reassign_core;
    void* ptr;
    size_t count;
};

class reassign_core {
  public:
    static reassign_core& get_instance() noexcept;

    ~reassign_core();

    void print_counters();

    counter* get_new_counter() noexcept;

    counter* copy(counter* counter) noexcept;

    void set(counter* counter, class counter* other) noexcept;

    // counter* move(class counter* other) noexcept {}

    template <class T>
    counter* get_new_counter(T&& value) noexcept;
    template <class T>
    void remove(counter* counter) noexcept;
    template <class T>
    void remove_data_ptr(counter* counter);
    template <class T>
    void move(counter* counter, class counter* other) noexcept;

    static size_t copy_count;

  private:
    reassign_core();
    counter* get_empty() noexcept;
    static std::unique_ptr<reassign_core> instance;

    std::map<void*, size_t> count;
    std::queue<std::pair<size_t, size_t>> empty;
    std::vector<std::array<counter, 256>*> counters;
};

template <class T>
counter* reassign_core::get_new_counter(T&& value) noexcept {
    auto counter = get_new_counter();
    counter->ptr = new T(std::forward<T>(value));
    count[counter->ptr] = 1;
    return counter;
}

template <class T>
void reassign_core::remove(counter* counter) noexcept {
    if (counter == nullptr)
        return;
    counter->count--;
    if (counter->count == 0) {
        remove_data_ptr<T>(counter);
    }
}

template <class T>
void reassign_core::remove_data_ptr(counter* counter) {
    if (counter->ptr != nullptr) {
        auto& val = count[counter->ptr];
        val--;
        if (val == 0) {
            delete static_cast<T*>(counter->ptr);
            count.erase(counter->ptr);
            counter->ptr = nullptr;
        }
    }
}

template <class T>
void reassign_core::move(counter* counter, class counter* other) noexcept {
    if (counter == other)
        return;
    remove_data_ptr<T>(counter);
    counter->ptr = other->ptr;
    other->ptr = nullptr;
}
