#pragma once

#include "algorithms/AlgorithmDetail.h"
#include "algorithms/IGraphAlgorithm.h"

// дейкстра с линейным поиском ближайшей вершины, O(V^2 + E), для сравнения
template <EdgeWeight W>
class ArrayDijkstra final : public IShortestPath<W> {
public:
    const char* Name() const override { return "Dijkstra (array)"; }

    ResultInfo<ShortestPathResult<W>> Run(const IGraph<W>& graph, const StartVertex& input) const override {
        using R = ResultInfo<ShortestPathResult<W>>;
        int start = input.vertex;
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