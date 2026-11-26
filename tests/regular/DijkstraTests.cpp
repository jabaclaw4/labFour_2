#include "TestUtils.h"
#include "algorithms/ArrayDijkstra.h"
#include "algorithms/HeapDijkstra.h"
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

    void CheckSample(const IShortestPath<int>& alg) {
        Graph<int> g = Sample();
        ResultInfo<ShortestPathResult<int>> r = alg.Run(g, StartVertex{0});
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

        CHECK_FAIL(alg.Run(g, StartVertex{-1}));
        CHECK_FAIL(alg.Run(g, StartVertex{5}));
    }

    void CheckNegative(const IShortestPath<int>& alg) {
        Graph<int> g(3);
        CHECK_OK(g.AddEdge(0, 1, 2));   // граф отрицательный вес принимает
        CHECK_OK(g.AddEdge(1, 2, -4));
        CHECK_FAIL(alg.Run(g, StartVertex{0})); // а Дейкстра нет
    }

// новый алгоритм без правок существующих файлов: число вершин, достижимых из старта
    class ReachableCount final : public IGraphAlgorithm<int, StartVertex, int> {
    public:
        const char* Name() const override { return "Reachable count"; }
        ResultInfo<int> Run(const IGraph<int>& graph, const StartVertex& in) const override {
            ResultInfo<bool> check = detail::CheckStart(graph, in.vertex);
            if (check.IsFailure()) return ResultInfo<int>::Failure(check.GetError());
            DynamicArray<bool> seen(graph.GetVerticesCount(), false);
            DynamicArray<int> stack;
            stack.PushBack(in.vertex);
            seen[in.vertex] = true;
            int count = 0;
            while (!stack.IsEmpty()) {
                int u = stack.PopBack().GetValue();
                ++count;
                ResultInfo<EdgeList<int>> nb = graph.GetNeighbors(u);
                if (nb.IsFailure()) return ResultInfo<int>::Failure(nb.GetError());
                for (size_t i = 0; i < nb.GetValue().GetSize(); ++i) {
                    int to = nb.GetValue()[i]->To();
                    if (!seen[to]) {
                        seen[to] = true;
                        stack.PushBack(to);
                    }
                }
            }
            return ResultInfo<int>::Success(count);
        }
    };

} // namespace

void RunDijkstraTests() {
    HeapDijkstra<int> heap;
    ArrayDijkstra<int> arr;

    CheckSample(heap);
    CheckSample(arr);
    CheckNegative(heap);
    CheckNegative(arr);

    // работа через указатель на интерфейс
    const IShortestPath<int>* iface = &heap;
    CHECK(iface->Name() != nullptr);

    // обе реализации дают одинаковые расстояния на случайных графах
    for (unsigned seed = 1; seed <= 5; ++seed) {
        Graph<int> g(200);
        CHECK_OK(GraphGenerator<int>::MakeRandom(g, 800, seed, 1, 50));
        ResultInfo<ShortestPathResult<int>> a = heap.Run(g, StartVertex{0});
        ResultInfo<ShortestPathResult<int>> b = arr.Run(g, StartVertex{0});
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
    CHECK(heap.Run(chain, StartVertex{0}).GetValue().dist[99] == 198);

    // вес ноль
    Graph<int> zero(3);
    zero.AddEdge(0, 1, 0);
    zero.AddEdge(1, 2, 0);
    CHECK(heap.Run(zero, StartVertex{0}).GetValue().dist[2] == 0);

    // граф из одной вершины
    Graph<int> single(1);
    CHECK(heap.Run(single, StartVertex{0}).GetValue().dist[0] == 0);

    // double и long long
    Graph<double> gd(3);
    gd.AddEdge(0, 1, 0.5);
    gd.AddEdge(1, 2, 0.25);
    HeapDijkstra<double> heapD;
    CHECK(heapD.Run(gd, StartVertex{0}).GetValue().dist[2] == 0.75);
    Graph<long long> gl(3);
    gl.AddEdge(0, 1, 3000000000LL);
    gl.AddEdge(1, 2, 3000000000LL);
    ArrayDijkstra<long long> arrL;
    CHECK(arrL.Run(gl, StartVertex{0}).GetValue().dist[2] == 6000000000LL);

    // расширяемость: свой алгоритм с другим типом результата работает с тем же графом
    ReachableCount reach;
    Graph<int> s = Sample();
    CHECK(reach.Run(s, StartVertex{0}).GetValue() == 4); // вершина 4 недостижима
    CHECK_FAIL(reach.Run(s, StartVertex{9}));
}