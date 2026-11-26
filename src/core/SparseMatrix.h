#pragma once

#include "DynamicArray.h"
#include "ResultInfo.h"

// разреженная матрица: хранятся только заполненные ячейки
// каждая строка это массив пар (столбец, значение), отсортированный по столбцу
// поиск ячейки бинарным поиском O(log k), вставка O(k), где k число ячеек в строке
// отсутствие значения в ячейке: Has возвращает false, Get возвращает ошибку
template <typename T>
class SparseMatrix {
public:
    struct Entry {
        int col;
        T value;
        Entry() : col(-1), value() {}
        Entry(int c, const T& v) : col(c), value(v) {}
    };

private:
    int rows_;
    int cols_;
    size_t count_;
    DynamicArray<DynamicArray<Entry>> data_;

    // первая позиция в строке, где col >= искомого
    static size_t LowerBound(const DynamicArray<Entry>& row, int col) {
        size_t lo = 0, hi = row.GetSize();
        while (lo < hi) {
            size_t mid = lo + (hi - lo) / 2;
            if (row[mid].col < col) lo = mid + 1;
            else hi = mid;
        }
        return lo;
    }

    bool InRange(int r, int c) const {
        return r >= 0 && r < rows_ && c >= 0 && c < cols_;
    }

public:
    SparseMatrix() : rows_(0), cols_(0), count_(0), data_() {}

    SparseMatrix(int rows, int cols)
            : rows_(rows < 0 ? 0 : rows), cols_(cols < 0 ? 0 : cols), count_(0), data_() {
        for (int i = 0; i < rows_; ++i) {
            data_.PushBack(DynamicArray<Entry>());
        }
    }

    int Rows() const { return rows_; }
    int Cols() const { return cols_; }
    size_t NonZeroCount() const { return count_; }

    // записать значение в ячейку
    // Success(true): создана новая ячейка, Success(false): старое значение заменено
    ResultInfo<bool> Set(int r, int c, const T& value) {
        if (!InRange(r, c)) return ResultInfo<bool>::Failure("cell out of range");
        DynamicArray<Entry>& row = data_[r];
        size_t pos = LowerBound(row, c);
        if (pos < row.GetSize() && row[pos].col == c) {
            row[pos].value = value;
            return ResultInfo<bool>::Success(false);
        }
        row.PushBack(Entry(c, value));
        for (size_t i = row.GetSize() - 1; i > pos; --i) {
            row[i] = row[i - 1];
        }
        row[pos] = Entry(c, value);
        ++count_;
        return ResultInfo<bool>::Success(true);
    }

    bool Has(int r, int c) const {
        if (!InRange(r, c)) return false;
        const DynamicArray<Entry>& row = data_[r];
        size_t pos = LowerBound(row, c);
        return pos < row.GetSize() && row[pos].col == c;
    }

    ResultInfo<T> Get(int r, int c) const {
        if (!InRange(r, c)) return ResultInfo<T>::Failure("cell out of range");
        const DynamicArray<Entry>& row = data_[r];
        size_t pos = LowerBound(row, c);
        if (pos < row.GetSize() && row[pos].col == c) return ResultInfo<T>::Success(row[pos].value);
        return ResultInfo<T>::Failure("cell is empty");
    }

    // быстрый доступ к строке без копирования, nullptr если номер строки неверный
    const DynamicArray<Entry>* RowPtr(int r) const {
        if (r < 0 || r >= rows_) return nullptr;
        return &data_[r];
    }

    // копия строки в виде результата
    ResultInfo<DynamicArray<Entry>> GetRow(int r) const {
        const DynamicArray<Entry>* row = RowPtr(r);
        if (!row) return ResultInfo<DynamicArray<Entry>>::Failure("row out of range");
        return ResultInfo<DynamicArray<Entry>>::Success(*row);
    }
};