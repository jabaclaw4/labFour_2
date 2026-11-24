#pragma once

#include <stdexcept>
#include <utility>
#include "DynamicArray.h"

// очередь с приоритетом на бинарной куче, первым выходит минимальный приоритет
// при равных приоритетах порядок выдачи совпадает с порядком добавления
template <typename T, typename K>
class PriorityQueue {
private:
    struct Node {
        T item;
        K priority;
        int order; // для детерминизма при равных приоритетах
        Node() : item(), priority(), order(0) {}
        Node(const T& i, const K& p, int o) : item(i), priority(p), order(o) {}
    };

    DynamicArray<Node> heap;
    int counter;

    // a должен стоять в куче выше b
    static bool Less(const Node& a, const Node& b) {
        if (a.priority < b.priority) return true;
        if (b.priority < a.priority) return false;
        return a.order < b.order;
    }

    void SiftUp(size_t index) {
        while (index > 0) {
            size_t parent = (index - 1) / 2;
            if (!Less(heap[index], heap[parent])) break;
            std::swap(heap[index], heap[parent]);
            index = parent;
        }
    }

    void SiftDown(size_t index) {
        size_t size = heap.GetSize();
        while (2 * index + 1 < size) {
            size_t smallest = 2 * index + 1;
            size_t right = smallest + 1;
            if (right < size && Less(heap[right], heap[smallest])) smallest = right;
            if (!Less(heap[smallest], heap[index])) break;
            std::swap(heap[index], heap[smallest]);
            index = smallest;
        }
    }

public:
    PriorityQueue() : heap(), counter(0) {}

    size_t GetCount() const { return heap.GetSize(); }
    bool IsEmpty() const { return heap.IsEmpty(); }

    void Enqueue(const T& item, const K& priority) {
        heap.PushBack(Node(item, priority, counter++));
        SiftUp(heap.GetSize() - 1);
    }

    T Dequeue() {
        if (heap.IsEmpty()) throw std::out_of_range("queue is empty");
        T result = heap[0].item;
        heap[0] = heap[heap.GetSize() - 1];
        heap.PopBack();
        if (!heap.IsEmpty()) SiftDown(0);
        return result;
    }

    T Peek() const {
        if (heap.IsEmpty()) throw std::out_of_range("queue is empty");
        return heap[0].item;
    }
};