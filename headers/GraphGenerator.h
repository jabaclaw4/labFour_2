#pragma once

#include <random>
#include <stdexcept>
#include "Graph.h"

// порядок добавления рёбер для бенчмарка
enum class EdgeOrder {
    Ascending,   // источники по возрастанию номера
    Descending,  // источники по убыванию номера
    Random       // источники в случайном порядке
};

template <typename W>
class GraphGenerator {
public:
    // цепь
    static void MakeChain(Graph<W>& g, int n) {
        for (int i = 0; i < n - 1; ++i) g.AddEdge(i, i + 1, W(1));
    }

    // полный граф
    static void MakeComplete(Graph<W>& g, int n) {
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i != j) g.AddEdge(i, j, W(1));
            }
        }
    }

    // случайный граф, одинаковое зерно даёт одинаковый граф
    static void MakeRandom(Graph<W>& g, int n, int edges, unsigned seed, W maxWeight) {
        if (n <= 0 || edges < 0 || maxWeight < W(1)) {
            throw std::invalid_argument("bad generator parameters");
        }
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> vertexDist(0, n - 1);
        std::uniform_int_distribution<int> weightDist(1, static_cast<int>(maxWeight));

        for (int i = 0; i < edges; ++i) {
            int u = vertexDist(rng);
            int v = vertexDist(rng);
            g.AddEdge(u, v, W(weightDist(rng)));
        }
    }

    // у каждой вершины perVertex исходящих рёбер, всего n * perVertex
    // order задаёт, в каком порядке добавляются источники
    static void MakeOrdered(Graph<W>& g, int n, int perVertex, unsigned seed, W maxWeight, EdgeOrder order) {
        if (n <= 0 || perVertex < 0 || maxWeight < W(1)) {
            throw std::invalid_argument("bad generator parameters");
        }
        if (order == EdgeOrder::Random) {
            MakeRandom(g, n, n * perVertex, seed, maxWeight);
            return;
        }
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> vertexDist(0, n - 1);
        std::uniform_int_distribution<int> weightDist(1, static_cast<int>(maxWeight));

        for (int k = 0; k < n; ++k) {
            int u = (order == EdgeOrder::Ascending) ? k : (n - 1 - k);
            for (int j = 0; j < perVertex; ++j) {
                g.AddEdge(u, vertexDist(rng), W(weightDist(rng)));
            }
        }
    }

    // сетка width x height (для LazyGraph), вершина = row * width + col
    // если height не задана, сетка квадратная
    static DynamicArray<Edge<W>> GridGenerator(int n, int width, int height = -1) {
        if (height < 0) height = width;
        DynamicArray<Edge<W>> neighbors;
        int total = width * height;
        if (n < 0 || n >= total) return neighbors;

        // право
        if ((n + 1) % width != 0 && n + 1 < total) neighbors.PushBack(Edge<W>(n + 1, W(1)));
        // лево
        if (n % width != 0) neighbors.PushBack(Edge<W>(n - 1, W(1)));
        // вниз
        if (n + width < total) neighbors.PushBack(Edge<W>(n + width, W(1)));
        // вверх
        if (n - width >= 0) neighbors.PushBack(Edge<W>(n - width, W(1)));
        return neighbors;
    }
};