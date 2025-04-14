#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "greatest/greatest.h"
#include "threading/threading.h"

#define SKIP_LIST_NAME skip_list_uint32
#define SKIP_LIST_KEY_TYPE uint32_t
#define SKIP_LIST_VALUE_TYPE char *
#include "skip_list.h"
#undef SKIP_LIST_NAME
#undef SKIP_LIST_KEY_TYPE
#undef SKIP_LIST_VALUE_TYPE

TEST test_skip_list(void) {
    skip_list_uint32 *list = skip_list_uint32_new();

    skip_list_uint32_insert(list, 1, "a");
    skip_list_uint32_insert(list, 5, "c");
    skip_list_uint32_insert(list, 3, "b");
    skip_list_uint32_insert(list, 9, "e");
    skip_list_uint32_insert(list, 7, "d");
    skip_list_uint32_insert(list, 11, "f");

    char *a = skip_list_uint32_get(list, 1);
    ASSERT_STR_EQ("a", a);

    char *b = skip_list_uint32_get(list, 3);
    ASSERT_STR_EQ("b", b);

    char *c = skip_list_uint32_get(list, 5);
    ASSERT_STR_EQ("c", c);

    char *d = skip_list_uint32_get(list, 7);
    ASSERT_STR_EQ("d", d);

    char *e = skip_list_uint32_get(list, 9);
    ASSERT_STR_EQ("e", e);

    e = skip_list_uint32_get_next(list, 7);
    ASSERT_STR_EQ("e", e);

    char *f = skip_list_uint32_get(list, 11);
    ASSERT_STR_EQ("f", f);

    char *last = skip_list_uint32_get_next(list, 11);
    ASSERT(last == NULL);

    e = skip_list_uint32_get_prev(list, 11);
    ASSERT_STR_EQ("e", e);

    e = skip_list_uint32_get_prev(list, 10);
    ASSERT_STR_EQ("e", e);

    ASSERT_EQ(skip_list_uint32_size(list), 6);

    a = skip_list_uint32_delete(list, 1);
    ASSERT_STR_EQ(a, "a");
  
    a = skip_list_uint32_get(list, 1);
    ASSERT(a == NULL);
  
    b = skip_list_uint32_delete(list, 3);
    ASSERT_STR_EQ(b, "b");

    ASSERT_EQ(skip_list_uint32_size(list), 4);

    e = skip_list_uint32_delete(list, 9);
    ASSERT_STR_EQ(e, "e");

    ASSERT_EQ(skip_list_uint32_size(list), 3);

    c = skip_list_uint32_get(list, 5);
    ASSERT_STR_EQ(c, "c");

    c = skip_list_uint32_delete(list, 5);
    ASSERT_STR_EQ(c, "c");

    d = skip_list_uint32_delete(list, 7);
    ASSERT_STR_EQ(d, "d");

    d = skip_list_uint32_get(list, 7);
    ASSERT(d == NULL);

    skip_list_uint32_insert(list, 7, "d");

    ASSERT_EQ(skip_list_uint32_size(list), 2);

    d = skip_list_uint32_get(list, 7);
    ASSERT_STR_EQ(d, "d");

    f = skip_list_uint32_get_next(list, 7);
    ASSERT_STR_EQ("f", f);

    f = skip_list_uint32_get_next(list, 8);
    ASSERT_STR_EQ("f", f);

    char *first = skip_list_uint32_get_prev(list, 7);
    ASSERT(first == NULL);

    skip_list_uint32_destroy(list);
    PASS();
}

#define SKIP_LIST_NAME concurrent_skip_list_uint32
#define SKIP_LIST_KEY_TYPE uint32_t
#define SKIP_LIST_VALUE_TYPE char *
#define SKIP_LIST_THREAD_SAFE
#include "skip_list.h"
#undef SKIP_LIST_NAME
#undef SKIP_LIST_KEY_TYPE
#undef SKIP_LIST_VALUE_TYPE
#undef SKIP_LIST_THREAD_SAFE


static char alphabet[26][2] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"};

struct thread_args {
    concurrent_skip_list_uint32 *list;
    uint32_t multiplier;
};

#define NUM_THREADS 8
#define NUM_INSERTS 100000

int test_skip_list_thread(void *arg) {
    struct thread_args *args = (struct thread_args *)arg;
    concurrent_skip_list_uint32 *list = args->list;
    uint32_t multiplier = args->multiplier;
    for (uint32_t i = multiplier; i < NUM_THREADS * NUM_INSERTS; i += NUM_THREADS) {
        uint32_t key = i;
        char *value = alphabet[i % 26];
        concurrent_skip_list_uint32_insert(list, key, value);
        char *fetched = concurrent_skip_list_uint32_get(list, key);
        if (fetched == NULL || strcmp(value, fetched) != 0) {
            fprintf(stderr, "i: %u, value: %s, expected: %s\n", key, value, fetched);
            return 1;
        }
    }
    return 0;
}


TEST test_skip_list_multithreaded(void) {
    concurrent_skip_list_uint32 *list = concurrent_skip_list_uint32_new();
    struct thread_args args[NUM_THREADS];
    thrd_t threads[NUM_THREADS];
    for (uint32_t i = 0; i < NUM_THREADS; i++) {
        args[i].list = list;
        args[i].multiplier = i;
        thrd_create(&threads[i], test_skip_list_thread, &args[i]);
    }
    for (uint32_t i = 0; i < NUM_THREADS; i++) {
        thrd_join(threads[i], NULL);
    }
    size_t size = concurrent_skip_list_uint32_size(list);
    ASSERT_EQ(size, NUM_THREADS * NUM_INSERTS);
    concurrent_skip_list_uint32_destroy(list);
    PASS();
}

/* Add definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line options, initialization. */

    RUN_TEST(test_skip_list);
    RUN_TEST(test_skip_list_multithreaded);

    GREATEST_MAIN_END();        /* display results */
}