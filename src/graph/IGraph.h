#pragma once

#include "../core/Concepts.h"
#include "../core/ResultInfo.h"
#include "../graph/IEdge.h"

// абстрактный интерфейс графа, алгоритмы зависят только от него
template <EdgeWeight W>
class IGraph {
public:
    virtual ~IGraph() = default;
    virtual int GetVerticesCount() const = 0;
    // исходящие рёбра вершины, ошибка если номер вершины неверный
    virtual ResultInfo<EdgeList<W>> GetNeighbors(int vertexId) const = 0;
};