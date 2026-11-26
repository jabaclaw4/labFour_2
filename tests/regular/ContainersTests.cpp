#include <string>
#include <utility>
#include "TestUtils.h"
#include "core/DynamicArray.h"
#include "core/PriorityQueue.h"
#include "core/ResultInfo.h"

namespace {
    ResultInfo<int> Half(const int& v) {
        if (v % 2 != 0) return ResultInfo<int>::Failure("odd");
        return ResultInfo<int>::Success(v / 2);
    }
}

void RunContainersTests() {
    // DynamicArray
    DynamicArray<int> a;
    CHECK(a.IsEmpty());
    for (int i = 0; i < 100; ++i) a.PushBack(i);
    CHECK(a.GetSize() == 100);
    CHECK(a[50] == 50);
    CHECK(a.At(10).IsSuccess() && a.At(10).GetValue() == 10);
    CHECK_FAIL(a.At(100));
    ResultInfo<int> last = a.PopBack();
    CHECK(last.IsSuccess() && last.GetValue() == 99);
    CHECK(a.GetSize() == 99);
    DynamicArray<int> empty;
    CHECK_FAIL(empty.PopBack());

    DynamicArray<int> copy = a;
    copy[0] = 777;
    CHECK(a[0] == 0);
    DynamicArray<int> moved = std::move(copy);
    CHECK(moved[0] == 777);
    CHECK(copy.IsEmpty());

    DynamicArray<int> filled(5, 7);
    CHECK(filled.GetSize() == 5 && filled[4] == 7);
    filled.Clear();
    CHECK(filled.IsEmpty());

    // PriorityQueue: порядок по ключу
    PriorityQueue<int, int> pq;
    CHECK(pq.IsEmpty());
    CHECK_FAIL(pq.Dequeue());
    CHECK_FAIL(pq.Peek());
    pq.Enqueue(10, 5);
    pq.Enqueue(20, 1);
    pq.Enqueue(30, 3);
    CHECK(pq.GetCount() == 3);
    CHECK(pq.Peek().GetValue() == 20);
    CHECK(pq.Dequeue().GetValue() == 20);
    CHECK(pq.Dequeue().GetValue() == 30);
    CHECK(pq.Dequeue().GetValue() == 10);
    CHECK(pq.IsEmpty());

    // равные ключи выходят в порядке добавления
    PriorityQueue<int, int> eq;
    for (int i = 0; i < 10; ++i) eq.Enqueue(i, 1);
    bool fifo = true;
    for (int i = 0; i < 10; ++i) {
        if (eq.Dequeue().GetValue() != i) fifo = false;
    }
    CHECK(fifo);

    // ключ любого упорядоченного типа, например строка или double
    PriorityQueue<int, std::string> sq;
    sq.Enqueue(1, "banana");
    sq.Enqueue(2, "apple");
    CHECK(sq.Dequeue().GetValue() == 2);
    PriorityQueue<int, double> dq;
    dq.Enqueue(1, 2.5);
    dq.Enqueue(2, -1.5);
    CHECK(dq.Dequeue().GetValue() == 2);

    // много элементов подряд
    PriorityQueue<int, int> big;
    for (int i = 1000; i > 0; --i) big.Enqueue(i, i);
    bool sorted = true;
    for (int i = 1; i <= 1000; ++i) {
        if (big.Dequeue().GetValue() != i) sorted = false;
    }
    CHECK(sorted);

    // ResultInfo
    ResultInfo<int> ok = ResultInfo<int>::Success(8);
    ResultInfo<int> bad = ResultInfo<int>::Failure("boom");
    CHECK(ok.IsSuccess() && !ok.IsFailure() && static_cast<bool>(ok));
    CHECK(bad.IsFailure() && !static_cast<bool>(bad));
    CHECK(std::string(bad.GetError()) == "boom");
    CHECK(ok.GetValueOrDefault(-1) == 8);
    CHECK(bad.GetValueOrDefault(-1) == -1);

    int factor = 3; // лямбда с захватом
    CHECK(ok.Map([factor](const int& v) { return v * factor; }).GetValue() == 24);
    CHECK(bad.Map([](const int& v) { return v + 1; }).IsFailure());
    CHECK(ok.FlatMap(Half).GetValue() == 4);
    CHECK(ResultInfo<int>::Success(7).FlatMap(Half).IsFailure());
    CHECK(bad.FlatMap(Half).IsFailure());
    CHECK(ok.Filter([](const int& v) { return v > 5; }, "small").IsSuccess());
    CHECK(std::string(ok.Filter([](const int& v) { return v > 50; }, "small").GetError()) == "small");
    CHECK(bad.Recover([](const char*) { return 0; }).GetValue() == 0);
    CHECK(ok.Recover([](const char*) { return 0; }).GetValue() == 8);

    int matched = 0;
    ok.Match([&](const int& v) { matched = v; }, [&](const char*) { matched = -1; });
    CHECK(matched == 8);
    bad.Match([&](const int& v) { matched = v; }, [&](const char*) { matched = -1; });
    CHECK(matched == -1);

    ResultInfo<int> taken = ResultInfo<int>::Success(5);
    CHECK(taken.TakeValue() == 5);
}