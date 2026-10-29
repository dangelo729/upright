// upright — tests/main.cpp
// runs every registered test, or those whose name contains argv[1].
#include <cstring>

#include "up_test.h"

namespace uptest {
Case*& head() { static Case* h = nullptr; return h; }
int& failures() { static int f = 0; return f; }
int& checks() { static int c = 0; return c; }
int& known() { static int k = 0; return k; }
}  // namespace uptest

int main(int argc, char** argv) {
    const char* filter = argc > 1 ? argv[1] : nullptr;
    int ran = 0, failedCases = 0;
    for (uptest::Case* c = uptest::head(); c; c = c->next) {
        if (filter && !std::strstr(c->name, filter)) continue;
        const int before = uptest::failures();
        printf("  %s\n", c->name);
        c->fn();
        ++ran;
        if (uptest::failures() != before) ++failedCases;
    }
    printf("%d tests, %d checks, %d failures, %d known\n", ran, uptest::checks(), uptest::failures(), uptest::known());
    return failedCases ? 1 : 0;
}
