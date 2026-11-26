#include <iostream>
#include "TestUtils.h"

// наборы тестов из подпапок regular/ и lazy/
void RunContainersTests();
void RunSparseMatrixTests();
void RunGraphStructureTests();
void RunDijkstraTests();
void RunSccTests();
void RunLazyGraphStructureTests();
void RunLazyAlgorithmsTests();

namespace {
    void Section(const char* name, void (*suite)()) {
        int before = g_testFailures;
        std::cout << name << " ... ";
        suite();
        std::cout << (g_testFailures == before ? "ok" : "\nпровал") << "\n";
    }
}

// возвращает число проваленных проверок
int RunAllTests() {
    g_testFailures = 0;
    g_testChecks = 0;

    std::cout << "обычный граф\n";
    Section("  контейнеры и монада", RunContainersTests);
    Section("  разреженная матрица", RunSparseMatrixTests);
    Section("  структура графа", RunGraphStructureTests);
    Section("  дейкстра", RunDijkstraTests);
    Section("  компоненты связности", RunSccTests);
    std::cout << "ленивый граф\n";
    Section("  структура графа", RunLazyGraphStructureTests);
    Section("  алгоритмы", RunLazyAlgorithmsTests);

    std::cout << "проверок: " << g_testChecks << ", провалено: " << g_testFailures << "\n";
    return g_testFailures;
}