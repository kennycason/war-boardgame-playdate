#ifndef WAR_TEST_H
#define WAR_TEST_H

#include <stdio.h>
#include <string.h>

extern int g_test_count;
extern int g_test_failed;
extern int g_section_count;
extern int g_section_failed;
extern const char* g_current_test;

#define TEST_PASS_TAG "\x1b[32mPASS\x1b[0m"
#define TEST_FAIL_TAG "\x1b[31mFAIL\x1b[0m"

#define ASSERT(cond) do {                                                    \
    g_test_count++;                                                          \
    g_section_count++;                                                       \
    if (!(cond)) {                                                           \
        g_test_failed++;                                                     \
        g_section_failed++;                                                  \
        printf("  " TEST_FAIL_TAG " %s:%d  %s   [in %s]\n",                  \
               __FILE__, __LINE__, #cond, g_current_test);                   \
    }                                                                        \
} while(0)

#define ASSERT_EQ(a, b) do {                                                 \
    g_test_count++;                                                          \
    g_section_count++;                                                       \
    long _a = (long)(a), _b = (long)(b);                                     \
    if (_a != _b) {                                                          \
        g_test_failed++;                                                     \
        g_section_failed++;                                                  \
        printf("  " TEST_FAIL_TAG " %s:%d  expected %s==%s (%ld vs %ld)  [in %s]\n", \
               __FILE__, __LINE__, #a, #b, _a, _b, g_current_test);          \
    }                                                                        \
} while(0)

#define ASSERT_TRUE(c)  ASSERT(c)
#define ASSERT_FALSE(c) ASSERT(!(c))

#define RUN(test_fn) do {                                                    \
    g_current_test = #test_fn;                                               \
    int prev_failed = g_test_failed;                                         \
    test_fn();                                                               \
    if (g_test_failed == prev_failed) {                                      \
        printf("  " TEST_PASS_TAG " %s\n", #test_fn);                        \
    }                                                                        \
} while(0)

#define SECTION(label) do {                                                  \
    g_section_count = 0;                                                     \
    g_section_failed = 0;                                                    \
    printf("\n== %s ==\n", label);                                           \
} while(0)

#endif
