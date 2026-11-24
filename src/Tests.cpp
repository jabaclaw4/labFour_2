#include <iostream>
#include <fstream>
#include <cstdio>
#include <stdexcept>
#include <limits>
#include <string>
#include "TestUtils.h"
#include "Common.h"
#include "Graph.h"
#include "Algorithms.h"
#include "PriorityQueue.h"
#include "GraphGenerator.h"
#include "Benchmark.h"

int g_testFailures = 0;
int g_testChecks = 0;

namespace {

    // динамический массив
    void TestDynamicArray() {
        DynamicArray<int> a;
        CHECK(a.IsEmpty());
        for (int i = 0; i < 1000; ++i) a.PushBack(i);
        CHECK(a.GetSize() == 1000);
        CHECK(a[0] == 0 && a[999] == 999);

        DynamicArray<int> copy = a;
        copy[0] = 42;
        CHECK(a[0] == 0); // копия независима

        a.PopBack();
        CHECK(a.GetSize() == 999);
        a.Clear();
        CHECK(a.IsEmpty());

        CHECK_THROWS(a.PopBack(), std::out_of_range);
        CHECK_THROWS(a.Get(0), std::out_of_range);

        DynamicArray<int> filled(5, 7);
        CHECK(filled.GetSize() == 5 && filled[4] == 7);
    }

    // базовые операции графа
    void TestGraphBasics() {
        Graph<int> g(5);
        g.AddEdge(0, 1, 2);
        g.AddEdge(0, 2, 1);
        g.AddEdge(0, 0, 5); // петля
        g.AddEdge(0, 1, 3); // кратное ребро
        g.AddEdge(3, 4, 0); // нулевой вес разрешён

        CHECK(g.GetVerticesCount() == 5);
        CHECK(g.GetEdgesCount() == 5); // петля и кратное считаются

        CHECK(g.GetNeighbors(0).GetSize() == 4);
        CHECK(g.GetNeighbors(4).GetSize() == 0);

        CHECK_THROWS(g.AddEdge(0, 10, 1), std::out_of_range);
        CHECK_THROWS(g.AddEdge(-1, 1, 1), std::out_of_range);
        CHECK_THROWS(g.AddEdge(0, 1, -5), std::invalid_argument);
        CHECK_THROWS(g.GetNeighbors(7), std::out_of_range);
        CHECK_THROWS(Graph<int> bad(-1), std::invalid_argument);
        CHECK(g.GetEdgesCount() == 5); // неудачные добавления не считаются
    }

    // граничные случаи
    void TestEdgeCases() {
        Graph<int> empty(0);
        CHECK(empty.GetVerticesCount() == 0);
        SccResult r0 = GraphAlgorithms<int>::StrongComponents(empty);
        CHECK(r0.count == 0);
        CHECK(r0.component.GetSize() == 0);
        CHECK_THROWS(GraphAlgorithms<int>::Dijkstra(empty, 0), std::out_of_range);

        Graph<int> single(1);
        single.AddEdge(0, 0, 10); // петля на единственной вершине
        CHECK(single.GetEdgesCount() == 1);

        DynamicArray<int> dist = GraphAlgorithms<int>::Dijkstra(single, 0);
        CHECK(dist.GetSize() == 1);
        CHECK(dist[0] == 0);

        CHECK_THROWS(GraphAlgorithms<int>::Dijkstra(single, 5), std::out_of_range);
        CHECK_THROWS(GraphAlgorithms<int>::Dijkstra(single, -1), std::out_of_range);
        CHECK_THROWS(GraphAlgorithms<int>::DijkstraArray(single, 5), std::out_of_range);
    }

    // дейкстра: расстояния, пути, особые случаи
    void TestDijkstra() {
        Graph<int> g(6);
        g.AddEdge(0, 1, 1);
        g.AddEdge(1, 2, 2);
        g.AddEdge(0, 2, 5);
        g.AddEdge(3, 4, 1); // вторая компонента
        // 5 изолирована

        DynamicArray<int> prev;
        DynamicArray<int> dist = GraphAlgorithms<int>::Dijkstra(g, 0, &prev);
        CHECK(dist[0] == 0);
        CHECK(dist[1] == 1);
        CHECK(dist[2] == 3); // обошли через 1
        CHECK(dist[3] == INF); // недостижима
        CHECK(dist[5] == INF);

        DynamicArray<int> path = GraphAlgorithms<int>::RestorePath(prev, 0, 2);
        CHECK(path.GetSize() == 3);
        CHECK(path[0] == 0 && path[1] == 1 && path[2] == 2);

        // путь до самой себя и до недостижимой
        DynamicArray<int> self = GraphAlgorithms<int>::RestorePath(prev, 0, 0);
        CHECK(self.GetSize() == 1 && self[0] == 0);
        CHECK(GraphAlgorithms<int>::RestorePath(prev, 0, 3).IsEmpty());
        CHECK(GraphAlgorithms<int>::RestorePath(prev, 0, 99).IsEmpty());
        CHECK(GraphAlgorithms<int>::RestorePath(prev, -1, 2).IsEmpty());

        // версия на массиве даёт тот же путь
        DynamicArray<int> prevArr;
        GraphAlgorithms<int>::DijkstraArray(g, 0, &prevArr);
        DynamicArray<int> pathArr = GraphAlgorithms<int>::RestorePath(prevArr, 0, 2);
        CHECK(pathArr.GetSize() == 3);

        // нулевые веса, петли и кратные рёбра
        Graph<int> z(3);
        z.AddEdge(0, 1, 0);
        z.AddEdge(1, 2, 0);
        z.AddEdge(1, 1, 3);
        z.AddEdge(0, 2, 7);
        z.AddEdge(0, 2, 4);
        DynamicArray<int> dz = GraphAlgorithms<int>::Dijkstra(z, 0);
        CHECK(dz[1] == 0);
        CHECK(dz[2] == 0);

        // тип double
        Graph<double> gd(3);
        gd.AddEdge(0, 1, 0.5);
        gd.AddEdge(1, 2, 0.25);
        DynamicArray<double> dd = GraphAlgorithms<double>::Dijkstra(gd, 0);
        CHECK(dd[2] == 0.75);

        // отрицательный вес в ленивом графе
        LazyGraph<int> neg(2, [](int id) {
            DynamicArray<Edge<int>> r;
            if (id == 0) r.PushBack(Edge<int>(1, -1));
            return r;
        });
        CHECK_THROWS(GraphAlgorithms<int>::Dijkstra(neg, 0), std::invalid_argument);
        CHECK_THROWS(GraphAlgorithms<int>::DijkstraArray(neg, 0), std::invalid_argument);

        // переполнение при сложении не происходит
        Graph<int> big(3);
        big.AddEdge(0, 1, INF - 1);
        big.AddEdge(1, 2, 5);
        DynamicArray<int> db = GraphAlgorithms<int>::Dijkstra(big, 0);
        CHECK(db[1] == INF - 1);
        CHECK(db[2] == INF);
    }

    // компоненты сильной связности
    void TestStrongComponents() {
        // цикл: одна компонента
        Graph<int> cycle(4);
        cycle.AddEdge(0, 1);
        cycle.AddEdge(1, 2);
        cycle.AddEdge(2, 3);
        cycle.AddEdge(3, 0);
        SccResult r1 = GraphAlgorithms<int>::StrongComponents(cycle);
        CHECK(r1.count == 1);
        CHECK(r1.component[0] == r1.component[1] && r1.component[1] == r1.component[2]
              && r1.component[2] == r1.component[3]);

        // цепь: каждая вершина отдельно
        Graph<int> chain(4);
        chain.AddEdge(0, 1);
        chain.AddEdge(1, 2);
        chain.AddEdge(2, 3);
        SccResult r2 = GraphAlgorithms<int>::StrongComponents(chain);
        CHECK(r2.count == 4);
        CHECK(r2.component[0] != r2.component[1] && r2.component[1] != r2.component[2]
              && r2.component[2] != r2.component[3]);

        // изолированные вершины и петля
        Graph<int> iso(3);
        iso.AddEdge(1, 1, 5);
        SccResult r3 = GraphAlgorithms<int>::StrongComponents(iso);
        CHECK(r3.count == 3);
        CHECK(r3.component[0] != r3.component[1] && r3.component[1] != r3.component[2]);

        // ребро в одну сторону: не одна компонента
        Graph<int> one(2);
        one.AddEdge(0, 1);
        SccResult r4 = GraphAlgorithms<int>::StrongComponents(one);
        CHECK(r4.count == 2);
        CHECK(r4.component[0] != r4.component[1]);

        // два цикла, соединённые ребром в одну сторону
        Graph<int> two(4);
        two.AddEdge(0, 1);
        two.AddEdge(1, 0);
        two.AddEdge(2, 3);
        two.AddEdge(3, 2);
        two.AddEdge(1, 2);
        SccResult r5 = GraphAlgorithms<int>::StrongComponents(two);
        CHECK(r5.count == 2);
        CHECK(r5.component[0] == r5.component[1]);
        CHECK(r5.component[2] == r5.component[3]);
        CHECK(r5.component[0] != r5.component[2]);
    }

    // сравнение двух версий дейкстры
    void TestDijkstraEquality() {
        for (unsigned seed = 1; seed <= 20; ++seed) {
            Graph<int> g(30);
            GraphGenerator<int>::MakeRandom(g, 30, 90, seed, 10);

            DynamicArray<int> distHeap = GraphAlgorithms<int>::Dijkstra(g, 0);
            DynamicArray<int> distArray = GraphAlgorithms<int>::DijkstraArray(g, 0);

            bool same = true;
            for (int i = 0; i < 30; ++i) {
                if (distHeap[i] != distArray[i]) same = false;
            }
            CHECK(same);
        }
    }

    // длинная цепь не должна переполнять стек вызовов
    void TestLongChain() {
        int n = 100000;
        Graph<int> g(n);
        for (int i = 0; i < n - 1; ++i) {
            g.AddEdge(i, i + 1, 1);
        }

        SccResult r = GraphAlgorithms<int>::StrongComponents(g);
        CHECK(r.count == n); // в ориентированной цепи каждая вершина отдельно
        CHECK(r.component[0] != r.component[n - 1]);
    }

    // ленивый граф
    void TestLazyGraph() {
        int n = 1000000;
        LazyGraph<int> lg(n, [n](int id) {
            DynamicArray<Edge<int>> res;
            if (id + 1 < n) res.PushBack(Edge<int>(id + 1, 1));
            if (id + 2 < n) res.PushBack(Edge<int>(id + 2, 1));
            return res;
        });

        CHECK(lg.GetVerticesCount() == n);
        CHECK(lg.GetNeighbors(0).GetSize() == 2);
        CHECK(lg.GetNeighbors(n - 1).GetSize() == 0);
        CHECK_THROWS(lg.GetNeighbors(n), std::out_of_range);

        DynamicArray<int> dist = GraphAlgorithms<int>::Dijkstra(lg, 0);
        CHECK(dist[0] == 0);
        CHECK(dist[1] == 1);
        CHECK(dist[2] == 1);
        CHECK(dist[n - 1] == (n - 1 + 1) / 2);
    }

    // сетка: соседи по формуле и граничные клетки
    void TestGrid() {
        CHECK(GraphGenerator<int>::GridGenerator(0, 3).GetSize() == 2); // угол
        CHECK(GraphGenerator<int>::GridGenerator(2, 3).GetSize() == 2); // правая граница
        CHECK(GraphGenerator<int>::GridGenerator(1, 3).GetSize() == 3); // край
        CHECK(GraphGenerator<int>::GridGenerator(4, 3).GetSize() == 4); // центр
        CHECK(GraphGenerator<int>::GridGenerator(8, 3).GetSize() == 2); // угол

        // прямоугольник 4x2
        CHECK(GraphGenerator<int>::GridGenerator(3, 4, 2).GetSize() == 2);
        CHECK(GraphGenerator<int>::GridGenerator(5, 4, 2).GetSize() == 3);

        // ленивый граф на сетке 3x3: до последней клетки 4 шага
        LazyGraph<int> lg(9, [](int id) { return GraphGenerator<int>::GridGenerator(id, 3, 3); });
        DynamicArray<int> dist = GraphAlgorithms<int>::Dijkstra(lg, 0);
        CHECK(dist[8] == 4);
        CHECK(dist[4] == 2);
    }

    // очередь с приоритетом
    void TestPriorityQueue() {
        PriorityQueue<std::string, int> pq;
        CHECK(pq.IsEmpty());
        pq.Enqueue("low", 10);
        pq.Enqueue("high", 1);
        pq.Enqueue("mid", 5);
        pq.Enqueue("high2", 1); // равный приоритет

        CHECK(pq.GetCount() == 4);
        CHECK(pq.Peek() == "high");
        CHECK(pq.Dequeue() == "high");
        CHECK(pq.Dequeue() == "high2");
        CHECK(pq.Dequeue() == "mid");
        CHECK(pq.Dequeue() == "low");
        CHECK(pq.GetCount() == 0);

        CHECK_THROWS(pq.Dequeue(), std::out_of_range);
        CHECK_THROWS(pq.Peek(), std::out_of_range);
    }

    // генератор графов
    void TestGenerator() {
        Graph<int> chain(10);
        GraphGenerator<int>::MakeChain(chain, 10);
        CHECK(chain.GetEdgesCount() == 9);

        Graph<int> full(6);
        GraphGenerator<int>::MakeComplete(full, 6);
        CHECK(full.GetEdgesCount() == 30);

        Graph<int> a(20), b(20);
        GraphGenerator<int>::MakeRandom(a, 20, 60, 7, 9);
        GraphGenerator<int>::MakeRandom(b, 20, 60, 7, 9);
        CHECK(a.GetEdgesCount() == 60);
        bool same = true;
        for (int i = 0; i < 20; ++i) {
            DynamicArray<Edge<int>> na = a.GetNeighbors(i);
            DynamicArray<Edge<int>> nb = b.GetNeighbors(i);
            if (na.GetSize() != nb.GetSize()) {
                same = false;
                continue;
            }
            for (size_t k = 0; k < na.GetSize(); ++k) {
                if (na[k].to != nb[k].to || na[k].weight != nb[k].weight) same = false;
            }
        }
        CHECK(same); // одинаковое зерно даёт одинаковый граф

        // три порядка добавления дают одинаковое число рёбер
        EdgeOrder orders[3] = {EdgeOrder::Ascending, EdgeOrder::Descending, EdgeOrder::Random};
        for (int o = 0; o < 3; ++o) {
            Graph<int> g(15);
            GraphGenerator<int>::MakeOrdered(g, 15, 4, 3, 10, orders[o]);
            CHECK(g.GetEdgesCount() == 60);
        }

        // неверные параметры
        Graph<int> g(5);
        CHECK_THROWS(GraphGenerator<int>::MakeRandom(g, 0, 5, 1, 10), std::invalid_argument);
        CHECK_THROWS(GraphGenerator<int>::MakeRandom(g, 5, 5, 1, 0), std::invalid_argument);
    }

    // выгрузка в csv
    void TestCsv() {
        DynamicArray<BenchRow> rows;
        BenchRow r;
        r.order = "random";
        r.n = 10;
        r.edges = 50;
        r.heapMs = 1.5;
        r.arrayMs = 2.5;
        rows.PushBack(r);

        const std::string name = "test_bench.csv";
        CHECK(Benchmark<int>::ExportToCSV(name, rows));

        std::ifstream in(name);
        std::string header, line;
        std::getline(in, header);
        std::getline(in, line);
        in.close();
        std::remove(name.c_str());

        CHECK(header == "order,n,edges,heap_ms,array_ms");
        CHECK(line == "random,10,50,1.5,2.5");

        // файл не открылся
        CHECK(!Benchmark<int>::ExportToCSV("no_such_dir/x.csv", rows));
    }

    using TestFunction = void (*)();

    void RunTest(const char* name, TestFunction test) {
        int before = g_testFailures;
        std::cout << name << " ... ";
        try {
            test();
        } catch (const std::exception& e) {
            ++g_testFailures;
            std::cout << "\n  ошибка " << e.what();
        }
        std::cout << (g_testFailures == before ? "ok" : "провал") << "\n";
    }
}

int RunAllTests() {
    g_testFailures = 0;
    g_testChecks = 0;
    RunTest("динамический массив", TestDynamicArray);
    RunTest("базовые операции", TestGraphBasics);
    RunTest("граничные случаи", TestEdgeCases);
    RunTest("дейкстра", TestDijkstra);
    RunTest("компоненты сильной связности", TestStrongComponents);
    RunTest("сравнение дейкстры", TestDijkstraEquality);
    RunTest("длинная цепь 100к", TestLongChain);
    RunTest("ленивый граф 1млн", TestLazyGraph);
    RunTest("сетка", TestGrid);
    RunTest("очередь с приоритетом", TestPriorityQueue);
    RunTest("генератор", TestGenerator);
    RunTest("csv", TestCsv);

    std::cout << "проверок " << g_testChecks << " провалов " << g_testFailures << "\n";
    return g_testFailures;
}