#pragma once

#include <cstddef>
#include <utility>
#include "ResultInfo.h"

// динамический массив с удвоением вместимости
// operator[] не проверяет границы (как у std::vector), безопасные версии: At и PopBack возвращают ResultInfo
// для хранения типа нужен конструктор по умолчанию
template <typename T>
class DynamicArray {
private:
    T* data_;
    size_t size_;
    size_t capacity_;

    void Reallocate(size_t newCapacity) {
        T* newData = new T[newCapacity];
        for (size_t i = 0; i < size_; ++i) {
            newData[i] = std::move(data_[i]);
        }
        delete[] data_;
        data_ = newData;
        capacity_ = newCapacity;
    }

    void Swap(DynamicArray& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

public:
    DynamicArray() : data_(nullptr), size_(0), capacity_(0) {}

    DynamicArray(size_t count, const T& value)
            : data_(count > 0 ? new T[count] : nullptr), size_(count), capacity_(count) {
        for (size_t i = 0; i < size_; ++i) {
            data_[i] = value;
        }
    }

    DynamicArray(const DynamicArray& other)
            : data_(other.capacity_ > 0 ? new T[other.capacity_] : nullptr),
              size_(other.size_),
              capacity_(other.capacity_) {
        for (size_t i = 0; i < size_; ++i) {
            data_[i] = other.data_[i];
        }
    }

    DynamicArray(DynamicArray&& other) noexcept
            : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    DynamicArray& operator=(const DynamicArray& other) {
        if (this != &other) {
            DynamicArray tmp(other);
            Swap(tmp);
        }
        return *this;
    }

    DynamicArray& operator=(DynamicArray&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    ~DynamicArray() { delete[] data_; }

    size_t GetSize() const { return size_; }
    size_t GetCapacity() const { return capacity_; }
    bool IsEmpty() const { return size_ == 0; }

    // быстрый доступ, индекс должен быть меньше размера
    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }

    // безопасный доступ: копия элемента или ошибка
    ResultInfo<T> At(size_t index) const {
        if (index >= size_) return ResultInfo<T>::Failure("index out of range");
        return ResultInfo<T>::Success(data_[index]);
    }

    void PushBack(const T& value) {
        if (size_ == capacity_) {
            T copy = value;
            Reallocate(capacity_ == 0 ? 4 : capacity_ * 2);
            data_[size_++] = std::move(copy);
        } else {
            data_[size_++] = value;
        }
    }

    // убирает последний элемент и возвращает его, ошибка если массив пуст
    ResultInfo<T> PopBack() {
        if (size_ == 0) return ResultInfo<T>::Failure("array is empty");
        --size_;
        return ResultInfo<T>::Success(std::move(data_[size_]));
    }

    void Clear() { size_ = 0; }
};