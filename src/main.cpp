#include <iostream>
#include <memory>
#include <string>
#include <iomanip>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif
#include "Common.h"
#include "Graph.h"
#include "Algorithms.h"
#include "GraphGenerator.h"
#include "HtmlVisualizer.h"
#include "Benchmark.h"

int RunAllTests();

namespace {

    const int PRINT_LIMIT = 100; // сколько вершин печатать максимум

    // чтение целого числа в диапазоне, false если ввод закончился
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

    void PrintGraph(const Graph<int>& g) {
        int n = g.GetVerticesCount();
        std::cout << "вершин: " << n << ", рёбер: " << g.GetEdgesCount() << "\n";
        if (n > PRINT_LIMIT) {
            std::cout << "граф слишком большой для печати\n";
            return;
        }
        for (int i = 0; i < n; ++i) {
            DynamicArray<Edge<int>> nb = g.GetNeighbors(i);
            std::cout << i << " ->";
            if (nb.IsEmpty()) std::cout << " (нет рёбер)";
            for (size_t j = 0; j < nb.GetSize(); ++j) {
                std::cout << " " << nb[j].to << "(" << nb[j].weight << ")";
            }
            std::cout << "\n";
        }
    }

    void PrintDistances(const DynamicArray<int>& dist) {
        int n = static_cast<int>(dist.GetSize());
        int shown = n < PRINT_LIMIT ? n : PRINT_LIMIT;
        for (int i = 0; i < shown; ++i) {
            std::cout << "  до " << i << ": ";
            if (dist[i] == INF) std::cout << "недостижима\n";
            else std::cout << dist[i] << "\n";
        }
        if (n > shown) std::cout << "  ... (показано " << shown << " из " << n << ")\n";
    }

    void PrintPath(const DynamicArray<int>& path) {
        if (path.IsEmpty()) {
            std::cout << "пути нет\n";
            return;
        }
        std::cout << "путь:";
        for (size_t i = 0; i < path.GetSize(); ++i) std::cout << " " << path[i];
        std::cout << "\n";
    }

    // ручной ввод графа
    void CreateManual(std::unique_ptr<Graph<int>>& g) {
        int n, m;
        if (!ReadInt("число вершин (1..100000): ", 1, 100000, n)) return;
        if (!ReadInt("число рёбер (0..1000000): ", 0, 1000000, m)) return;

        auto graph = std::make_unique<Graph<int>>(n);
        for (int i = 0; i < m; ++i) {
            int from, to, w;
            std::cout << "ребро " << (i + 1) << " из " << m << "\n";
            if (!ReadInt("  откуда: ", 0, n - 1, from)) return;
            if (!ReadInt("  куда: ", 0, n - 1, to)) return;
            if (!ReadInt("  вес (>= 0): ", 0, INF, w)) return;
            graph->AddEdge(from, to, w);
        }
        g = std::move(graph);
        std::cout << "граф создан\n";
    }

    // автоматическая генерация
    void CreateAuto(std::unique_ptr<Graph<int>>& g) {
        int type;
        std::cout << "тип графа: 1 цепь, 2 полный, 3 случайный\n";
        if (!ReadInt("выбор: ", 1, 3, type)) return;

        int n;
        int maxN = (type == 2) ? 300 : 100000;
        if (!ReadInt("число вершин (1.." + std::to_string(maxN) + "): ", 1, maxN, n)) return;

        auto graph = std::make_unique<Graph<int>>(n);
        if (type == 1) {
            GraphGenerator<int>::MakeChain(*graph, n);
        } else if (type == 2) {
            GraphGenerator<int>::MakeComplete(*graph, n);
        } else {
            int edges, seed, maxW;
            if (!ReadInt("число рёбер (0..1000000): ", 0, 1000000, edges)) return;
            if (!ReadInt("зерно генератора: ", 0, INF, seed)) return;
            if (!ReadInt("максимальный вес (1..1000000): ", 1, 1000000, maxW)) return;
            GraphGenerator<int>::MakeRandom(*graph, n, edges, static_cast<unsigned>(seed), maxW);
        }
        g = std::move(graph);
        std::cout << "граф создан\n";
        PrintGraph(*g);
    }

    void RunDijkstra(const std::unique_ptr<Graph<int>>& g) {
        if (!g) {
            std::cout << "сначала создайте граф\n";
            return;
        }
        int n = g->GetVerticesCount();
        int start, target;
        if (!ReadInt("стартовая вершина: ", 0, n - 1, start)) return;

        DynamicArray<int> prev;
        DynamicArray<int> dist = GraphAlgorithms<int>::Dijkstra(*g, start, &prev);
        std::cout << "расстояния от вершины " << start << ":\n";
        PrintDistances(dist);

        if (!ReadInt("вершина для пути (-1 чтобы пропустить): ", -1, n - 1, target)) return;
        DynamicArray<int> path;
        if (target >= 0) {
            path = GraphAlgorithms<int>::RestorePath(prev, start, target);
            PrintPath(path);
        }

        // сравнение двух версий на этом же графе
        double heapMs = Benchmark<int>::MeasureTime(*g, start, true);
        double arrayMs = Benchmark<int>::MeasureTime(*g, start, false);
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "время: куча " << heapMs << " мс, массив " << arrayMs << " мс\n";
        std::cout.unsetf(std::ios::floatfield);

        if (HtmlVisualizer<int>::Generate("graph.html", *g, dist, path.IsEmpty() ? nullptr : &path)) {
            std::cout << "graph.html создан\n";
        } else {
            std::cout << "не удалось записать graph.html\n";
        }
    }

    void RunComponents(const std::unique_ptr<Graph<int>>& g) {
        if (!g) {
            std::cout << "сначала создайте граф\n";
            return;
        }
        int n = g->GetVerticesCount();
        SccResult scc = GraphAlgorithms<int>::StrongComponents(*g);
        std::cout << "компонент сильной связности: " << scc.count << "\n";
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

        LazyGraph<int> lg(w * h, [w, h](int id) { return GraphGenerator<int>::GridGenerator(id, w, h); });
        DynamicArray<int> dist = GraphAlgorithms<int>::Dijkstra(lg, 0);

        std::cout << "вершин в сетке: " << lg.GetVerticesCount() << " (рёбра не хранятся)\n";
        std::cout << "расстояние от клетки 0 до последней клетки: " << dist[w * h - 1] << "\n";

        if (w * h <= PRINT_LIMIT) {
            for (int r = 0; r < h; ++r) {
                for (int c = 0; c < w; ++c) std::cout << std::setw(4) << dist[r * w + c];
                std::cout << "\n";
            }
        }
        HtmlVisualizer<int>::Generate("lazy.html", lg, dist);
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
        DynamicArray<BenchRow> rows = Benchmark<int>::Run(sizes, perVertex, static_cast<unsigned>(seed), 100);

        std::cout << std::fixed << std::setprecision(3);
        std::cout << std::setw(12) << "порядок" << std::setw(8) << "n" << std::setw(10) << "рёбер"
                  << std::setw(12) << "куча, мс" << std::setw(12) << "массив, мс" << "\n";
        for (size_t i = 0; i < rows.GetSize(); ++i) {
            std::cout << std::setw(12) << rows[i].order << std::setw(8) << rows[i].n
                      << std::setw(10) << rows[i].edges << std::setw(12) << rows[i].heapMs
                      << std::setw(12) << rows[i].arrayMs << "\n";
        }
        std::cout.unsetf(std::ios::floatfield);

        if (Benchmark<int>::ExportToCSV("benchmark.csv", rows)) {
            std::cout << "результаты записаны в benchmark.csv\n";
        } else {
            std::cout << "не удалось записать benchmark.csv\n";
        }
    }
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::unique_ptr<Graph<int>> graph;

    while (true) {
        std::cout << "\n1 запустить тесты\n"
                  << "2 ввести граф вручную\n"
                  << "3 сгенерировать граф\n"
                  << "4 показать граф\n"
                  << "5 кратчайшие пути (Дейкстра)\n"
                  << "6 компоненты сильной связности (Тарьян)\n"
                  << "7 ленивый граф (сетка)\n"
                  << "8 сравнение по времени и выгрузка в CSV\n"
                  << "0 выход\n";
        int choice;
        if (!ReadInt("выбор: ", 0, 8, choice)) break;
        if (choice == 0) break;

        try {
            switch (choice) {
                case 1: RunAllTests(); break;
                case 2: CreateManual(graph); break;
                case 3: CreateAuto(graph); break;
                case 4:
                    if (graph) PrintGraph(*graph);
                    else std::cout << "сначала создайте граф\n";
                    break;
                case 5: RunDijkstra(graph); break;
                case 6: RunComponents(graph); break;
                case 7: RunLazy(); break;
                case 8: RunBenchmark(); break;
            }
        } catch (const std::exception& e) {
            std::cout << "ошибка: " << e.what() << "\n";
        }
    }
    return 0;
}