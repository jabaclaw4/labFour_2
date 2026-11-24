#pragma once

#include <fstream>
#include <string>
#include <cmath>
#include <limits>
#include "Graph.h"

template <typename W>
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
    static bool Generate(const std::string& filename, const IGraph<W>& graph,
                         const DynamicArray<W>& dist, const DynamicArray<int>* path = nullptr) {
        std::ofstream out(filename);
        if (!out.is_open()) return false;

        int n = graph.GetVerticesCount();
        if (n > 50) {
            out << "<html><body><h1>too many vertices to draw</h1></body></html>";
            return true;
        }

        const double PI = 3.14159265358979323846;
        const int width = 800, height = 600;
        const double cx = width / 2.0, cy = height / 2.0;
        const double R = 220, nodeR = 20;

        out << "<!DOCTYPE html>\n<html>\n<head>\n<meta charset='UTF-8'>\n<title>graph</title>\n";
        out << "<style>body{background:#222;color:#fff;text-align:center;font-family:sans-serif;}"
               " svg{background:#333;border-radius:10px;}</style>\n";
        out << "</head>\n<body>\n<h1>graph visualization</h1>\n";
        out << "<svg width='" << width << "' height='" << height << "'>\n";

        out << "<defs>";
        out << "<marker id='arrow' markerWidth='10' markerHeight='10' refX='9' refY='3' orient='auto'>"
               "<path d='M0,0 L0,6 L9,3 z' fill='#aaa' /></marker>";
        out << "<marker id='arrowPath' markerWidth='10' markerHeight='10' refX='9' refY='3' orient='auto'>"
               "<path d='M0,0 L0,6 L9,3 z' fill='#FFC107' /></marker>";
        out << "</defs>\n";

        // рёбра
        for (int i = 0; i < n; ++i) {
            double angI = 2 * PI * i / n;
            double x1 = cx + R * std::cos(angI);
            double y1 = cy + R * std::sin(angI);

            DynamicArray<Edge<W>> neighbors = graph.GetNeighbors(i);
            for (size_t j = 0; j < neighbors.GetSize(); ++j) {
                int to = neighbors[j].to;
                bool onPath = IsPathEdge(path, i, to);
                const char* color = onPath ? "#FFC107" : "#aaa";
                const char* marker = onPath ? "arrowPath" : "arrow";

                if (to == i) {
                    // петля рисуется кружком снаружи вершины
                    double lx = x1 + 28 * std::cos(angI);
                    double ly = y1 + 28 * std::sin(angI);
                    out << "<circle cx='" << lx << "' cy='" << ly << "' r='12' fill='none' stroke='" << color
                        << "' stroke-width='2' />\n";
                    out << "<text x='" << (x1 + 50 * std::cos(angI)) << "' y='" << (y1 + 50 * std::sin(angI) + 4)
                        << "' fill='" << color << "' font-size='13' text-anchor='middle'>"
                        << neighbors[j].weight << "</text>\n";
                    continue;
                }

                double angT = 2 * PI * to / n;
                double x2 = cx + R * std::cos(angT);
                double y2 = cy + R * std::sin(angT);
                double dx = x2 - x1, dy = y2 - y1;
                double len = std::sqrt(dx * dx + dy * dy);
                double ux = dx / len, uy = dy / len;
                // смещение вбок, чтобы встречные рёбра не сливались
                double ox = -uy * 7, oy = ux * 7;

                double sx = x1 + ux * nodeR + ox, sy = y1 + uy * nodeR + oy;
                double ex = x2 - ux * (nodeR + 2) + ox, ey = y2 - uy * (nodeR + 2) + oy;

                out << "<line x1='" << sx << "' y1='" << sy << "' x2='" << ex << "' y2='" << ey
                    << "' stroke='" << color << "' stroke-width='" << (onPath ? 3 : 2)
                    << "' marker-end='url(#" << marker << ")' />\n";

                double mx = (x1 + x2) / 2 + ox * 2.2;
                double my = (y1 + y2) / 2 + oy * 2.2;
                out << "<text x='" << mx << "' y='" << (my + 4) << "' fill='" << color
                    << "' font-size='13' text-anchor='middle'>" << neighbors[j].weight << "</text>\n";
            }
        }

        // вершины
        for (int i = 0; i < n; ++i) {
            double ang = 2 * PI * i / n;
            double x = cx + R * std::cos(ang);
            double y = cy + R * std::sin(ang);

            bool unreachable = (size_t)i < dist.GetSize() && dist[i] == std::numeric_limits<W>::max();
            std::string color = unreachable ? "#f44336" : "#4CAF50";
            if (IsPathVertex(path, i)) color = "#FFC107";

            out << "<circle cx='" << x << "' cy='" << y << "' r='" << nodeR << "' fill='" << color
                << "' stroke='#fff' stroke-width='2' />\n";
            out << "<text x='" << x << "' y='" << (y + 5)
                << "' fill='#000' text-anchor='middle' font-weight='bold'>" << i << "</text>\n";

            if ((size_t)i < dist.GetSize()) {
                out << "<text x='" << x << "' y='" << (y + nodeR + 14)
                    << "' fill='#ddd' font-size='12' text-anchor='middle'>";
                if (unreachable) out << "inf";
                else out << "d=" << dist[i];
                out << "</text>\n";
            }
        }

        out << "</svg>\n";
        out << "<p>зелёный: достижима, красный: недостижима, жёлтый: кратчайший путь</p>\n";
        out << "</body>\n</html>\n";
        return true;
    }
};