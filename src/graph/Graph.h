#pragma once

#include <memory>
#include <utility>
#include "../core/SparseMatrix.h"
#include "../graph/IGraph.h"

// обычный граф, рёбра лежат в разреженной матрице смежности: ячейка (from, to) это ребро
// веса любые, в том числе отрицательные, ограничения накладывают уже алгоритмы
// между парой вершин может быть одно ребро в каждом направлении:
// повторный AddEdge с теми же from и to заменяет вес
template <EdgeWeight W>
class Graph : public IGraph<W> {
private:
    // конкретное ребро спрятано внутри графа, наружу видно только IEdge
    class Edge final : public IEdge<W> {
    private:
        int from_;
        int to_;
        W weight_;
    public:
        Edge() : from_(-1), to_(-1), weight_(W(0)) {}
        Edge(int f, int t, W w) : from_(f), to_(t), weight_(w) {}
        int From() const override { return from_; }
        int To() const override { return to_; }
        W Weight() const override { return weight_; }
    };

    SparseMatrix<std::shared_ptr<const Edge>> matrix_;

public:
    // отрицательное число вершин превращается в 0, для проверки используйте Create
    explicit Graph(int verticesCount = 0)
            : matrix_(verticesCount < 0 ? 0 : verticesCount, verticesCount < 0 ? 0 : verticesCount) {}

    static ResultInfo<Graph> Create(int verticesCount) {
        if (verticesCount < 0) return ResultInfo<Graph>::Failure("vertices count must not be negative");
        return ResultInfo<Graph>::Success(Graph(verticesCount));
    }

    // Success(true): добавлено новое ребро, Success(false): заменён вес существующего
    ResultInfo<bool> AddEdge(int from, int to, W weight = W(1)) {
        return matrix_.Set(from, to, std::make_shared<const Edge>(from, to, weight));
    }

    int GetVerticesCount() const override { return matrix_.Rows(); }

    size_t GetEdgesCount() const { return matrix_.NonZeroCount(); }

    bool HasEdge(int from, int to) const { return matrix_.Has(from, to); }

    ResultInfo<W> GetEdgeWeight(int from, int to) const {
        return matrix_.Get(from, to).Map([](const std::shared_ptr<const Edge>& e) { return e->Weight(); });
    }

    ResultInfo<EdgeList<W>> GetNeighbors(int vertexId) const override {
        using R = ResultInfo<EdgeList<W>>;
        const auto* row = matrix_.RowPtr(vertexId);
        if (!row) return R::Failure("vertex id out of bounds");
        EdgeList<W> result;
        for (size_t i = 0; i < row->GetSize(); ++i) {
            result.PushBack((*row)[i].value);
        }
        return R::Success(std::move(result));
    }
};