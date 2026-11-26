#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include "algorithms/ArrayDijkstra.h"
#include "algorithms/HeapDijkstra.h"
#include "algorithms/Tarjan.h"
#include "benchmark/Benchmark.h"
#include "core/Common.h"
#include "graph/Graph.h"
#include "graph/GraphGenerator.h"
#include "graph/LazyGraph.h"
#include "gui/GuiApp.h"
#include "visualization/HtmlVisualizer.h"

int RunAllTests(); // определена в tests/TestRunner.cpp

namespace {

    const int PRINT_LIMIT = 100;

// чтение целого в диапазоне, false если ввод закончился
    bool ReadInt(const std::string& prompt, int lo, int hi, int& out) {
        while (true) {
            std::cout << prompt;
            if (std::cin >> out) {
                if (out >= lo && out <= hi) return true;
                std::cout << "число должно быть от " << lo << " до " << hi << "\n";
            } else {
                if (std::cin.eof()) return false;
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "нужно ввести целое число\n";
            }
        }
    }

    void PrintError(const ResultInfo<bool>& r) {
        std::cout << "ошибка: " << r.GetError() << "\n";
    }

    void PrintGraph(const Graph<int>& g) {
        int n = g.GetVerticesCount();
        std::cout << "вершин: " << n << ", рёбер: " << g.GetEdgesCount() << "\n";
        if (n > PRINT_LIMIT) {
            std::cout << "граф слишком большой для печати\n";
            return;
        }
        for (int i = 0; i < n; ++i) {
            ResultInfo<EdgeList<int>> nb = g.GetNeighbors(i);
            if (nb.IsFailure()) continue;
            const EdgeList<int>& es = nb.GetValue();
            std::cout << i << " ->";
            if (es.IsEmpty()) std::cout << " (нет рёбер)";
            for (size_t j = 0; j < es.GetSize(); ++j) {
                std::cout << " " << es[j]->To() << "(" << es[j]->Weight() << ")";
            }
            std::cout << "\n";
        }
    }

    void PrintDistances(const ShortestPathResult<int>& sp) {
        int n = static_cast<int>(sp.dist.GetSize());
        int shown = n < PRINT_LIMIT ? n : PRINT_LIMIT;
        for (int i = 0; i < shown; ++i) {
            std::cout << "  до " << i << ": ";
            if (sp.IsReachable(i)) std::cout << sp.dist[i] << "\n";
            else std::cout << "недостижима\n";
        }
        if (n > shown) std::cout << "  ... (показано " << shown << " из " << n << ")\n";
    }

    void PrintPath(const ResultInfo<DynamicArray<int>>& path) {
        if (path.IsFailure()) {
            std::cout << "пути нет\n";
            return;
        }
        std::cout << "путь:";
        for (size_t i = 0; i < path.GetValue().GetSize(); ++i) std::cout << " " << path.GetValue()[i];
        std::cout << "\n";
    }

    bool HasGraph(const Graph<int>& g) {
        if (g.GetVerticesCount() == 0) {
            std::cout << "сначала создайте граф\n";
            return false;
        }
        return true;
    }

    void CreateManual(Graph<int>& g) {
        int n, m;
        if (!ReadInt("число вершин (1..100000): ", 1, 100000, n)) return;
        if (!ReadInt("число рёбер (0..1000000): ", 0, 1000000, m)) return;

        Graph<int> graph(n);
        for (int i = 0; i < m; ++i) {
            int from, to, w;
            std::cout << "ребро " << (i + 1) << " из " << m << "\n";
            if (!ReadInt("  откуда: ", 0, n - 1, from)) return;
            if (!ReadInt("  куда: ", 0, n - 1, to)) return;
            if (!ReadInt("  вес (можно отрицательный): ", -1000000, 1000000, w)) return;
            ResultInfo<bool> r = graph.AddEdge(from, to, w);
            if (r.IsFailure()) PrintError(r);
            else if (!r.GetValue()) std::cout << "  такое ребро уже было, вес заменён\n";
        }
        g = std::move(graph);
        std::cout << "граф создан\n";
    }

    void CreateAuto(Graph<int>& g) {
        int type, n;
        std::cout << "тип графа: 1 цепь, 2 полный, 3 случайный\n";
        if (!ReadInt("выбор: ", 1, 3, type)) return;
        int maxN = (type == 2) ? 300 : 100000;
        if (!ReadInt("число вершин (1.." + std::to_string(maxN) + "): ", 1, maxN, n)) return;

        Graph<int> graph(n);
        ResultInfo<bool> r = ResultInfo<bool>::Success(true);
        if (type == 1) {
            r = GraphGenerator<int>::MakeChain(graph);
        } else if (type == 2) {
            r = GraphGenerator<int>::MakeComplete(graph);
        } else {
            int edges, seed, minW, maxW;
            if (!ReadInt("число рёбер (0..1000000): ", 0, 1000000, edges)) return;
            if (!ReadInt("зерно генератора: ", 0, INF, seed)) return;
            if (!ReadInt("минимальный вес (может быть < 0): ", -1000000, 1000000, minW)) return;
            if (!ReadInt("максимальный вес: ", minW, 1000000, maxW)) return;
            r = GraphGenerator<int>::MakeRandom(graph, edges, static_cast<unsigned>(seed), minW, maxW);
        }
        if (r.IsFailure()) {
            PrintError(r);
            return;
        }
        g = std::move(graph);
        std::cout << "граф создан\n";
        PrintGraph(g);
    }

    void RunShortestPaths(const Graph<int>& g) {
        if (!HasGraph(g)) return;
        int n = g.GetVerticesCount();
        int start, target;
        if (!ReadInt("стартовая вершина: ", 0, n - 1, start)) return;

        HeapDijkstra<int> heap;
        ArrayDijkstra<int> arr;
        ResultInfo<ShortestPathResult<int>> r = heap.Run(g, StartVertex{start});
        if (r.IsFailure()) {
            std::cout << "ошибка: " << r.GetError() << "\n";
            return;
        }
        const ShortestPathResult<int>& sp = r.GetValue();
        std::cout << "расстояния от вершины " << start << ":\n";
        PrintDistances(sp);

        if (!ReadInt("вершина для пути (-1 чтобы пропустить): ", -1, n - 1, target)) return;
        ResultInfo<DynamicArray<int>> path = ResultInfo<DynamicArray<int>>::Failure("no target");
        if (target >= 0) {
            path = sp.PathTo(target);
            PrintPath(path);
        }

        ResultInfo<double> th = Benchmark<int>::MeasureTime(heap, g, start);
        ResultInfo<double> ta = Benchmark<int>::MeasureTime(arr, g, start);
        if (th.IsSuccess() && ta.IsSuccess()) {
            std::cout << std::fixed << std::setprecision(3) << "время: куча " << th.GetValue()
                      << " мс, массив " << ta.GetValue() << " мс\n";
            std::cout.unsetf(std::ios::floatfield);
        }

        ResultInfo<bool> html = HtmlVisualizer<int>::Generate("graph.html", g, &sp,
                                                              path.IsSuccess() ? &path.GetValue() : nullptr);
        if (html.IsSuccess()) std::cout << "graph.html создан\n";
        else PrintError(html);
    }

    void RunComponents(const Graph<int>& g) {
        if (!HasGraph(g)) return;
        Tarjan<int> alg;
        ResultInfo<SccResult> r = alg.Run(g, NoInput{});
        if (r.IsFailure()) {
            std::cout << "ошибка: " << r.GetError() << "\n";
            return;
        }
        const SccResult& scc = r.GetValue();
        std::cout << "компонент сильной связности: " << scc.count << "\n";
        int n = g.GetVerticesCount();
        if (n > PRINT_LIMIT) {
            std::cout << "граф слишком большой для печати компонент\n";
            return;
        }
        for (int c = 0; c < scc.count; ++c) {
            std::cout << "  компонента " << c << ":";
            for (int v = 0; v < n; ++v) {
                if (scc.component[v] == c) std::cout << " " << v;
            }
            std::cout << "\n";
        }
    }

// ленивый граф: сетка, рёбра вычисляются при запросе
    void RunLazy() {
        int w, h;
        if (!ReadInt("ширина сетки (1..1000): ", 1, 1000, w)) return;
        if (!ReadInt("высота сетки (1..1000): ", 1, 1000, h)) return;

        ResultInfo<LazyGraph<int>> created = LazyGraph<int>::Create(
                w * h, [w, h](int id) { return GraphGenerator<int>::GridGenerator(id, w, h); });
        if (created.IsFailure()) {
            std::cout << "ошибка: " << created.GetError() << "\n";
            return;
        }
        const LazyGraph<int>& lg = created.GetValue();

        HeapDijkstra<int> alg;
        ResultInfo<ShortestPathResult<int>> r = alg.Run(lg, StartVertex{0});
        if (r.IsFailure()) {
            std::cout << "ошибка: " << r.GetError() << "\n";
            return;
        }
        const ShortestPathResult<int>& sp = r.GetValue();
        std::cout << "вершин в сетке: " << lg.GetVerticesCount() << " (рёбра не хранятся)\n";
        std::cout << "расстояние от клетки 0 до последней: " << sp.dist[w * h - 1] << "\n";
        if (w * h <= PRINT_LIMIT) {
            for (int row = 0; row < h; ++row) {
                for (int c = 0; c < w; ++c) std::cout << std::setw(4) << sp.dist[row * w + c];
                std::cout << "\n";
            }
        }
        HtmlVisualizer<int>::Generate("lazy.html", lg, &sp);
        std::cout << "lazy.html создан\n";
    }

    void RunBenchmark() {
        int perVertex, seed;
        if (!ReadInt("рёбер на вершину (1..20): ", 1, 20, perVertex)) return;
        if (!ReadInt("зерно генератора: ", 0, INF, seed)) return;

        DynamicArray<int> sizes;
        sizes.PushBack(500);
        sizes.PushBack(1000);
        sizes.PushBack(2000);
        sizes.PushBack(4000);

        std::cout << "замеры, подождите...\n";
        ResultInfo<DynamicArray<BenchRow>> res =
                Benchmark<int>::Run(sizes, perVertex, static_cast<unsigned>(seed), 100);
        if (res.IsFailure()) {
            std::cout << "ошибка: " << res.GetError() << "\n";
            return;
        }
        const DynamicArray<BenchRow>& rows = res.GetValue();

        std::cout << std::fixed << std::setprecision(3);
        std::cout << std::setw(12) << "порядок" << std::setw(8) << "n" << std::setw(10) << "рёбер"
                  << std::setw(12) << "куча, мс" << std::setw(12) << "массив, мс" << "\n";
        for (size_t i = 0; i < rows.GetSize(); ++i) {
            std::cout << std::setw(12) << rows[i].order << std::setw(8) << rows[i].n << std::setw(10)
                      << rows[i].edges << std::setw(12) << rows[i].heapMs << std::setw(12)
                      << rows[i].arrayMs << "\n";
        }
        std::cout.unsetf(std::ios::floatfield);

        ResultInfo<bool> csv = Benchmark<int>::ExportToCSV("benchmark.csv", rows);
        if (csv.IsSuccess()) std::cout << "результаты записаны в benchmark.csv\n";
        else PrintError(csv);
    }

} // namespace

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    Graph<int> graph;

    while (true) {
        std::cout << "\n1 запустить тесты\n"
                  << "2 открыть окно GUI\n"
                  << "3 ввести граф вручную\n"
                  << "4 сгенерировать граф\n"
                  << "5 показать граф\n"
                  << "6 кратчайшие пути (Дейкстра)\n"
                  << "7 компоненты сильной связности (Тарьян)\n"
                  << "8 ленивый граф (сетка)\n"
                  << "9 сравнение по времени и выгрузка в CSV\n"
                  << "0 выход\n";
        int choice;
        if (!ReadInt("выбор: ", 0, 9, choice)) break;
        if (choice == 0) break;

        switch (choice) {
            case 1: RunAllTests(); break;
            case 2: RunGui(); break;
            case 3: CreateManual(graph); break;
            case 4: CreateAuto(graph); break;
            case 5: if (HasGraph(graph)) PrintGraph(graph); break;
            case 6: RunShortestPaths(graph); break;
            case 7: RunComponents(graph); break;
            case 8: RunLazy(); break;
            case 9: RunBenchmark(); break;
        }
    }
    return 0;
}