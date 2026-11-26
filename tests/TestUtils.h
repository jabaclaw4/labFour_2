#pragma once

#include <iostream>

// счётчики общие на весь тестовый бинарник (inline переменные, C++17)
inline int g_testFailures = 0;
inline int g_testChecks = 0;

#define CHECK(cond) \
    do { \
        ++g_testChecks; \
        if (!(cond)) { \
            ++g_testFailures; \
            std::cout << "\n  ошибка " << __FILE__ << ":" << __LINE__ << " " << #cond; \
        } \
    } while (0)

// проверки результатов-монад: исключений в проекте нет, поэтому проверяем ResultInfo
#define CHECK_OK(res) CHECK((res).IsSuccess())
#define CHECK_FAIL(res) CHECK((res).IsFailure())