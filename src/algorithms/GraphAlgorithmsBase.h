#pragma once

#include <utility>
#include "../algorithms/IGraphAlgorithms.h"

// общая часть всех реализаций: компоненты сильной связности (Тарьян, без рекурсии, O(V + E))
// компоненты не зависят от весов, поэтому работают и с отрицательными
// кратчайшие пути остаются на совести наследников
template <EdgeWeight W>
class GraphAlgorithmsBase : public IGraphAlgorithms<W> {
private:
    // кадр итеративного обхода в глубину, соседи запрашиваются один раз
    struct Frame {
        int v;
        size_t next;
        EdgeList<W> nb;
        Frame() : v(-1), next(0), nb() {}
        Frame(int vertex, EdgeList<W> neighbors) : v(vertex), next(0), nb(std::move(neighbors)) {}
    };

public:
    ResultInfo<SccResult> StrongComponents(const IGraph<W>& graph) const override {
        using R = ResultInfo<SccResult>;
        int n = graph.GetVerticesCount();
        SccResult result;
        result.component = DynamicArray<int>(n, -1);
        if (n == 0) return R::Success(std::move(result));

        DynamicArray<int> index(n, -1);
        DynamicArray<int> lowlink(n, -1);
        DynamicArray<bool> onStack(n, false);
        DynamicArray<int> tStack;
        DynamicArray<Frame> callStack;
        int idx = 0;

        for (int root = 0; root < n; ++root) {
            if (index[root] != -1) continue;

            ResultInfo<EdgeList<W>> rootNb = graph.GetNeighbors(root);
            if (rootNb.IsFailure()) return R::Failure(rootNb.GetError());

            index[root] = lowlink[root] = idx++;
            tStack.PushBack(root);
            onStack[root] = true;
            callStack.PushBack(Frame(root, rootNb.GetValue()));

            while (!callStack.IsEmpty()) {
                Frame& curr = callStack[callStack.GetSize() - 1];
                int v = curr.v;

                if (curr.next < curr.nb.GetSize()) {
                    int w = curr.nb[curr.next]->To();
                    ++curr.next;

                    if (index[w] == -1) {
                        ResultInfo<EdgeList<W>> wNb = graph.GetNeighbors(w);
                        if (wNb.IsFailure()) return R::Failure(wNb.GetError());
                        // идём вглубь, curr после PushBack больше не используется
                        index[w] = lowlink[w] = idx++;
                        tStack.PushBack(w);
                        onStack[w] = true;
                        callStack.PushBack(Frame(w, wNb.GetValue()));
                    } else if (onStack[w] && index[w] < lowlink[v]) {
                        lowlink[v] = index[w];
                    }
                } else {
                    // все соседи обработаны
                    if (lowlink[v] == index[v]) {
                        int w;
                        do {
                            w = tStack.PopBack().GetValue();
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
        return R::Success(std::move(result));
    }
};