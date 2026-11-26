#pragma once

#include <chrono>
#include <fstream>
#include <string>
#include "algorithms/ArrayDijkstra.h"
#include "algorithms/HeapDijkstra.h"
#include "graph/GraphGenerator.h"

// одна строка таблицы замеров
struct BenchRow {
    std::string order;
    int n;
    size_t edges;
    double heapMs;
    double arrayMs;
    BenchRow() : order(), n(0), edges(0), heapMs(0), arrayMs(0) {}
};

template <EdgeWeight W>
class Benchmark {
public:
    // время одного алгоритма в миллисекундах, берём лучшее из repeats запусков
    static ResultInfo<double> MeasureTime(const IShortestPath<W>& alg, const IGraph<W>& graph,
                                          int start, int repeats = 3) {
        double best = -1;
        for (int r = 0; r < repeats; ++r) {
            auto t1 = std::chrono::high_resolution_clock::now();
            ResultInfo<ShortestPathResult<W>> res = alg.Run(graph, StartVertex{start});
            auto t2 = std::chrono::high_resolution_clock::now();
            if (res.IsFailure()) return ResultInfo<double>::Failure(res.GetError());
            double ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
            if (best < 0 || ms < best) best = ms;
        }
        return ResultInfo<double>::Success(best);
    }

    // сравнение двух реализаций на одинаковых данных: рёбра по возрастанию, по убыванию, случайно
    static ResultInfo<DynamicArray<BenchRow>> Run(const DynamicArray<int>& sizes, int perVertex,
                                                  unsigned seed, W maxWeight) {
        using R = ResultInfo<DynamicArray<BenchRow>>;
        HeapDijkstra<W> heap;
        ArrayDijkstra<W> arr;
        DynamicArray<BenchRow> rows;
        EdgeOrder orders[3] = {EdgeOrder::Ascending, EdgeOrder::Descending, EdgeOrder::Random};
        const char* names[3] = {"ascending", "descending", "random"};

        for (size_t s = 0; s < sizes.GetSize(); ++s) {
            int n = sizes[s];
            for (int o = 0; o < 3; ++o) {
                Graph<W> g(n);
                ResultInfo<bool> gen = GraphGenerator<W>::MakeOrdered(g, perVertex, seed, maxWeight, orders[o]);
                if (gen.IsFailure()) return R::Failure(gen.GetError());

                ResultInfo<double> th = MeasureTime(heap, g, 0);
                if (th.IsFailure()) return R::Failure(th.GetError());
                ResultInfo<double> ta = MeasureTime(arr, g, 0);
                if (ta.IsFailure()) return R::Failure(ta.GetError());

                BenchRow row;
                row.order = names[o];
                row.n = n;
                row.edges = g.GetEdgesCount();
                row.heapMs = th.GetValue();
                row.arrayMs = ta.GetValue();
                rows.PushBack(row);
            }
        }
        return R::Success(std::move(rows));
    }

    static ResultInfo<bool> ExportToCSV(const std::string& filename, const DynamicArray<BenchRow>& rows) {
        std::ofstream out(filename);
        if (!out.is_open()) return ResultInfo<bool>::Failure("cannot open output file");
        out << "order,n,edges,heap_ms,array_ms\n";
        for (size_t i = 0; i < rows.GetSize(); ++i) {
            out << rows[i].order << "," << rows[i].n << "," << rows[i].edges << ","
                << rows[i].heapMs << "," << rows[i].arrayMs << "\n";
        }
        return ResultInfo<bool>::Success(true);
    }
};