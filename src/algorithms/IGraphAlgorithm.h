#pragma once

#include "algorithms/AlgorithmTypes.h"

// интерфейс одного алгоритма на графе: вход Input, результат Output
// новый алгоритм это новый класс,
template <EdgeWeight W, typename Input, typename Output>
class IGraphAlgorithm {
public:
    virtual ~IGraphAlgorithm() = default;
    virtual const char* Name() const = 0;
    virtual ResultInfo<Output> Run(const IGraph<W>& graph, const Input& input) const = 0;
};

// семейства алгоритмов по смыслу: всё, что ищет кратчайшие пути из вершины, подходит под IShortestPath
template <EdgeWeight W>
using IShortestPath = IGraphAlgorithm<W, StartVertex, ShortestPathResult<W>>;

// всё, что ищет компоненты сильной связности
template <EdgeWeight W>
using IComponents = IGraphAlgorithm<W, NoInput, SccResult>;