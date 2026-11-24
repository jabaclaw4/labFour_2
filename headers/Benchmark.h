#pragma once

#include <chrono>
#include <fstream>
#include <string>
#include "Graph.h"
#include "Algorithms.h"
#include "GraphGenerator.h"

// одна строка таблицы замеров
struct BenchRow {
    std::string order;
    int n;
    size_t edges;
    double heapMs;
    double arrayMs;
    BenchRow() : order(), n(0), edges(0), heapMs(0), arrayMs(0) {}
};

template <typename W>
class Benchmark {
public:
    // время одного запуска в миллисекундах, берём лучшее из repeats
    static double MeasureTime(const IGraph<W>& graph, int start, bool useHeap, int repeats = 3) {
        double best = -1;
        for (int r = 0; r < repeats; ++r) {
            auto t1 = std::chrono::high_resolution_clock::now();
            if (useHeap) {
                GraphAlgorithms<W>::Dijkstra(graph, start);
            } else {
                GraphAlgorithms<W>::DijkstraArray(graph, start);
            }
            auto t2 = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
            if (best < 0 || ms < best) best = ms;
        }
        return best;
    }

    // сравнение двух дейкстр на одинаковых данных для каждого размера
    // три случая: рёбра по возрастанию источника, по убыванию, случайно
    static DynamicArray<BenchRow> Run(const DynamicArray<int>& sizes, int perVertex, unsigned seed, W maxWeight) {
        DynamicArray<BenchRow> rows;
        EdgeOrder orders[3] = {EdgeOrder::Ascending, EdgeOrder::Descending, EdgeOrder::Random};
        const char* names[3] = {"ascending", "descending", "random"};

        for (size_t s = 0; s < sizes.GetSize(); ++s) {
            int n = sizes[s];
            for (int o = 0; o < 3; ++o) {
                Graph<W> g(n);
                GraphGenerator<W>::MakeOrdered(g, n, perVertex, seed, maxWeight, orders[o]);

                BenchRow row;
                row.order = names[o];
                row.n = n;
                row.edges = g.GetEdgesCount();
                row.heapMs = MeasureTime(g, 0, true);
                row.arrayMs = MeasureTime(g, 0, false);
                rows.PushBack(row);
            }
        }
        return rows;
    }

    // выгрузка в csv, false если файл не открылся
    static bool ExportToCSV(const std::string& filename, const DynamicArray<BenchRow>& rows) {
        std::ofstream out(filename);
        if (!out.is_open()) return false;
        out << "order,n,edges,heap_ms,array_ms\n";
        for (size_t i = 0; i < rows.GetSize(); ++i) {
            out << rows[i].order << "," << rows[i].n << "," << rows[i].edges << ","
                << rows[i].heapMs << "," << rows[i].arrayMs << "\n";
        }
        return true;
    }
};