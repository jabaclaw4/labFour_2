#pragma once

#include "algorithms/AlgorithmDetail.h"
#include "algorithms/IGraphAlgorithm.h"
#include "core/PriorityQueue.h"

// дейкстра с кучей, O((V + E) log V)
// только неотрицательные веса: на отрицательном ребре возвращается ошибка
template <EdgeWeight W>
class HeapDijkstra final : public IShortestPath<W> {
public:
    const char* Name() const override { return "Dijkstra (heap)"; }

    ResultInfo<ShortestPathResult<W>> Run(const IGraph<W>& graph, const StartVertex& input) const override {
        using R = ResultInfo<ShortestPathResult<W>>;
        int start = input.vertex;
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