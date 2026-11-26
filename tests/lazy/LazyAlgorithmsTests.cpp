#include "TestUtils.h"
#include "algorithms/ArrayDijkstra.h"
#include "algorithms/HeapDijkstra.h"
#include "algorithms/Tarjan.h"
#include "graph/GraphGenerator.h"
#include "graph/LazyGraph.h"

namespace {
    LazyGraph<int> MakeGrid(int w, int h) {
        return LazyGraph<int>(w * h, [w, h](int id) { return GraphGenerator<int>::GridGenerator(id, w, h); });
    }
}

void RunLazyAlgorithmsTests() {
    HeapDijkstra<int> heap;
    ArrayDijkstra<int> arr;
    Tarjan<int> tarjan;

    // сетка 10x10: до противоположного угла 18 шагов
    LazyGraph<int> g = MakeGrid(10, 10);
    ResultInfo<ShortestPathResult<int>> r = heap.Run(g, StartVertex{0});
    CHECK_OK(r);
    CHECK(r.GetValue().dist[99] == 18);
    ResultInfo<DynamicArray<int>> path = r.GetValue().PathTo(99);
    CHECK_OK(path);
    CHECK(path.GetValue().GetSize() == 19);
    CHECK(path.GetValue()[0] == 0 && path.GetValue()[18] == 99);

    // в сетке все рёбра двусторонние: одна компонента
    CHECK(tarjan.Run(g, NoInput{}).GetValue().count == 1);

    // обе реализации согласованы на ленивом графе
    LazyGraph<int> mid = MakeGrid(30, 30);
    ResultInfo<ShortestPathResult<int>> a = heap.Run(mid, StartVertex{0});
    ResultInfo<ShortestPathResult<int>> b = arr.Run(mid, StartVertex{0});
    CHECK(a.IsSuccess() && b.IsSuccess());
    bool same = true;
    for (int v = 0; v < 900; ++v) {
        if (a.GetValue().dist[v] != b.GetValue().dist[v]) same = false;
    }
    CHECK(same);

    // большая сетка, рёбра нигде не хранятся
    LazyGraph<int> big = MakeGrid(300, 300);
    CHECK(heap.Run(big, StartVertex{0}).GetValue().dist[300 * 300 - 1] == 598);

    // граф без рёбер: все кроме старта недостижимы
    LazyGraph<int> isolated(5, [](int) { return DynamicArray<std::pair<int, int>>(); });
    ResultInfo<ShortestPathResult<int>> iso = heap.Run(isolated, StartVertex{0});
    CHECK(iso.GetValue().IsReachable(0) && !iso.GetValue().IsReachable(3));
    CHECK(tarjan.Run(isolated, NoInput{}).GetValue().count == 5);

    // отрицательный вес из генератора: Дейкстра отказывается, а компоненты работают
    LazyGraph<int> negative(2, [](int id) {
        DynamicArray<std::pair<int, int>> r;
        if (id == 0) r.PushBack(std::pair<int, int>(1, -1));
        return r;
    });
    CHECK_FAIL(heap.Run(negative, StartVertex{0}));
    CHECK_FAIL(arr.Run(negative, StartVertex{0}));
    CHECK(tarjan.Run(negative, NoInput{}).GetValue().count == 2);

    CHECK_FAIL(heap.Run(g, StartVertex{100})); // старт вне графа
}