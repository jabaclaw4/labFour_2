#pragma once

#include <utility>
#include "core/Common.h"
#include "core/DynamicArray.h"
#include "core/ResultInfo.h"
#include "graph/IGraph.h"

// типы входов и результатов алгоритмов, самих алгоритмов здесь нет

// вход алгоритма, которому нужна стартовая вершина
struct StartVertex {
    int vertex;
};

// вход алгоритма, которому ничего кроме графа не нужно
struct NoInput {};

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