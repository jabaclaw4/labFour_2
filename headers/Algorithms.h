#pragma once

#include <limits>
#include <stdexcept>
#include <utility>
#include "Graph.h"
#include "DynamicArray.h"
#include "PriorityQueue.h"

// результат поиска компонент сильной связности
struct SccResult {
    DynamicArray<int> component; // номер компоненты для каждой вершины
    int count;                   // число компонент
    SccResult() : component(), count(0) {}
};

template <typename W>
class GraphAlgorithms {
private:
    // состояние поиска кратчайших путей, общее для обеих версий дейкстры
    struct SsspState {
        DynamicArray<W> dist;
        DynamicArray<int> prev;
        DynamicArray<bool> visited;
        SsspState(int n, int start)
                : dist(n, std::numeric_limits<W>::max()), prev(n, -1), visited(n, false) {
            dist[start] = W(0);
        }
    };

    // кадр итеративного обхода в глубину, соседи запрашиваются один раз
    struct Frame {
        int v;
        size_t next;
        DynamicArray<Edge<W>> nb;
        Frame() : v(-1), next(0), nb() {}
        Frame(int vertex, DynamicArray<Edge<W>> neighbors)
                : v(vertex), next(0), nb(std::move(neighbors)) {}
    };

    static void CheckStart(const IGraph<W>& graph, int start) {
        if (start < 0 || start >= graph.GetVerticesCount()) {
            throw std::out_of_range("start out of bounds");
        }
    }

    // пробует улучшить расстояние до e.to через u, без переполнения
    static bool TryRelax(SsspState& s, int u, const Edge<W>& e) {
        const W inf = std::numeric_limits<W>::max();
        if (e.weight < W(0)) throw std::invalid_argument("negative weight found");
        if (s.dist[u] == inf) return false;
        if (e.weight > inf - s.dist[u]) return false; // сумма не помещается в тип

        W candidate = s.dist[u] + e.weight;
        if (candidate < s.dist[e.to]) {
            s.dist[e.to] = candidate;
            s.prev[e.to] = u;
            return true;
        }
        return false;
    }

    // ближайшая ещё не закреплённая вершина, -1 если таких нет
    static int FindNearest(const SsspState& s) {
        const W inf = std::numeric_limits<W>::max();
        int best = -1;
        W bestDist = inf;
        for (size_t i = 0; i < s.dist.GetSize(); ++i) {
            if (!s.visited[i] && s.dist[i] < bestDist) {
                bestDist = s.dist[i];
                best = static_cast<int>(i);
            }
        }
        return best;
    }

public:
    // восстановление пути по массиву предков, пустой путь если вершина недостижима
    static DynamicArray<int> RestorePath(const DynamicArray<int>& prev, int start, int target) {
        DynamicArray<int> path;
        int n = static_cast<int>(prev.GetSize());
        if (start < 0 || start >= n || target < 0 || target >= n) return path;
        if (target != start && prev[target] == -1) return path;

        int curr = target;
        while (curr != -1) {
            path.PushBack(curr);
            if (curr == start) break;
            curr = prev[curr];
        }

        DynamicArray<int> reversed;
        for (size_t i = path.GetSize(); i > 0; --i) {
            reversed.PushBack(path[i - 1]);
        }
        return reversed;
    }

    // дейкстра с кучей, O((V + E) log V)
    static DynamicArray<W> Dijkstra(const IGraph<W>& graph, int start, DynamicArray<int>* outPrev = nullptr) {
        CheckStart(graph, start);
        SsspState s(graph.GetVerticesCount(), start);

        PriorityQueue<int, W> pq;
        pq.Enqueue(start, W(0));

        while (!pq.IsEmpty()) {
            int u = pq.Dequeue();
            if (s.visited[u]) continue; // устаревшая запись в очереди
            s.visited[u] = true;

            DynamicArray<Edge<W>> neighbors = graph.GetNeighbors(u);
            for (size_t i = 0; i < neighbors.GetSize(); ++i) {
                if (TryRelax(s, u, neighbors[i])) {
                    pq.Enqueue(neighbors[i].to, s.dist[neighbors[i].to]);
                }
            }
        }

        if (outPrev) *outPrev = s.prev;
        return std::move(s.dist);
    }

    // дейкстра с линейным поиском ближайшей вершины, O(V^2 + E), для сравнения
    static DynamicArray<W> DijkstraArray(const IGraph<W>& graph, int start, DynamicArray<int>* outPrev = nullptr) {
        CheckStart(graph, start);
        SsspState s(graph.GetVerticesCount(), start);

        while (true) {
            int u = FindNearest(s);
            if (u == -1) break;
            s.visited[u] = true;

            DynamicArray<Edge<W>> neighbors = graph.GetNeighbors(u);
            for (size_t i = 0; i < neighbors.GetSize(); ++i) {
                TryRelax(s, u, neighbors[i]);
            }
        }

        if (outPrev) *outPrev = s.prev;
        return std::move(s.dist);
    }

    // алгоритм тарьяна, обход в глубину без рекурсии, O(V + E)
    static SccResult StrongComponents(const IGraph<W>& graph) {
        int n = graph.GetVerticesCount();
        SccResult result;
        result.component = DynamicArray<int>(n, -1);
        if (n == 0) return result;

        DynamicArray<int> index(n, -1);
        DynamicArray<int> lowlink(n, -1);
        DynamicArray<bool> onStack(n, false);
        DynamicArray<int> tStack;
        DynamicArray<Frame> callStack;
        int idx = 0;

        for (int root = 0; root < n; ++root) {
            if (index[root] != -1) continue;

            index[root] = lowlink[root] = idx++;
            tStack.PushBack(root);
            onStack[root] = true;
            callStack.PushBack(Frame(root, graph.GetNeighbors(root)));

            while (!callStack.IsEmpty()) {
                Frame& curr = callStack[callStack.GetSize() - 1];
                int v = curr.v;

                if (curr.next < curr.nb.GetSize()) {
                    int w = curr.nb[curr.next].to;
                    ++curr.next;

                    if (index[w] == -1) {
                        // идем вглубь, curr после PushBack не используется
                        index[w] = lowlink[w] = idx++;
                        tStack.PushBack(w);
                        onStack[w] = true;
                        callStack.PushBack(Frame(w, graph.GetNeighbors(w)));
                    } else if (onStack[w] && index[w] < lowlink[v]) {
                        lowlink[v] = index[w];
                    }
                } else {
                    // все соседи обработаны
                    if (lowlink[v] == index[v]) {
                        int w;
                        do {
                            w = tStack[tStack.GetSize() - 1];
                            tStack.PopBack();
                            onStack[w] = false;
                            result.component[w] = result.count;
                        } while (w != v);
                        ++result.count;
                    }
                    callStack.PopBack();

                    if (!callStack.IsEmpty()) {
                        int parent = callStack[callStack.GetSize() - 1].v;
                        if (lowlink[v] < lowlink[parent]) lowlink[parent] = lowlink[v];
                    }
                }
            }
        }
        return result;
    }
};