#pragma once

#include <cmath>
#include <fstream>
#include <string>
#include "algorithms/AlgorithmTypes.h"

// статическая html-картинка графа (дополнение к окну GUI)
template <EdgeWeight W>
class HtmlVisualizer {
private:
    static bool IsPathVertex(const DynamicArray<int>* path, int v) {
        if (!path) return false;
        for (size_t k = 0; k < path->GetSize(); ++k) {
            if ((*path)[k] == v) return true;
        }
        return false;
    }

    static bool IsPathEdge(const DynamicArray<int>* path, int a, int b) {
        if (!path) return false;
        for (size_t k = 0; k + 1 < path->GetSize(); ++k) {
            if ((*path)[k] == a && (*path)[k + 1] == b) return true;
        }
        return false;
    }

public:
    // sp и path необязательны: без них рисуется просто граф
    static ResultInfo<bool> Generate(const std::string& filename, const IGraph<W>& graph,
                                     const ShortestPathResult<W>* sp = nullptr,
                                     const DynamicArray<int>* path = nullptr) {
        std::ofstream out(filename);
        if (!out.is_open()) return ResultInfo<bool>::Failure("cannot open output file");

        int n = graph.GetVerticesCount();
        if (n > 50) {
            out << "<html><body><h1>too many vertices to draw</h1></body></html>";
            return ResultInfo<bool>::Success(true);
        }

        const double PI = 3.14159265358979323846;
        const int width = 800, height = 600;
        const double cx = width / 2.0, cy = height / 2.0, R = 220, nodeR = 20;

        out << "<!DOCTYPE html>\n<html>\n<head>\n<meta charset='UTF-8'>\n<title>graph</title>\n"
            << "<style>body{background:#222;color:#fff;text-align:center;font-family:sans-serif;}"
               " svg{background:#333;border-radius:10px;}</style>\n</head>\n<body>\n"
            << "<h1>graph visualization</h1>\n<svg width='" << width << "' height='" << height << "'>\n";
        out << "<defs><marker id='arrow' markerWidth='10' markerHeight='10' refX='9' refY='3' orient='auto'>"
               "<path d='M0,0 L0,6 L9,3 z' fill='#aaa' /></marker>"
               "<marker id='arrowPath' markerWidth='10' markerHeight='10' refX='9' refY='3' orient='auto'>"
               "<path d='M0,0 L0,6 L9,3 z' fill='#FFC107' /></marker></defs>\n";

        for (int i = 0; i < n; ++i) {
            double angI = 2 * PI * i / n;
            double x1 = cx + R * std::cos(angI), y1 = cy + R * std::sin(angI);

            ResultInfo<EdgeList<W>> nb = graph.GetNeighbors(i);
            if (nb.IsFailure()) return ResultInfo<bool>::Failure(nb.GetError());
            const EdgeList<W>& edges = nb.GetValue();

            for (size_t j = 0; j < edges.GetSize(); ++j) {
                int to = edges[j]->To();
                W weight = edges[j]->Weight();
                bool onPath = IsPathEdge(path, i, to);
                const char* color = onPath ? "#FFC107" : (weight < W(0) ? "#ef5350" : "#aaa");
                const char* marker = onPath ? "arrowPath" : "arrow";

                if (to == i) {
                    double lx = x1 + 28 * std::cos(angI), ly = y1 + 28 * std::sin(angI);
                    out << "<circle cx='" << lx << "' cy='" << ly << "' r='12' fill='none' stroke='" << color
                        << "' stroke-width='2' />\n<text x='" << (x1 + 50 * std::cos(angI)) << "' y='"
                        << (y1 + 50 * std::sin(angI) + 4) << "' fill='" << color
                        << "' font-size='13' text-anchor='middle'>" << weight << "</text>\n";
                    continue;
                }

                double angT = 2 * PI * to / n;
                double x2 = cx + R * std::cos(angT), y2 = cy + R * std::sin(angT);
                double dx = x2 - x1, dy = y2 - y1;
                double len = std::sqrt(dx * dx + dy * dy);
                double ux = dx / len, uy = dy / len;
                double ox = -uy * 7, oy = ux * 7;

                out << "<line x1='" << (x1 + ux * nodeR + ox) << "' y1='" << (y1 + uy * nodeR + oy)
                    << "' x2='" << (x2 - ux * (nodeR + 2) + ox) << "' y2='" << (y2 - uy * (nodeR + 2) + oy)
                    << "' stroke='" << color << "' stroke-width='" << (onPath ? 3 : 2)
                    << "' marker-end='url(#" << marker << ")' />\n";
                out << "<text x='" << ((x1 + x2) / 2 + ox * 2.2) << "' y='" << ((y1 + y2) / 2 + oy * 2.2 + 4)
                    << "' fill='" << color << "' font-size='13' text-anchor='middle'>" << weight << "</text>\n";
            }
        }

        for (int i = 0; i < n; ++i) {
            double ang = 2 * PI * i / n;
            double x = cx + R * std::cos(ang), y = cy + R * std::sin(ang);

            bool known = sp != nullptr && static_cast<size_t>(i) < sp->dist.GetSize();
            bool unreachable = known && !sp->IsReachable(i);
            std::string color = unreachable ? "#f44336" : "#4CAF50";
            if (IsPathVertex(path, i)) color = "#FFC107";

            out << "<circle cx='" << x << "' cy='" << y << "' r='" << nodeR << "' fill='" << color
                << "' stroke='#fff' stroke-width='2' />\n<text x='" << x << "' y='" << (y + 5)
                << "' fill='#000' text-anchor='middle' font-weight='bold'>" << i << "</text>\n";
            if (known) {
                out << "<text x='" << x << "' y='" << (y + nodeR + 14)
                    << "' fill='#ddd' font-size='12' text-anchor='middle'>";
                if (unreachable) out << "inf";
                else out << "d=" << sp->dist[i];
                out << "</text>\n";
            }
        }

        out << "</svg>\n<p>зелёный: достижима, красный: недостижима, жёлтый: кратчайший путь, "
               "красное ребро: отрицательный вес</p>\n</body>\n</html>\n";
        return ResultInfo<bool>::Success(true);
    }
};