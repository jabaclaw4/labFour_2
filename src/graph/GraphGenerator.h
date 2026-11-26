#pragma once

#include <random>
#include <type_traits>
#include <utility>
#include "../graph/Graph.h"

// порядок добавления рёбер для бенчмарка
enum class EdgeOrder {
    Ascending,   // источники по возрастанию номера
    Descending,  // источники по убыванию номера
    Random       // источники в случайном порядке
};

template <EdgeWeight W>
class GraphGenerator {
private:
    static W PickWeight(std::mt19937& rng, W lo, W hi) {
        if constexpr (std::is_integral_v<W>) {
            std::uniform_int_distribution<long long> d(static_cast<long long>(lo), static_cast<long long>(hi));
            return static_cast<W>(d(rng));
        } else {
            std::uniform_real_distribution<double> d(static_cast<double>(lo), static_cast<double>(hi));
            return static_cast<W>(d(rng));
        }
    }

    static ResultInfo<bool> CheckGraph(const Graph<W>& g) {
        if (g.GetVerticesCount() <= 0) return ResultInfo<bool>::Failure("graph has no vertices");
        return ResultInfo<bool>::Success(true);
    }

public:
    // цепь 0 -> 1 -> ... -> n-1
    static ResultInfo<bool> MakeChain(Graph<W>& g, W weight = W(1)) {
        for (int i = 0; i + 1 < g.GetVerticesCount(); ++i) {
            ResultInfo<bool> r = g.AddEdge(i, i + 1, weight);
            if (r.IsFailure()) return r;
        }
        return ResultInfo<bool>::Success(true);
    }

    // полный граф
    static ResultInfo<bool> MakeComplete(Graph<W>& g, W weight = W(1)) {
        int n = g.GetVerticesCount();
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i == j) continue;
                ResultInfo<bool> r = g.AddEdge(i, j, weight);
                if (r.IsFailure()) return r;
            }
        }
        return ResultInfo<bool>::Success(true);
    }

    // случайный граф, одинаковое зерно даёт одинаковый граф
    // вес берётся из [minWeight, maxWeight], может быть отрицательным
    // повторившаяся пара (from, to) заменяет вес, поэтому рёбер может получиться меньше edges
    static ResultInfo<bool> MakeRandom(Graph<W>& g, int edges, unsigned seed, W minWeight, W maxWeight) {
        ResultInfo<bool> check = CheckGraph(g);
        if (check.IsFailure()) return check;
        if (edges < 0) return ResultInfo<bool>::Failure("edges count must not be negative");
        if (maxWeight < minWeight) return ResultInfo<bool>::Failure("min weight is greater than max weight");

        int n = g.GetVerticesCount();
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> vertexDist(0, n - 1);
        for (int i = 0; i < edges; ++i) {
            int u = vertexDist(rng);
            int v = vertexDist(rng);
            ResultInfo<bool> r = g.AddEdge(u, v, PickWeight(rng, minWeight, maxWeight));
            if (r.IsFailure()) return r;
        }
        return ResultInfo<bool>::Success(true);
    }

    // у каждой вершины perVertex исходящих рёбер с положительным весом, order задаёт порядок источников
    static ResultInfo<bool> MakeOrdered(Graph<W>& g, int perVertex, unsigned seed, W maxWeight, EdgeOrder order) {
        ResultInfo<bool> check = CheckGraph(g);
        if (check.IsFailure()) return check;
        if (perVertex < 0) return ResultInfo<bool>::Failure("edges per vertex must not be negative");
        if (maxWeight < W(1)) return ResultInfo<bool>::Failure("max weight must be at least 1");

        int n = g.GetVerticesCount();
        if (order == EdgeOrder::Random) {
            return MakeRandom(g, n * perVertex, seed, W(1), maxWeight);
        }
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> vertexDist(0, n - 1);
        for (int k = 0; k < n; ++k) {
            int u = (order == EdgeOrder::Ascending) ? k : (n - 1 - k);
            for (int j = 0; j < perVertex; ++j) {
                int v = vertexDist(rng);
                ResultInfo<bool> r = g.AddEdge(u, v, PickWeight(rng, W(1), maxWeight));
                if (r.IsFailure()) return r;
            }
        }
        return ResultInfo<bool>::Success(true);
    }

    // соседи клетки сетки width x height для LazyGraph, вершина = row * width + col
    // если height не задана, сетка квадратная
    static DynamicArray<std::pair<int, W>> GridGenerator(int n, int width, int height = -1) {
        if (height < 0) height = width;
        DynamicArray<std::pair<int, W>> neighbors;
        if (width <= 0 || height <= 0) return neighbors;
        int total = width * height;
        if (n < 0 || n >= total) return neighbors;

        if ((n + 1) % width != 0 && n + 1 < total) neighbors.PushBack(std::pair<int, W>(n + 1, W(1)));  // право
        if (n % width != 0) neighbors.PushBack(std::pair<int, W>(n - 1, W(1)));                         // лево
        if (n + width < total) neighbors.PushBack(std::pair<int, W>(n + width, W(1)));                  // вниз
        if (n - width >= 0) neighbors.PushBack(std::pair<int, W>(n - width, W(1)));                     // вверх
        return neighbors;
    }
};