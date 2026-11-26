#include "TestUtils.h"
#include "graph/GraphGenerator.h"
#include "graph/LazyGraph.h"

void RunLazyGraphStructureTests() {
    NeighborGenerator<int> grid3 = [](int id) { return GraphGenerator<int>::GridGenerator(id, 3, 3); };

    CHECK_FAIL(LazyGraph<int>::Create(-1, grid3));
    CHECK_FAIL(LazyGraph<int>::Create(5, NeighborGenerator<int>())); // пустой генератор

    ResultInfo<LazyGraph<int>> created = LazyGraph<int>::Create(9, grid3);
    CHECK_OK(created);
    const LazyGraph<int>& g = created.GetValue();
    CHECK(g.GetVerticesCount() == 9);

    // углы, края и центр сетки 3x3
    CHECK(g.GetNeighbors(0).GetValue().GetSize() == 2);
    CHECK(g.GetNeighbors(1).GetValue().GetSize() == 3);
    CHECK(g.GetNeighbors(4).GetValue().GetSize() == 4);
    CHECK(g.GetNeighbors(8).GetValue().GetSize() == 2);

    CHECK_FAIL(g.GetNeighbors(-1));
    CHECK_FAIL(g.GetNeighbors(9));

    // рёбра читаются через IEdge
    ResultInfo<EdgeList<int>> nb = g.GetNeighbors(0);
    const EdgeList<int>& es = nb.GetValue();
    bool allFrom0 = true;
    for (size_t i = 0; i < es.GetSize(); ++i) {
        if (es[i]->From() != 0 || es[i]->Weight() != 1) allFrom0 = false;
    }
    CHECK(allFrom0);

    // цели вне графа отбрасываются
    LazyGraph<int> dirty(3, [](int) {
        DynamicArray<std::pair<int, int>> r;
        r.PushBack(std::pair<int, int>(1, 1));
        r.PushBack(std::pair<int, int>(99, 1));
        r.PushBack(std::pair<int, int>(-1, 1));
        return r;
    });
    CHECK(dirty.GetNeighbors(0).GetValue().GetSize() == 1);

    // граф по умолчанию пуст
    LazyGraph<int> none;
    CHECK(none.GetVerticesCount() == 0);
    CHECK_FAIL(none.GetNeighbors(0));

    // генератор не вызывается до запроса: счётчик вызовов
    int calls = 0;
    LazyGraph<int> counted(100, [&calls](int) {
        ++calls;
        return DynamicArray<std::pair<int, int>>();
    });
    CHECK(calls == 0);
    counted.GetNeighbors(5);
    CHECK(calls == 1);
}