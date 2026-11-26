#pragma once

#include "algorithms/AlgorithmTypes.h"

// вспомогательные части для алгоритмов кратчайших путей
namespace detail {

    template <EdgeWeight W>
    struct SsspState {
        DynamicArray<W> dist;
        DynamicArray<int> prev;
        DynamicArray<bool> visited;

        SsspState(int n, int start)
                : dist(n, InfinityOf<W>()), prev(n, -1), visited(n, false) {
            dist[start] = W(0);
        }

        ShortestPathResult<W> ToResult(int start) {
            ShortestPathResult<W> r;
            r.start = start;
            r.dist = std::move(dist);
            r.prev = std::move(prev);
            return r;
        }
    };

    template <EdgeWeight W>
    ResultInfo<bool> CheckStart(const IGraph<W>& graph, int start) {
        if (start < 0 || start >= graph.GetVerticesCount()) {
            return ResultInfo<bool>::Failure("start vertex out of range");
        }
        return ResultInfo<bool>::Success(true);
    }

    // пробует улучшить расстояние через ребро e без переполнения
    // Success(true) расстояние улучшено, Success(false) нет, Failure: ребро с отрицательным весом
    template <EdgeWeight W>
    ResultInfo<bool> Relax(SsspState<W>& s, int u, const IEdge<W>& e) {
        const W inf = InfinityOf<W>();
        W weight = e.Weight();
        if (weight < W(0)) return ResultInfo<bool>::Failure("Dijkstra requires non-negative edge weights");
        if (s.dist[u] == inf) return ResultInfo<bool>::Success(false);
        if (weight > inf - s.dist[u]) return ResultInfo<bool>::Success(false); // сумма не помещается в тип

        W candidate = s.dist[u] + weight;
        int to = e.To();
        if (candidate < s.dist[to]) {
            s.dist[to] = candidate;
            s.prev[to] = u;
            return ResultInfo<bool>::Success(true);
        }
        return ResultInfo<bool>::Success(false);
    }

    // ближайшая ещё не закреплённая вершина, -1 если таких нет
    template <EdgeWeight W>
    int FindNearest(const SsspState<W>& s) {
        W bestDist = InfinityOf<W>();
        int best = -1;
        for (size_t i = 0; i < s.dist.GetSize(); ++i) {
            if (!s.visited[i] && s.dist[i] < bestDist) {
                bestDist = s.dist[i];
                best = static_cast<int>(i);
            }
        }
        return best;
    }
}