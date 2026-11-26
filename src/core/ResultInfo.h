#pragma once

#include <type_traits>
#include <utility>

// монада для обработки ошибок без исключений
// Map / FlatMap / Filter / Recover принимают любые вызываемые объекты, в том числе лямбды с захватом
template <class T>
class ResultInfo {
private:
    bool isSuccess;
    T value;
    const char* errorMessage;

    ResultInfo(bool success, T val, const char* error)
            : isSuccess(success), value(std::move(val)), errorMessage(error) {}

public:
    static ResultInfo Success(T val) {
        return ResultInfo(true, std::move(val), nullptr);
    }

    static ResultInfo Failure(const char* error) {
        return ResultInfo(false, T(), error);
    }

    bool IsSuccess() const { return isSuccess; }
    bool IsFailure() const { return !isSuccess; }

    // значение; при ошибке лежит значение по умолчанию, поэтому сначала проверяйте IsSuccess
    // исключений здесь нет
    const T& GetValue() const { return value; }

    // забрать значение без копирования
    T TakeValue() { return std::move(value); }

    T GetValueOrDefault(T defaultValue) const {
        if (isSuccess) return value;
        return defaultValue;
    }

    const char* GetError() const {
        return errorMessage ? errorMessage : "";
    }

    // T -> U
    template <class F>
    auto Map(F func) const -> ResultInfo<std::invoke_result_t<F, const T&>> {
        using U = std::invoke_result_t<F, const T&>;
        if (isSuccess) return ResultInfo<U>::Success(func(value));
        return ResultInfo<U>::Failure(errorMessage);
    }

    // T -> ResultInfo<U>
    template <class F>
    auto FlatMap(F func) const -> std::invoke_result_t<F, const T&> {
        using R = std::invoke_result_t<F, const T&>;
        if (isSuccess) return func(value);
        return R::Failure(errorMessage);
    }

    template <class Pred>
    ResultInfo<T> Filter(Pred predicate, const char* errorMsg) const {
        if (!isSuccess) return *this;
        if (predicate(value)) return *this;
        return ResultInfo<T>::Failure(errorMsg);
    }

    template <class F>
    ResultInfo<T> Recover(F recoveryFunc) const {
        if (isSuccess) return *this;
        return ResultInfo<T>::Success(recoveryFunc(GetError()));
    }

    template <class OnSuccess, class OnFailure>
    void Match(OnSuccess onSuccess, OnFailure onFailure) const {
        if (isSuccess) onSuccess(value);
        else onFailure(GetError());
    }

    explicit operator bool() const { return isSuccess; }
};