#pragma once

#include <concepts>
#include <limits>
#include <type_traits>

// ключ приоритета: копируемый, с конструктором по умолчанию и полным порядком
template <typename K>
concept PriorityKey = std::copyable<K> && std::default_initializable<K> && std::totally_ordered<K>;

// элемент очереди
template <typename T>
concept QueueItem = std::copyable<T> && std::default_initializable<T>;

// вес ребра: любой арифметический тип кроме bool (int, long long, double ...)
template <typename W>
concept EdgeWeight = std::is_arithmetic_v<W> && !std::is_same_v<W, bool> &&
                     std::numeric_limits<W>::is_specialized;