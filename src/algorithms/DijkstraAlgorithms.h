#pragma once

#include "../algorithms/AlgorithmDetail.h"
#include "../algorithms/GraphAlgorithmsBase.h"
#include "../core/PriorityQueue.h"

// дейкстра с кучей, O((V + E) log V)
// только неотрицательные веса: на отрицательном ребре возвращается ошибка
template <EdgeWeight W>
class HeapGraphAlgorithms final : public GraphAlgorithmsBase<W> {
public:
    const char* Name() const override { return "Dijkstra (heap)"; }

    ResultInfo<ShortestPathResult<W>> ShortestPaths(const IGraph<W>& graph, int start) const override {
        using R = ResultInfo<ShortestPathResult<W>>;
        ResultInfo<bool> check = detail::CheckStart(graph, start);
        if (check.IsFailure()) return R::Failure(check.GetError());

        detail::SsspState<W> s(graph.GetVerticesCount(), start);
        PriorityQueue<int, W> pq;
        pq.Enqueue(start, W(0));

        while (!pq.IsEmpty()) {
            int u = pq.Dequeue().GetValue();
            if (s.visited[u]) continue; // устаревшая запись в очереди
            s.visited[u] = true;

            ResultInfo<EdgeList<W>> nb = graph.GetNeighbors(u);
            if (nb.IsFailure()) return R::Failure(nb.GetError());
            const EdgeList<W>& edges = nb.GetValue();

            for (size_t i = 0; i < edges.GetSize(); ++i) {
                ResultInfo<bool> relaxed = detail::Relax(s, u, *edges[i]);
                if (relaxed.IsFailure()) return R::Failure(relaxed.GetError());
                if (relaxed.GetValue()) {
                    int to = edges[i]->To();
                    pq.Enqueue(to, s.dist[to]);
                }
            }
        }
        return R::Success(s.ToResult(start));
    }
};

// дейкстра с линейным поиском ближайшей вершины, O(V^2 + E), для сравнения
template <EdgeWeight W>
class ArrayGraphAlgorithms final : public GraphAlgorithmsBase<W> {
public:
    const char* Name() const override { return "Dijkstra (array)"; }

    ResultInfo<ShortestPathResult<W>> ShortestPaths(const IGraph<W>& graph, int start) const override {
        using R = ResultInfo<ShortestPathResult<W>>;
        ResultInfo<bool> check = detail::CheckStart(graph, start);
        if (check.IsFailure()) return R::Failure(check.GetError());

        detail::SsspState<W> s(graph.GetVerticesCount(), start);

        while (true) {
            int u = detail::FindNearest(s);
            if (u == -1) break;
            s.visited[u] = true;

            ResultInfo<EdgeList<W>> nb = graph.GetNeighbors(u);
            if (nb.IsFailure()) return R::Failure(nb.GetError());
            const EdgeList<W>& edges = nb.GetValue();

            for (size_t i = 0; i < edges.GetSize(); ++i) {
                ResultInfo<bool> relaxed = detail::Relax(s, u, *edges[i]);
                if (relaxed.IsFailure()) return R::Failure(relaxed.GetError());
            }
        }
        return R::Success(s.ToResult(start));
    }
};