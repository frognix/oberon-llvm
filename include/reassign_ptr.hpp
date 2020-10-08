#include <vector>
#include <map>
#include <iostream>

class counter {
public:
    counter() noexcept : ptr(nullptr) {}

    void* get() const { return ptr; }

    counter(const counter&) = default;
private:
    friend class reassignment;
    void* ptr;
    size_t count;
};

class reassignment {
public:
    counter* get_new_counter() {
        if (y == 255) {
            y = 0;
            x++;
            counters.push_back(new std::array<counter, 256>());
        }
        auto& it = counters[x]->at(y);
        y++;
        return &it;
    }

    template <class T>
    counter* get_new_counter(T&& value) {
        auto counter = get_new_counter();
        counter->ptr = new T(std::forward<T>(value));
        counter->count = 1;
        count[counter->ptr] = 1;
        return counter;
    }

    counter* copy(counter* counter) {
        if (counter == nullptr) return nullptr;
        counter->count++;
        return counter;
    }

    template <class T>
    void remove(counter* counter) {
        if (counter->ptr == nullptr) return;
        counter->count--;
        if (counter->count == 0) {
            auto& val = count[counter->ptr];
            val--;
            if (val == 0) {
                delete static_cast<T*>(counter->ptr);
                count.erase(counter->ptr);
            }
        }
    }

    template <class T>
    void set(counter* counter, class counter* other) {
        if (counter == other) return;
        if (counter != nullptr)
            remove<T>(counter);
        counter->ptr = other->ptr;
        count[counter->ptr]++;
    }

    static reassignment& get_instance() {
        if (instance == nullptr)
            instance = new class reassignment();
        return *instance;
    }
private:
    static reassignment* instance;
    reassignment() : counters() {
        counters.push_back(new std::array<counter, 256>());
    }
    std::map<void*, size_t> count;
    size_t x = 0;
    size_t y = 0;
    std::vector<std::array<counter, 256>*> counters;
};

template <class T>
class reassign_ptr {
public:
    reassign_ptr() : ptr(reassignment::get_instance().get_new_counter()) {}
    reassign_ptr(T&& value) : ptr(reassignment::get_instance().get_new_counter(std::forward<T>(value))) {}
    reassign_ptr(const reassign_ptr& other) : ptr(reassignment::get_instance().copy(other.ptr)) {}
    ~reassign_ptr() { reassignment::get_instance().remove<T>(ptr); }

    reassign_ptr(reassign_ptr&& other) : ptr(reassignment::get_instance().copy(other.ptr)) {}
    reassign_ptr& operator = (reassign_ptr&& other) {
        reassignment::get_instance().set<T>(ptr, other.ptr);
        return *this;
    };
    reassign_ptr& operator = (const reassign_ptr& other) {
        reassignment::get_instance().set<T>(ptr, other.ptr);
        return *this;
    };

    T& operator*() const { return *get(); }
    T* operator->() const { return get(); }
    T* get() const { return static_cast<T*>(ptr->get()); }
    counter* get_counter() const { return ptr; }

    template <class Base>
    static reassign_ptr<Base> to_base(T&& value) {
        reassign_ptr<T> ptr(std::forward<T>(value));
        return reassign_ptr<Base>(ptr, nullptr);
    }
    template <class O>
    reassign_ptr(const reassign_ptr<O>& other, void*) : ptr(reassignment::get_instance().copy(other.get_counter())) {}
private:
    counter *const ptr;
};
