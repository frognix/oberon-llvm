#include "reassign_core.hpp"

template <class T>
class reassign_ptr {
public:
    reassign_ptr() noexcept : ptr(reassign_core::get_instance().get_new_counter()) {}
    reassign_ptr(T&& value) noexcept : ptr(reassign_core::get_instance().get_new_counter(std::forward<T>(value))) {}
    reassign_ptr(const reassign_ptr& other) noexcept : ptr(reassign_core::get_instance().copy(other.ptr)) {}
    ~reassign_ptr() { reassign_core::get_instance().remove<T>(ptr); }

    reassign_ptr(reassign_ptr&& other) noexcept : ptr(reassign_core::get_instance().copy(other.ptr)) {}
    reassign_ptr& operator = (reassign_ptr&& other) noexcept {
        reassign_core::get_instance().move<T>(ptr, other.ptr);
        return *this;
    };
    reassign_ptr& operator = (const reassign_ptr& other) noexcept {
        reassign_core::get_instance().set(ptr, other.ptr);
        return *this;
    };

    T& operator*() const noexcept { return *get(); }
    T* operator->() const noexcept { return get(); }
    T* get() const noexcept { return static_cast<T*>(ptr->get()); }
    counter* get_counter() const noexcept { return ptr; }

    template <class Base>
    static reassign_ptr<Base> to_base(T&& value) noexcept {
        reassign_ptr<T> ptr(std::forward<T>(value));
        return reassign_ptr<Base>(ptr, nullptr);
    }
    template <class O>
    reassign_ptr(const reassign_ptr<O>& other, void*) noexcept : ptr(reassign_core::get_instance().copy(other.get_counter())) {}
private:
    counter *const ptr;
};
