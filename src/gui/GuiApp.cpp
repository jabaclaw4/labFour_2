#include "gui/GuiApp.h"

#ifdef _WIN32

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <string>
#include "algorithms/DijkstraAlgorithms.h"
#include "graph/GraphGenerator.h"
#include "visualization/HtmlVisualizer.h"

namespace {

    const int CANVAS_W = 680;  // ширина области рисования, правее лежит панель управления
    const int MAX_DRAW = 40;   // больше вершин не рисуем
    const double PI = 3.14159265358979323846;

    enum ControlId {
        ID_N = 101, ID_CREATE, ID_FROM, ID_TO, ID_WEIGHT, ID_ADD,
        ID_CHAIN, ID_COMPLETE, ID_RANDOM, ID_NEG,
        ID_START, ID_TARGET, ID_HEAP, ID_ARRAY, ID_SCC, ID_HTML, ID_STATUS
    };

// всё состояние окна
    struct AppState {
        Graph<int> graph;
        ShortestPathResult<int> sp;
        bool hasSp = false;
        DynamicArray<int> path;
        SccResult scc;
        bool hasScc = false;
    };

    AppState g_app;
    HWND g_main = nullptr;

    std::wstring Widen(const char* s) {
        std::wstring w;
        while (*s) w.push_back(static_cast<unsigned char>(*s++));
        return w;
    }

    bool ReadInt(int id, int& out) {
        wchar_t buf[32];
        GetWindowTextW(GetDlgItem(g_main, id), buf, 32);
        wchar_t* end = nullptr;
        long v = std::wcstol(buf, &end, 10);
        if (end == buf || *end != L'\0') return false;
        out = static_cast<int>(v);
        return true;
    }

    void SetStatus(const std::wstring& text) {
        SetWindowTextW(GetDlgItem(g_main, ID_STATUS), text.c_str());
    }

    void Redraw() {
        RECT r = {0, 0, CANVAS_W, 10000};
        InvalidateRect(g_main, &r, FALSE);
    }

    void ResetResults() {
        g_app.hasSp = false;
        g_app.hasScc = false;
        g_app.path.Clear();
    }

    bool IsPathEdge(int a, int b) {
        const DynamicArray<int>& p = g_app.path;
        for (size_t k = 0; k + 1 < p.GetSize(); ++k) {
            if (p[k] == a && p[k + 1] == b) return true;
        }
        return false;
    }

    bool IsPathVertex(int v) {
        for (size_t k = 0; k < g_app.path.GetSize(); ++k) {
            if (g_app.path[k] == v) return true;
        }
        return false;
    }

    void Text(HDC dc, int x, int y, const std::wstring& s, COLORREF c) {
        SetTextColor(dc, c);
        TextOutW(dc, x, y, s.c_str(), static_cast<int>(s.size()));
    }

// рисование графа: вершины по кругу, рёбра со стрелками и весами
    void DrawCanvas(HDC dc, int W, int H) {
        HBRUSH bg = CreateSolidBrush(RGB(34, 34, 34));
        RECT full = {0, 0, W, H};
        FillRect(dc, &full, bg);
        DeleteObject(bg);
        SetBkMode(dc, TRANSPARENT);
        SetTextAlign(dc, TA_CENTER | TA_TOP);
        SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT));

        int n = g_app.graph.GetVerticesCount();
        if (n == 0) {
            Text(dc, W / 2, H / 2, L"Создайте граф на панели справа", RGB(200, 200, 200));
            return;
        }
        if (n > MAX_DRAW) {
            Text(dc, W / 2, H / 2,
                 L"Граф слишком большой для рисования (" + std::to_wstring(n) + L" вершин), результаты справа",
                 RGB(200, 200, 200));
            return;
        }

        const double cx = W / 2.0, cy = H / 2.0;
        const double R = std::min(W, H) / 2.0 - 60.0, nodeR = 20.0;
        auto ang = [&](int i) { return 2 * PI * i / n - PI / 2; };
        auto px = [&](int i) { return cx + R * std::cos(ang(i)); };
        auto py = [&](int i) { return cy + R * std::sin(ang(i)); };

        // рёбра
        for (int i = 0; i < n; ++i) {
            ResultInfo<EdgeList<int>> nb = g_app.graph.GetNeighbors(i);
            if (nb.IsFailure()) continue;
            const EdgeList<int>& es = nb.GetValue();
            for (size_t k = 0; k < es.GetSize(); ++k) {
                int to = es[k]->To();
                int weight = es[k]->Weight();
                bool onPath = IsPathEdge(i, to);
                COLORREF col = onPath ? RGB(255, 193, 7) : (weight < 0 ? RGB(239, 83, 80) : RGB(160, 160, 160));

                HPEN pen = CreatePen(PS_SOLID, onPath ? 3 : 1, col);
                HBRUSH br = CreateSolidBrush(col);
                HGDIOBJ oldPen = SelectObject(dc, pen);
                HGDIOBJ oldBr = SelectObject(dc, br);

                double x1 = px(i), y1 = py(i);
                if (to == i) {
                    // петля: кружок снаружи вершины
                    double lx = x1 + 28 * std::cos(ang(i)), ly = y1 + 28 * std::sin(ang(i));
                    SelectObject(dc, GetStockObject(NULL_BRUSH));
                    Ellipse(dc, int(lx - 11), int(ly - 11), int(lx + 11), int(ly + 11));
                    Text(dc, int(x1 + 50 * std::cos(ang(i))), int(y1 + 50 * std::sin(ang(i)) - 7),
                         std::to_wstring(weight), col);
                } else {
                    double x2 = px(to), y2 = py(to);
                    double dx = x2 - x1, dy = y2 - y1;
                    double len = std::sqrt(dx * dx + dy * dy);
                    double ux = dx / len, uy = dy / len;
                    double nx = -uy, ny = ux;
                    double ox = nx * 6, oy = ny * 6; // сдвиг вбок, чтобы встречные рёбра не сливались

                    double sx = x1 + ux * nodeR + ox, sy = y1 + uy * nodeR + oy;
                    double ex = x2 - ux * nodeR + ox, ey = y2 - uy * nodeR + oy;
                    MoveToEx(dc, int(sx), int(sy), nullptr);
                    LineTo(dc, int(ex), int(ey));

                    POINT head[3] = {
                            {int(ex), int(ey)},
                            {int(ex - ux * 11 + nx * 5), int(ey - uy * 11 + ny * 5)},
                            {int(ex - ux * 11 - nx * 5), int(ey - uy * 11 - ny * 5)}};
                    Polygon(dc, head, 3);

                    Text(dc, int((x1 + x2) / 2 + ox * 2.2), int((y1 + y2) / 2 + oy * 2.2 - 7),
                         std::to_wstring(weight), col);
                }
                SelectObject(dc, oldPen);
                SelectObject(dc, oldBr);
                DeleteObject(pen);
                DeleteObject(br);
            }
        }

        // вершины
        static const COLORREF palette[8] = {RGB(100, 181, 246), RGB(129, 199, 132), RGB(255, 138, 101),
                                            RGB(186, 104, 200), RGB(77, 208, 225), RGB(240, 98, 146),
                                            RGB(220, 231, 117), RGB(161, 136, 127)};
        for (int i = 0; i < n; ++i) {
            COLORREF col = RGB(100, 149, 237);
            if (g_app.hasScc) col = palette[g_app.scc.component[i] % 8];
            else if (g_app.hasSp) col = g_app.sp.IsReachable(i) ? RGB(76, 175, 80) : RGB(244, 67, 54);
            if (IsPathVertex(i)) col = RGB(255, 193, 7);

            HPEN pen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
            HBRUSH br = CreateSolidBrush(col);
            HGDIOBJ oldPen = SelectObject(dc, pen);
            HGDIOBJ oldBr = SelectObject(dc, br);
            int x = int(px(i)), y = int(py(i));
            Ellipse(dc, int(x - nodeR), int(y - nodeR), int(x + nodeR), int(y + nodeR));
            SelectObject(dc, oldPen);
            SelectObject(dc, oldBr);
            DeleteObject(pen);
            DeleteObject(br);

            Text(dc, x, y - 7, std::to_wstring(i), RGB(0, 0, 0));
            if (g_app.hasSp) {
                std::wstring d = g_app.sp.IsReachable(i) ? L"d=" + std::to_wstring(g_app.sp.dist[i]) : L"inf";
                Text(dc, x, y + int(nodeR) + 2, d, RGB(220, 220, 220));
            }
            if (g_app.hasScc) {
                Text(dc, x, y + int(nodeR) + 2, L"к" + std::to_wstring(g_app.scc.component[i]), RGB(220, 220, 220));
            }
        }

        Text(dc, W / 2, H - 22,
             L"красное ребро: отрицательный вес, жёлтое: кратчайший путь", RGB(160, 160, 160));
    }

    void AddControl(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style,
                    int x, int y, int w, int h, int id) {
        HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h, parent,
                                 reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
        SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
    }

    void BuildControls(HWND h) {
        const int x0 = CANVAS_W + 15;
        const DWORD edit = WS_BORDER | ES_AUTOHSCROLL;
        const DWORD btn = BS_PUSHBUTTON;

        AddControl(h, L"STATIC", L"Вершин:", 0, x0, 14, 60, 20, 0);
        AddControl(h, L"EDIT", L"6", edit, x0 + 65, 10, 50, 24, ID_N);
        AddControl(h, L"BUTTON", L"Создать граф", btn, x0 + 125, 9, 190, 26, ID_CREATE);

        AddControl(h, L"STATIC", L"Ребро: откуда / куда / вес (можно < 0)", 0, x0, 48, 320, 18, 0);
        AddControl(h, L"EDIT", L"0", edit, x0, 68, 60, 24, ID_FROM);
        AddControl(h, L"EDIT", L"1", edit, x0 + 70, 68, 60, 24, ID_TO);
        AddControl(h, L"EDIT", L"1", edit, x0 + 140, 68, 60, 24, ID_WEIGHT);
        AddControl(h, L"BUTTON", L"Добавить ребро", btn, x0 + 210, 67, 105, 26, ID_ADD);

        AddControl(h, L"BUTTON", L"Цепь", btn, x0, 105, 100, 26, ID_CHAIN);
        AddControl(h, L"BUTTON", L"Полный", btn, x0 + 105, 105, 100, 26, ID_COMPLETE);
        AddControl(h, L"BUTTON", L"Случайный", btn, x0 + 210, 105, 105, 26, ID_RANDOM);
        AddControl(h, L"BUTTON", L"случайные веса бывают отрицательными", BS_AUTOCHECKBOX, x0, 135, 320, 20, ID_NEG);

        AddControl(h, L"STATIC", L"Старт / цель (цель можно не вводить)", 0, x0, 168, 320, 18, 0);
        AddControl(h, L"EDIT", L"0", edit, x0, 188, 60, 24, ID_START);
        AddControl(h, L"EDIT", L"", edit, x0 + 70, 188, 60, 24, ID_TARGET);
        AddControl(h, L"BUTTON", L"Дейкстра (куча)", btn, x0, 222, 155, 26, ID_HEAP);
        AddControl(h, L"BUTTON", L"Дейкстра (массив)", btn, x0 + 160, 222, 155, 26, ID_ARRAY);
        AddControl(h, L"BUTTON", L"Компоненты (Тарьян)", btn, x0, 254, 155, 26, ID_SCC);
        AddControl(h, L"BUTTON", L"Сохранить graph.html", btn, x0 + 160, 254, 155, 26, ID_HTML);

        AddControl(h, L"EDIT", L"", WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                   x0, 292, 320, 330, ID_STATUS);
    }

    void RunShortestPaths(const IGraphAlgorithms<int>& alg) {
        int start, target = -1;
        if (!ReadInt(ID_START, start)) { SetStatus(L"Введите стартовую вершину"); return; }
        bool hasTarget = ReadInt(ID_TARGET, target);

        ResetResults();
        ResultInfo<ShortestPathResult<int>> r = alg.ShortestPaths(g_app.graph, start);
        if (r.IsFailure()) {
            SetStatus(L"Ошибка: " + Widen(r.GetError()));
            Redraw();
            return;
        }
        g_app.sp = r.GetValue();
        g_app.hasSp = true;

        std::wstring s = Widen(alg.Name()) + L"\r\nрасстояния от " + std::to_wstring(start) + L":\r\n";
        int n = g_app.graph.GetVerticesCount();
        for (int v = 0; v < n && v < 60; ++v) {
            s += L"  до " + std::to_wstring(v) + L": ";
            s += g_app.sp.IsReachable(v) ? std::to_wstring(g_app.sp.dist[v]) : std::wstring(L"недостижима");
            s += L"\r\n";
        }
        if (hasTarget) {
            ResultInfo<DynamicArray<int>> p = g_app.sp.PathTo(target);
            if (p.IsSuccess()) {
                g_app.path = p.GetValue();
                s += L"путь:";
                for (size_t i = 0; i < g_app.path.GetSize(); ++i) s += L" " + std::to_wstring(g_app.path[i]);
                s += L"\r\n";
            } else {
                s += L"пути нет: " + Widen(p.GetError()) + L"\r\n";
            }
        }
        SetStatus(s);
        Redraw();
    }

    void RunComponents() {
        HeapGraphAlgorithms<int> alg; // Тарьян лежит в базовом классе, подойдёт любая реализация
        ResetResults();
        ResultInfo<SccResult> r = alg.StrongComponents(g_app.graph);
        if (r.IsFailure()) { SetStatus(L"Ошибка: " + Widen(r.GetError())); Redraw(); return; }
        g_app.scc = r.GetValue();
        g_app.hasScc = true;

        std::wstring s = L"компонент сильной связности: " + std::to_wstring(g_app.scc.count) + L"\r\n";
        int n = g_app.graph.GetVerticesCount();
        if (n <= 60) {
            for (int c = 0; c < g_app.scc.count; ++c) {
                s += L"  к" + std::to_wstring(c) + L":";
                for (int v = 0; v < n; ++v) {
                    if (g_app.scc.component[v] == c) s += L" " + std::to_wstring(v);
                }
                s += L"\r\n";
            }
        }
        SetStatus(s);
        Redraw();
    }

    void OnCommand(int id) {
        int n = g_app.graph.GetVerticesCount();
        switch (id) {
            case ID_CREATE: {
                int count;
                if (!ReadInt(ID_N, count) || count < 1 || count > 2000) {
                    SetStatus(L"Число вершин: целое от 1 до 2000");
                    return;
                }
                ResultInfo<Graph<int>> r = Graph<int>::Create(count);
                if (r.IsFailure()) { SetStatus(L"Ошибка: " + Widen(r.GetError())); return; }
                g_app.graph = r.GetValue();
                ResetResults();
                SetStatus(L"Граф создан, вершин: " + std::to_wstring(count));
                Redraw();
                break;
            }
            case ID_ADD: {
                int from, to, w;
                if (!ReadInt(ID_FROM, from) || !ReadInt(ID_TO, to) || !ReadInt(ID_WEIGHT, w)) {
                    SetStatus(L"Введите целые числа откуда / куда / вес");
                    return;
                }
                ResultInfo<bool> r = g_app.graph.AddEdge(from, to, w);
                ResetResults();
                if (r.IsFailure()) SetStatus(L"Ошибка: " + Widen(r.GetError()));
                else SetStatus(r.GetValue() ? L"Ребро добавлено" : L"Ребро уже было, вес заменён");
                Redraw();
                break;
            }
            case ID_CHAIN:
            case ID_COMPLETE:
            case ID_RANDOM: {
                if (n == 0) { SetStatus(L"Сначала создайте граф"); return; }
                ResultInfo<bool> r = ResultInfo<bool>::Success(true);
                if (id == ID_CHAIN) r = GraphGenerator<int>::MakeChain(g_app.graph);
                else if (id == ID_COMPLETE) r = GraphGenerator<int>::MakeComplete(g_app.graph);
                else {
                    bool neg = SendMessageW(GetDlgItem(g_main, ID_NEG), BM_GETCHECK, 0, 0) == BST_CHECKED;
                    r = GraphGenerator<int>::MakeRandom(g_app.graph, n * 2, GetTickCount(), neg ? -5 : 1, 10);
                }
                ResetResults();
                SetStatus(r.IsSuccess() ? L"Рёбер в графе: " + std::to_wstring(g_app.graph.GetEdgesCount())
                                        : L"Ошибка: " + Widen(r.GetError()));
                Redraw();
                break;
            }
            case ID_HEAP: { HeapGraphAlgorithms<int> a; RunShortestPaths(a); break; }
            case ID_ARRAY: { ArrayGraphAlgorithms<int> a; RunShortestPaths(a); break; }
            case ID_SCC: RunComponents(); break;
            case ID_HTML: {
                ResultInfo<bool> r = HtmlVisualizer<int>::Generate(
                        "graph.html", g_app.graph, g_app.hasSp ? &g_app.sp : nullptr,
                        g_app.path.IsEmpty() ? nullptr : &g_app.path);
                SetStatus(r.IsSuccess() ? L"graph.html создан" : L"Ошибка: " + Widen(r.GetError()));
                break;
            }
        }
    }

    LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
        switch (m) {
            case WM_CREATE:
                g_main = h;
                BuildControls(h);
                return 0;
            case WM_COMMAND:
                if (HIWORD(w) == BN_CLICKED) OnCommand(LOWORD(w));
                return 0;
            case WM_ERASEBKGND: {
                RECT c;
                GetClientRect(h, &c);
                RECT panel = {CANVAS_W, 0, c.right, c.bottom};
                FillRect(reinterpret_cast<HDC>(w), &panel, reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1));
                return 1;
            }
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC dc = BeginPaint(h, &ps);
                RECT c;
                GetClientRect(h, &c);
                int H = c.bottom;
                // двойная буферизация, чтобы не мигало
                HDC mem = CreateCompatibleDC(dc);
                HBITMAP bmp = CreateCompatibleBitmap(dc, CANVAS_W, H);
                HGDIOBJ old = SelectObject(mem, bmp);
                DrawCanvas(mem, CANVAS_W, H);
                BitBlt(dc, 0, 0, CANVAS_W, H, mem, 0, 0, SRCCOPY);
                SelectObject(mem, old);
                DeleteObject(bmp);
                DeleteDC(mem);
                EndPaint(h, &ps);
                return 0;
            }
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProcW(h, m, w, l);
    }

} // namespace

int RunGui() {
    HINSTANCE inst = GetModuleHandleW(nullptr);
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.lpszClassName = L"GraphLabWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc); // при повторном запуске вернёт ошибку, это не мешает

    HWND wnd = CreateWindowExW(0, L"GraphLabWindow", L"GraphLab: кратчайшие пути и компоненты",
                               WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                               CANVAS_W + 380, 700, nullptr, nullptr, inst, nullptr);
    if (!wnd) return 1;

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}

#else

#include <iostream>

int RunGui() {
    std::cout << "GUI реализован только для Windows\n";
    return 1;
}

#endif