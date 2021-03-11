#pragma once

#include <cstddef>
#include <fmt/format.h>
#include <stdexcept>
#include <unordered_set>
#include <utility>

template <class T>
class GraphLinker;

template <class T>
class GraphPtr {
public:
    template <class Other>
    inline static GraphPtr create_with_cast(Other&& data) {
        return GraphPtr(new size_t(1), new Other(std::move(data)));
    }
    explicit GraphPtr(T&& value) noexcept : counter(new size_t(1)), data(new T(std::move(value))) {}
    inline GraphPtr(const GraphPtr& other) noexcept : flags(other.flags), counter(other.counter), data(other.data) {
        if (!flags.weak)
            (*counter)++;
        save_pointer();
    }
    GraphPtr& operator=(const GraphPtr& other) = delete;
    GraphPtr& operator=(GraphPtr&& other) = delete;
    inline ~GraphPtr() {
        if (counter != nullptr && !flags.invalid && !flags.weak) {
            (*counter)--;
            if (*counter == 0) {
                delete counter;
                delete data;
            }
        }
        remove_pointer();
    }

    inline T& operator*() const noexcept { return *get(); }
    inline T* operator->() const noexcept { return get(); }
    inline T* get() const noexcept { return data; }

protected:
    friend class GraphLinker<T>;
    inline GraphPtr() noexcept : counter(new size_t(1)), data(nullptr) {}
    inline GraphPtr(size_t* c, T* d) : counter(c), data(d) {}

private:
    inline void save_pointer() {
        if (flags.invalid) {
            auto set = reinterpret_cast<std::unordered_set<GraphPtr<T>*>*>(data);
            set->insert(this);
        }
    }
    inline void remove_pointer() {
        if (flags.invalid) {
            auto set = reinterpret_cast<std::unordered_set<GraphPtr<T>*>*>(data);
            set->erase(this);
        }
    }
    struct {
        bool weak = false;
        bool invalid = false;
    } flags;
    size_t* counter = nullptr;
    T* data = nullptr;
};

template <class T>
class GraphLinker {
public:
    GraphLinker() {}
    GraphLinker(const GraphLinker&) = delete;
    GraphLinker& operator=(const GraphLinker&) = delete;
    GraphLinker(GraphLinker&&) = delete;
    GraphLinker& operator=(GraphLinker&&) = delete;
    inline GraphPtr<T> get() {
        GraphPtr<T> tmp(nullptr, reinterpret_cast<T*>(&pointers));
        tmp.flags.invalid = true;
        tmp.flags.weak = true;
        pointers.insert(&tmp);
        return tmp;
    }
    GraphPtr<T> link(GraphPtr<T> data) {
        for (auto pointer : pointers) {
            (*pointer).data = data.data;
            (*pointer).counter = data.counter;
            (*pointer).flags.invalid = false;
        }
        linked = true;
        return data;
    }
    ~GraphLinker() {
        if (!linked) {
            fmt::print("Graph not linked before destruction. Terminate\n");
            std::terminate();
        }
    }

private:
    bool linked = false;
    std::unordered_set<GraphPtr<T>*> pointers;
};
