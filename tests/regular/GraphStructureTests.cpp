#include "TestUtils.h"
#include "graph/Graph.h"

void RunGraphStructureTests() {
    CHECK_FAIL(Graph<int>::Create(-1));
    CHECK_OK(Graph<int>::Create(0));
    CHECK(Graph<int>(-5).GetVerticesCount() == 0);

    Graph<int> g(4);
    CHECK(g.GetVerticesCount() == 4);
    CHECK(g.GetEdgesCount() == 0);

    CHECK(g.AddEdge(0, 1, 5).GetValue());
    CHECK(g.AddEdge(0, 2).GetValue());              // вес по умолчанию 1
    CHECK(g.GetEdgesCount() == 2);
    CHECK(g.GetEdgeWeight(0, 2).GetValue() == 1);
    CHECK(g.HasEdge(0, 1) && !g.HasEdge(1, 0));      // граф ориентированный

    // повторное ребро заменяет вес, число рёбер не растёт
    CHECK(!g.AddEdge(0, 1, 9).GetValue());
    CHECK(g.GetEdgesCount() == 2);
    CHECK(g.GetEdgeWeight(0, 1).GetValue() == 9);

    // неверные вершины это ошибка, а не исключение
    CHECK_FAIL(g.AddEdge(-1, 0, 1));
    CHECK_FAIL(g.AddEdge(0, 4, 1));
    CHECK_FAIL(g.GetEdgeWeight(0, 3));
    CHECK_FAIL(g.GetNeighbors(4));
    CHECK(g.GetEdgesCount() == 2);

    // отрицательные веса и петли разрешены
    CHECK_OK(g.AddEdge(2, 3, -7));
    CHECK_OK(g.AddEdge(3, 3, 0));
    CHECK(g.GetEdgeWeight(2, 3).GetValue() == -7);

    // наружу видно только IEdge: чтение полей
    ResultInfo<EdgeList<int>> nb = g.GetNeighbors(0);
    CHECK_OK(nb);
    CHECK(nb.GetValue().GetSize() == 2);
    const IEdge<int>& e = *nb.GetValue()[0];
    CHECK(e.From() == 0 && e.To() == 1 && e.Weight() == 9);
    CHECK(g.GetNeighbors(1).GetValue().IsEmpty());

    // рёбра в порядке возрастания номера цели
    CHECK(nb.GetValue()[0]->To() < nb.GetValue()[1]->To());

    // другие типы весов
    Graph<double> gd(2);
    gd.AddEdge(0, 1, 0.5);
    CHECK(gd.GetEdgeWeight(0, 1).GetValue() == 0.5);
    Graph<long long> gl(2);
    gl.AddEdge(0, 1, 5000000000LL);
    CHECK(gl.GetEdgeWeight(0, 1).GetValue() == 5000000000LL);

    // копия независима
    Graph<int> copy = g;
    copy.AddEdge(1, 2, 3);
    CHECK(copy.GetEdgesCount() == g.GetEdgesCount() + 1);
}