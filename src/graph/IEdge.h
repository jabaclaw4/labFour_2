#pragma once

#include <memory>
#include "../core/Concepts.h"
#include "../core/DynamicArray.h"

// публичный интерфейс ребра: только чтение
// создать ребро снаружи нельзя, конкретные классы спрятаны внутри Graph и LazyGraph
template <EdgeWeight W>
class IEdge {
public:
    virtual ~IEdge() = default;
    virtual int From() const = 0;
    virtual int To() const = 0;
    virtual W Weight() const = 0;
};

template <EdgeWeight W>
using EdgePtr = std::shared_ptr<const IEdge<W>>;

template <EdgeWeight W>
using EdgeList = DynamicArray<EdgePtr<W>>;