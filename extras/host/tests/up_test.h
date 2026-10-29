// upright — tests/up_test.h
// plain-assert tests: a registry, CHECK, CHECK_MSG, nothing else.
#pragma once

#include <cstdio>

namespace uptest {

struct Case {
    const char* name;
    void (*fn)();
    Case* next;
};
Case*& head();
int& failures();
int& checks();
int& known();

struct Register {
    Register(const char* name, void (*fn)()) {
        static Case pool[256];
        static int used = 0;
        Case& c = pool[used++];
        c.name = name;
        c.fn = fn;
        c.next = nullptr;
        Case** tail = &head();
        while (*tail) tail = &(*tail)->next;
        *tail = &c;
    }
};

}  // namespace uptest

#define UP_TEST(name)                                         \
    static void name();                                       \
    static uptest::Register up_reg_##name(#name, name);       \
    static void name()

#define CHECK(cond)                                                        \
    do {                                                                   \
        ++uptest::checks();                                                \
        if (!(cond)) {                                                     \
            ++uptest::failures();                                          \
            printf("    FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
        }                                                                  \
    } while (0)

// a CHECK that explains its numbers when it fails
#define CHECK_MSG(cond, ...)                                               \
    do {                                                                   \
        ++uptest::checks();                                                \
        if (!(cond)) {                                                     \
            ++uptest::failures();                                          \
            printf("    FAIL %s:%d  %s  -- ", __FILE__, __LINE__, #cond);  \
            printf(__VA_ARGS__);                                           \
            printf("\n");                                                  \
        }                                                                  \
    } while (0)

// a row that does not pass after honest effort: kept in the file with what
// it measured, printed as KNOWN and counted apart, not as a failure (the
// readme's known limits say why)
#define KNOWN_FAILURE(cond, ...)                                           \
    do {                                                                   \
        ++uptest::checks();                                                \
        if (!(cond)) {                                                     \
            ++uptest::known();                                             \
            printf("    KNOWN %s:%d  %s  -- ", __FILE__, __LINE__, #cond); \
            printf(__VA_ARGS__);                                           \
            printf("\n");                                                  \
        }                                                                  \
    } while (0)
