#pragma once

#include <stdexcept>
#include <functional>
#include "DynamicArray.h"

// обобщенное ребро, вес шаблонный
template <typename W>
struct Edge {
    int to;
    W weight;
    Edge() : to(-1), weight(0) {}
    Edge(int t, W w) : to(t), weight(w) {}
};

// абстрактный интерфейс графа, алгоритмы зависят только от него
template <typename W>
class IGraph {
public:
    virtual ~IGraph() = default;
    virtual int GetVerticesCount() const = 0;
    virtual DynamicArray<Edge<W>> GetNeighbors(int vertexId) const = 0;
};

// обычный граф на списке смежности
// вершина это её номер, поэтому список рёбер лежит по индексу номера
template <typename W>
class Graph : public IGraph<W> {
private:
    DynamicArray<DynamicArray<Edge<W>>> adj;
    size_t edgesCount;

public:
    explicit Graph(int verticesCount) : adj(), edgesCount(0) {
        if (verticesCount < 0) {
            throw std::invalid_argument("vertices count must not be negative");
        }
        for (int i = 0; i < verticesCount; ++i) {
            adj.PushBack(DynamicArray<Edge<W>>());
        }
    }

    void AddEdge(int from, int to, W weight = W(1)) {
        int n = GetVerticesCount();
        if (from < 0 || from >= n || to < 0 || to >= n) {
            throw std::out_of_range("vertex id out of bounds");
        }
        if (weight < W(0)) {
            throw std::invalid_argument("negative weight not allowed");
        }
        adj[from].PushBack(Edge<W>(to, weight));
        ++edgesCount;
    }

    int GetVerticesCount() const override { return static_cast<int>(adj.GetSize()); }

    // число рёбер есть только у графа, который их хранит
    size_t GetEdgesCount() const { return edgesCount; }

    DynamicArray<Edge<W>> GetNeighbors(int vertexId) const override {
        if (vertexId < 0 || vertexId >= GetVerticesCount()) {
            throw std::out_of_range("vertex id out of bounds");
        }
        return adj[vertexId];
    }
};

// ленивый граф: рёбра не хранятся, а вычисляются функцией при запросе
// добавить ребро нельзя
template <typename W>
class LazyGraph : public IGraph<W> {
private:
    int numVertices;
    std::function<DynamicArray<Edge<W>>(int)> generator;

public:
    LazyGraph(int verticesCount, std::function<DynamicArray<Edge<W>>(int)> gen)
            : numVertices(verticesCount), generator(std::move(gen)) {
        if (verticesCount < 0) {
            throw std::invalid_argument("vertices count must not be negative");
        }
    }

    int GetVerticesCount() const override { return numVertices; }

    DynamicArray<Edge<W>> GetNeighbors(int vertexId) const override {
        if (vertexId < 0 || vertexId >= numVertices) {
            throw std::out_of_range("vertex id out of bounds");
        }
        return generator(vertexId);
    }
};