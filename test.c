#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "greatest/greatest.h"

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

    char *a = skip_list_uint32_search(list, 1);
    ASSERT_STR_EQ("a", a);

    char *b = skip_list_uint32_search(list, 3);
    ASSERT_STR_EQ("b", b);

    char *c = skip_list_uint32_search(list, 5);
    ASSERT_STR_EQ("c", c);

    char *d = skip_list_uint32_search(list, 7);
    ASSERT_STR_EQ("d", d);

    char *e = skip_list_uint32_search(list, 9);
    ASSERT_STR_EQ("e", e);

    e = skip_list_uint32_search_after(list, 7);
    ASSERT_STR_EQ("e", e);

    char *f = skip_list_uint32_search(list, 11);
    ASSERT_STR_EQ("f", f);

    char *last = skip_list_uint32_search_after(list, 11);
    ASSERT(last == NULL);

    e = skip_list_uint32_search_before(list, 11);
    ASSERT_STR_EQ("e", e);

    e = skip_list_uint32_search_before(list, 10);
    ASSERT_STR_EQ("e", e);

    a = skip_list_uint32_delete(list, 1);
    ASSERT_STR_EQ(a, "a");
  
    a = skip_list_uint32_search(list, 1);
    ASSERT(a == NULL);
  
    b = skip_list_uint32_delete(list, 3);
    ASSERT_STR_EQ(b, "b");

    e = skip_list_uint32_delete(list, 9);
    ASSERT_STR_EQ(e, "e");

    c = skip_list_uint32_search(list, 5);
    ASSERT_STR_EQ(c, "c");

    c = skip_list_uint32_delete(list, 5);
    ASSERT_STR_EQ(c, "c");

    d = skip_list_uint32_delete(list, 7);
    ASSERT_STR_EQ(d, "d");

    d = skip_list_uint32_search(list, 7);
    ASSERT(d == NULL);

    skip_list_uint32_insert(list, 7, "d");
    d = skip_list_uint32_search(list, 7);
    ASSERT_STR_EQ(d, "d");

    f = skip_list_uint32_search_after(list, 7);
    ASSERT_STR_EQ("f", f);

    f = skip_list_uint32_search_after(list, 8);
    ASSERT_STR_EQ("f", f);

    char *first = skip_list_uint32_search_before(list, 7);
    ASSERT(first == NULL);

    skip_list_uint32_destroy(list);
    PASS();
}



/* Add definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line options, initialization. */

    srand(0);
    RUN_TEST(test_skip_list);

    GREATEST_MAIN_END();        /* display results */
}