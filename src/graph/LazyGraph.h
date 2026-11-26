#pragma once

#include <functional>
#include <memory>
#include <utility>
#include "../graph/IGraph.h"

// генератор соседей: по номеру вершины возвращает пары (куда, вес)
// класс ребра пользователю не нужен
template <EdgeWeight W>
using NeighborGenerator = std::function<DynamicArray<std::pair<int, W>>(int)>;

// ленивый граф: рёбра не хранятся, а вычисляются функцией при запросе
// добавить ребро нельзя
template <EdgeWeight W>
class LazyGraph : public IGraph<W> {
private:
    class Edge final : public IEdge<W> {
    private:
        int from_;
        int to_;
        W weight_;
    public:
        Edge(int f, int t, W w) : from_(f), to_(t), weight_(w) {}
        int From() const override { return from_; }
        int To() const override { return to_; }
        W Weight() const override { return weight_; }
    };

    int numVertices;
    NeighborGenerator<W> generator;

public:
    LazyGraph() : numVertices(0), generator() {}

    // без проверки: отрицательное число вершин станет 0, пустой генератор даст граф без рёбер
    LazyGraph(int verticesCount, NeighborGenerator<W> gen)
            : numVertices(verticesCount < 0 ? 0 : verticesCount), generator(std::move(gen)) {}

    static ResultInfo<LazyGraph> Create(int verticesCount, NeighborGenerator<W> gen) {
        if (verticesCount < 0) return ResultInfo<LazyGraph>::Failure("vertices count must not be negative");
        if (!gen) return ResultInfo<LazyGraph>::Failure("generator must not be empty");
        return ResultInfo<LazyGraph>::Success(LazyGraph(verticesCount, std::move(gen)));
    }

    int GetVerticesCount() const override { return numVertices; }

    // рёбра с несуществующей целью, которые вернул генератор, отбрасываются
    ResultInfo<EdgeList<W>> GetNeighbors(int vertexId) const override {
        using R = ResultInfo<EdgeList<W>>;
        if (vertexId < 0 || vertexId >= numVertices) return R::Failure("vertex id out of bounds");
        EdgeList<W> result;
        if (!generator) return R::Success(std::move(result));

        DynamicArray<std::pair<int, W>> raw = generator(vertexId);
        for (size_t i = 0; i < raw.GetSize(); ++i) {
            int to = raw[i].first;
            if (to < 0 || to >= numVertices) continue;
            result.PushBack(std::make_shared<const Edge>(vertexId, to, raw[i].second));
        }
        return R::Success(std::move(result));
    }
};