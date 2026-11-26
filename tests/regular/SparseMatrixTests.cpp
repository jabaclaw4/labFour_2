#include "TestUtils.h"
#include "core/SparseMatrix.h"

void RunSparseMatrixTests() {
    SparseMatrix<int> m(5, 5);
    CHECK(m.Rows() == 5 && m.Cols() == 5);
    CHECK(m.NonZeroCount() == 0);
    CHECK(!m.Has(0, 0));
    CHECK_FAIL(m.Get(0, 0)); // пустая ячейка

    CHECK(m.Set(1, 2, 10).GetValue());   // новая ячейка
    CHECK(m.NonZeroCount() == 1);
    CHECK(m.Has(1, 2));
    CHECK(m.Get(1, 2).GetValue() == 10);

    CHECK(!m.Set(1, 2, 20).GetValue());  // замена
    CHECK(m.NonZeroCount() == 1);
    CHECK(m.Get(1, 2).GetValue() == 20);

    CHECK_FAIL(m.Set(5, 0, 1));          // вне матрицы
    CHECK_FAIL(m.Set(0, -1, 1));
    CHECK_FAIL(m.Get(0, 5));
    CHECK(!m.Has(-1, 0));

    // отрицательные значения и ноль тоже хранятся
    m.Set(0, 0, -5);
    m.Set(0, 1, 0);
    CHECK(m.Get(0, 0).GetValue() == -5);
    CHECK(m.Has(0, 1) && m.Get(0, 1).GetValue() == 0);

    // строка остаётся отсортированной по столбцам при любом порядке вставки
    SparseMatrix<int> s(1, 10);
    int cols[6] = {7, 2, 9, 0, 5, 3};
    for (int c : cols) s.Set(0, c, c * 10);
    const DynamicArray<SparseMatrix<int>::Entry>* row = s.RowPtr(0);
    CHECK(row != nullptr && row->GetSize() == 6);
    bool sorted = true;
    for (size_t i = 0; i + 1 < row->GetSize(); ++i) {
        if ((*row)[i].col >= (*row)[i + 1].col) sorted = false;
    }
    CHECK(sorted);
    CHECK(s.RowPtr(1) == nullptr);
    CHECK_OK(s.GetRow(0));
    CHECK_FAIL(s.GetRow(3));

    // большая матрица хранит только заполненные ячейки
    SparseMatrix<int> big(100000, 100000);
    big.Set(0, 99999, 1);
    big.Set(99999, 0, 2);
    CHECK(big.NonZeroCount() == 2);
    CHECK(big.Get(99999, 0).GetValue() == 2);

    SparseMatrix<int> none;
    CHECK(none.Rows() == 0 && none.NonZeroCount() == 0);
}