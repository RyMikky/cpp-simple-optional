#pragma once
#include <stdexcept>
#include <utility>
#include <type_traits>

// Исключение этого типа должно генерироватся при обращении к пустому optional
class BadOptionalAccess : public std::exception {
public:
    using exception::exception;

    virtual const char* what() const noexcept override {
        return "Bad optional access";
    }
};

template <typename T>
class Optional {
private:
    T* value_ = nullptr;

    // alignas нужен для правильного выравнивания блока памяти
    alignas(T) char data_[sizeof(T)];
    bool is_initialized_ = false;

public:
    Optional() = default;

    Optional(const T& value) 
        : value_(new (&data_[0]) T(value)), is_initialized_(true) {
    }

    Optional(T&& value) 
        : value_(new (&data_[0]) T(std::move(value))), is_initialized_(true) {
    }

    Optional(const Optional<T>& other);

    Optional(Optional<T>&& other) noexcept;

    Optional& operator=(const T& value) {
        if (is_initialized_) {
            *value_ = value;
        }
        else {
            value_ = new (&data_[0]) T(value);
            is_initialized_ = true;
        }
        return *this;
    }
    Optional& operator=(T&& rhs) {
        if (is_initialized_) {
            *value_ = std::move(rhs);
        } 
        else {
            value_ = new (&data_[0]) T(std::move(rhs));
            is_initialized_ = true;
        }
        return *this;
    }
    Optional& operator=(const Optional& rhs) {
        if (rhs.HasValue()) {
            if (!is_initialized_) {
                value_ = new (&data_[0]) T(rhs.Value());
                is_initialized_ = true;
            }
            else {
                *value_ = rhs.Value();
            }
        }
        else {
            CheckedDelete();
        }
        return *this;
    }
    Optional& operator=(Optional&& rhs) noexcept {

        if (rhs.HasValue()) {
            if (!is_initialized_) {
                value_ = new (&data_[0]) T(std::move(rhs.Value()));
                is_initialized_ = true;
            }
            else {
                *value_ = std::move(rhs.Value());
            }
        }
        else {
            CheckedDelete();
        }
        return *this;
    }

    ~Optional() {
        CheckedDelete();
    }

    bool HasValue() const {
        return is_initialized_;
    }

    // Операторы * и -> не должны делать никаких проверок на пустоту Optional.
    // Эти проверки остаются на совести программиста
    T& operator*() {
        return *value_;
    }
    const T& operator*() const {
        return *value_;
    }
    T* operator->() {
        return value_;
    }
    const T* operator->() const {
        return value_;
    }

    // Метод Value() генерирует исключение BadOptionalAccess, если Optional пуст
    T& Value() {
        if (!is_initialized_) {
            throw BadOptionalAccess();
        }
        return *value_;
    }

    const T& Value() const {
        if (!is_initialized_) {
            throw BadOptionalAccess();
        }
        return *value_;
    }

    void Reset() {
        if (is_initialized_) {
            value_ = nullptr;
            is_initialized_ = false;
        }
    }

private:

    bool CheckPointer() {
        if (*this != nullptr) {
            is_initialized_ = true;
        }
        else {
            is_initialized_ = false;
        }

        return is_initialized_;
    }

    bool CheckValueType(const Optional& other ) {
        return std::is_same<decltype(Value()), decltype(other.Value())>::value;
    }

    bool CheckValueType(T& other) {
        return std::is_same<decltype(Value()), decltype(other)>::value;
    }

    void CheckedDelete() {
        if (is_initialized_) {
            value_->~T();
            
        }
        is_initialized_ = false;
    }
};

template <typename T>
bool operator==(const Optional<T>& lhs, const Optional<T>& rhs){
    return lhs.HasValue() == rhs.HasValue()
        && lhs.Value() == rhs.Value();
}

template <typename T>
bool operator!=(const Optional<T>& lhs, const Optional<T>& rhs) {
    return !(lhs == rhs);
}

template <typename T>
Optional<T>::Optional(const Optional<T>& other) {
    is_initialized_ = other.HasValue();
    if (is_initialized_) {
        value_ = new (&data_[0]) T(other.Value());
    }
}

template <typename T>
Optional<T>::Optional(Optional<T>&& other) noexcept {
    is_initialized_ = other.HasValue();
    if (is_initialized_) {
        value_ = new (&data_[0])  T(std::move(other.Value()));
    }
}