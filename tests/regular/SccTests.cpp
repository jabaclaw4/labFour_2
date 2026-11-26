#include "TestUtils.h"
#include "algorithms/DijkstraAlgorithms.h"
#include "graph/GraphGenerator.h"

void RunSccTests() {
    HeapGraphAlgorithms<int> alg;

    // две компоненты {0,1,2} и {3,4,5} (веса отрицательные: на компоненты не влияют)
    Graph<int> g(6);
    g.AddEdge(0, 1, -1);
    g.AddEdge(1, 2, -1);
    g.AddEdge(2, 0, -1);
    g.AddEdge(2, 3, -1);
    g.AddEdge(3, 4, -1);
    g.AddEdge(4, 5, -1);
    g.AddEdge(5, 3, -1);
    ResultInfo<SccResult> r = alg.StrongComponents(g);
    CHECK_OK(r);
    const SccResult& scc = r.GetValue();
    CHECK(scc.count == 2);
    CHECK(scc.component[0] == scc.component[1] && scc.component[1] == scc.component[2]);
    CHECK(scc.component[3] == scc.component[4] && scc.component[4] == scc.component[5]);
    CHECK(scc.component[0] != scc.component[3]);

    // пустой граф
    Graph<int> empty(0);
    CHECK(alg.StrongComponents(empty).GetValue().count == 0);

    // без рёбер: каждая вершина сама по себе
    Graph<int> isolated(4);
    CHECK(alg.StrongComponents(isolated).GetValue().count == 4);

    // цепь (ациклический граф): n компонент
    Graph<int> chain(10);
    GraphGenerator<int>::MakeChain(chain);
    CHECK(alg.StrongComponents(chain).GetValue().count == 10);

    // полный граф: одна компонента
    Graph<int> full(8);
    GraphGenerator<int>::MakeComplete(full);
    CHECK(alg.StrongComponents(full).GetValue().count == 1);

    // длинный цикл: рекурсии нет, переполнения стека не будет
    int n = 50000;
    Graph<int> ring(n);
    GraphGenerator<int>::MakeChain(ring);
    ring.AddEdge(n - 1, 0, 1);
    CHECK(alg.StrongComponents(ring).GetValue().count == 1);

    // длинная цепь
    Graph<int> longChain(n);
    GraphGenerator<int>::MakeChain(longChain);
    CHECK(alg.StrongComponents(longChain).GetValue().count == n);

    // петля сама себе компонента
    Graph<int> loop(2);
    loop.AddEdge(0, 0, 1);
    CHECK(alg.StrongComponents(loop).GetValue().count == 2);

    // обе реализации интерфейса дают одно и то же
    ArrayGraphAlgorithms<int> arr;
    CHECK(arr.StrongComponents(g).GetValue().count == 2);
}