#include "TestUtils.h"
#include "algorithms/DijkstraAlgorithms.h"
#include "graph/GraphGenerator.h"
#include "graph/LazyGraph.h"

namespace {
    LazyGraph<int> MakeGrid(int w, int h) {
        return LazyGraph<int>(w * h, [w, h](int id) { return GraphGenerator<int>::GridGenerator(id, w, h); });
    }
}

void RunLazyAlgorithmsTests() {
    HeapGraphAlgorithms<int> heap;
    ArrayGraphAlgorithms<int> arr;

    // сетка 10x10: до противоположного угла 18 шагов
    LazyGraph<int> g = MakeGrid(10, 10);
    ResultInfo<ShortestPathResult<int>> r = heap.ShortestPaths(g, 0);
    CHECK_OK(r);
    CHECK(r.GetValue().dist[99] == 18);
    ResultInfo<DynamicArray<int>> path = r.GetValue().PathTo(99);
    CHECK_OK(path);
    CHECK(path.GetValue().GetSize() == 19);
    CHECK(path.GetValue()[0] == 0 && path.GetValue()[18] == 99);

    // в сетке все рёбра двусторонние: одна компонента
    CHECK(heap.StrongComponents(g).GetValue().count == 1);

    // обе реализации согласованы на ленивом графе
    LazyGraph<int> mid = MakeGrid(30, 30);
    ResultInfo<ShortestPathResult<int>> a = heap.ShortestPaths(mid, 0);
    ResultInfo<ShortestPathResult<int>> b = arr.ShortestPaths(mid, 0);
    CHECK(a.IsSuccess() && b.IsSuccess());
    bool same = true;
    for (int v = 0; v < 900; ++v) {
        if (a.GetValue().dist[v] != b.GetValue().dist[v]) same = false;
    }
    CHECK(same);

    // большая сетка, рёбра нигде не хранятся
    LazyGraph<int> big = MakeGrid(300, 300);
    CHECK(heap.ShortestPaths(big, 0).GetValue().dist[300 * 300 - 1] == 598);

    // граф без рёбер: все кроме старта недостижимы
    LazyGraph<int> isolated(5, [](int) { return DynamicArray<std::pair<int, int>>(); });
    ResultInfo<ShortestPathResult<int>> iso = heap.ShortestPaths(isolated, 0);
    CHECK(iso.GetValue().IsReachable(0) && !iso.GetValue().IsReachable(3));
    CHECK(heap.StrongComponents(isolated).GetValue().count == 5);

    // отрицательный вес из генератора: Дейкстра отказывается, а компоненты работают
    LazyGraph<int> negative(2, [](int id) {
        DynamicArray<std::pair<int, int>> r;
        if (id == 0) r.PushBack(std::pair<int, int>(1, -1));
        return r;
    });
    CHECK_FAIL(heap.ShortestPaths(negative, 0));
    CHECK_FAIL(arr.ShortestPaths(negative, 0));
    CHECK(heap.StrongComponents(negative).GetValue().count == 2);

    CHECK_FAIL(heap.ShortestPaths(g, 100)); // старт вне графа
}