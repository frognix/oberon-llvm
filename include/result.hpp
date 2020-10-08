#include <variant>

template <class OK, class ERR>
class Result {
public:
    using OkType = OK;
    using ErrType = ERR;

    Result(OkType&& ok) : data(std::forward<OkType>(ok)) {
        static_assert(!std::is_same_v<OkType, ErrType>, "Types must be unequal");
    }
    Result(ErrType&& err) : data(std::forward<ErrType>(err)) {
        static_assert(!std::is_same_v<OkType, ErrType>, "Types must be unequal");
    }

    bool is_ok() const {
        return std::holds_alternative<OkType>(data);
    }

    operator bool() const {
        return is_ok();
    }

    OkType get_ok() const {
        return std::get<OkType>(data);
    }
    ErrType get_err() const {
        return std::get<ErrType>(data);
    }
private:
    std::variant<OkType, ErrType> data;
};
