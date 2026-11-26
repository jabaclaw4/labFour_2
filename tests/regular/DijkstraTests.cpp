#include "TestUtils.h"
#include "algorithms/DijkstraAlgorithms.h"
#include "graph/GraphGenerator.h"

namespace {

    Graph<int> Sample() {
        Graph<int> g(5);
        g.AddEdge(0, 1, 4);
        g.AddEdge(0, 2, 1);
        g.AddEdge(2, 1, 2);
        g.AddEdge(1, 3, 1);
        g.AddEdge(2, 3, 5);
        return g; // вершина 4 недостижима
    }

    void CheckSample(const IGraphAlgorithms<int>& alg) {
        Graph<int> g = Sample();
        ResultInfo<ShortestPathResult<int>> r = alg.ShortestPaths(g, 0);
        CHECK_OK(r);
        const ShortestPathResult<int>& sp = r.GetValue();
        CHECK(sp.dist[0] == 0 && sp.dist[1] == 3 && sp.dist[2] == 1 && sp.dist[3] == 4);
        CHECK(sp.IsReachable(3) && !sp.IsReachable(4) && !sp.IsReachable(99));

        ResultInfo<DynamicArray<int>> path = sp.PathTo(3);
        CHECK_OK(path);
        CHECK(path.GetValue().GetSize() == 4);
        CHECK(path.GetValue()[0] == 0 && path.GetValue()[1] == 2 &&
              path.GetValue()[2] == 1 && path.GetValue()[3] == 3);
        CHECK(sp.PathTo(0).GetValue().GetSize() == 1);
        CHECK_FAIL(sp.PathTo(4));
        CHECK_FAIL(sp.PathTo(-1));

        CHECK_FAIL(alg.ShortestPaths(g, -1));
        CHECK_FAIL(alg.ShortestPaths(g, 5));
    }

    void CheckNegative(const IGraphAlgorithms<int>& alg) {
        Graph<int> g(3);
        CHECK_OK(g.AddEdge(0, 1, 2));   // граф отрицательный вес принимает
        CHECK_OK(g.AddEdge(1, 2, -4));
        ResultInfo<ShortestPathResult<int>> r = alg.ShortestPaths(g, 0);
        CHECK_FAIL(r);                  // а Дейкстра нет
    }

} // namespace

void RunDijkstraTests() {
    HeapGraphAlgorithms<int> heap;
    ArrayGraphAlgorithms<int> arr;

    CheckSample(heap);
    CheckSample(arr);
    CheckNegative(heap);
    CheckNegative(arr);

    // работа через указатель на интерфейс
    const IGraphAlgorithms<int>* iface = &heap;
    CHECK(iface->Name() != nullptr);

    // обе реализации дают одинаковые расстояния на случайных графах
    for (unsigned seed = 1; seed <= 5; ++seed) {
        Graph<int> g(200);
        CHECK_OK(GraphGenerator<int>::MakeRandom(g, 800, seed, 1, 50));
        ResultInfo<ShortestPathResult<int>> a = heap.ShortestPaths(g, 0);
        ResultInfo<ShortestPathResult<int>> b = arr.ShortestPaths(g, 0);
        CHECK(a.IsSuccess() && b.IsSuccess());
        bool same = true;
        for (int v = 0; v < 200; ++v) {
            if (a.GetValue().dist[v] != b.GetValue().dist[v]) same = false;
        }
        CHECK(same);
    }

    // цепь
    Graph<int> chain(100);
    GraphGenerator<int>::MakeChain(chain, 2);
    CHECK(heap.ShortestPaths(chain, 0).GetValue().dist[99] == 198);

    // вес ноль
    Graph<int> zero(3);
    zero.AddEdge(0, 1, 0);
    zero.AddEdge(1, 2, 0);
    CHECK(heap.ShortestPaths(zero, 0).GetValue().dist[2] == 0);

    // граф из одной вершины
    Graph<int> single(1);
    CHECK(heap.ShortestPaths(single, 0).GetValue().dist[0] == 0);

    // double и long long
    Graph<double> gd(3);
    gd.AddEdge(0, 1, 0.5);
    gd.AddEdge(1, 2, 0.25);
    HeapGraphAlgorithms<double> heapD;
    CHECK(heapD.ShortestPaths(gd, 0).GetValue().dist[2] == 0.75);
    Graph<long long> gl(3);
    gl.AddEdge(0, 1, 3000000000LL);
    gl.AddEdge(1, 2, 3000000000LL);
    ArrayGraphAlgorithms<long long> arrL;
    CHECK(arrL.ShortestPaths(gl, 0).GetValue().dist[2] == 6000000000LL);
}