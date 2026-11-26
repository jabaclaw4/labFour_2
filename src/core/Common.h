#pragma once

#include <limits>
#include "core/Concepts.h"

// бесконечное расстояние для целочисленных весов
constexpr int INF = std::numeric_limits<int>::max();

// бесконечность для любого типа веса
template <EdgeWeight W>
constexpr W InfinityOf() {
    return std::numeric_limits<W>::max();
}