#pragma once

#include "../core/Common.h"
#include "../core/DynamicArray.h"
#include "../core/ResultInfo.h"
#include "../graph/IGraph.h"

// результат поиска компонент сильной связности
struct SccResult {
    DynamicArray<int> component; // номер компоненты для каждой вершины
    int count;                   // число компонент
    SccResult() : component(), count(0) {}
};

// результат поиска кратчайших путей из одной вершины
template <EdgeWeight W>
struct ShortestPathResult {
    int start;
    DynamicArray<W> dist;
    DynamicArray<int> prev;

    ShortestPathResult() : start(-1), dist(), prev() {}

    bool IsReachable(int v) const {
        return v >= 0 && static_cast<size_t>(v) < dist.GetSize() && dist[v] != InfinityOf<W>();
    }

    // путь от start до target, ошибка если вершина недостижима
    ResultInfo<DynamicArray<int>> PathTo(int target) const {
        using R = ResultInfo<DynamicArray<int>>;
        if (!IsReachable(target)) return R::Failure("target is unreachable or out of range");

        DynamicArray<int> reversed;
        int cur = target;
        while (cur != -1) {
            reversed.PushBack(cur);
            if (cur == start) break;
            cur = prev[cur];
        }
        DynamicArray<int> path;
        for (size_t i = reversed.GetSize(); i > 0; --i) {
            path.PushBack(reversed[i - 1]);
        }
        return R::Success(std::move(path));
    }
};

// общий интерфейс алгоритмов на графах, конкретные алгоритмы это его реализации
template <EdgeWeight W>
class IGraphAlgorithms {
public:
    virtual ~IGraphAlgorithms() = default;
    virtual const char* Name() const = 0;
    virtual ResultInfo<ShortestPathResult<W>> ShortestPaths(const IGraph<W>& graph, int start) const = 0;
    virtual ResultInfo<SccResult> StrongComponents(const IGraph<W>& graph) const = 0;
};