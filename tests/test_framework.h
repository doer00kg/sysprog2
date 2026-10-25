/* Мінімальний тестовий каркас на assert-подібних макросах (без зовнішніх залежностей). */
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <math.h>

static int g_checks;
static int g_fails;

#define CHECK(cond) do { \
    g_checks++; \
    if (!(cond)) { g_fails++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

#define CHECK_EQ(a, b) do { \
    long _a = (long)(a), _b = (long)(b); \
    g_checks++; \
    if (_a != _b) { g_fails++; printf("  FAIL %s:%d: %s == %s (%ld != %ld)\n", \
                                      __FILE__, __LINE__, #a, #b, _a, _b); } \
} while (0)

#define CHECK_NEAR(a, b, tol) do { \
    double _a = (double)(a), _b = (double)(b); \
    g_checks++; \
    if (fabs(_a - _b) > (double)(tol)) { g_fails++; \
        printf("  FAIL %s:%d: %s ~ %s (%f vs %f, tol %f)\n", \
               __FILE__, __LINE__, #a, #b, _a, _b, (double)(tol)); } \
} while (0)

#define RUN(fn) do { printf("%s\n", #fn); fn(); } while (0)

#define TEST_REPORT() (printf("%d checks, %d failed\n", g_checks, g_fails), g_fails ? 1 : 0)

#endif /* TEST_FRAMEWORK_H */
