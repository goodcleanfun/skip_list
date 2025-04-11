#ifndef SKIP_LIST_H
#define SKIP_LIST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "bit_utils/bit_utils.h"

#if ((UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFu))
#define SKIP_LIST_MAX_LEVEL 64
// (1 << 64) - 1
#define SKIP_LIST_MAX_LEVEL_MASK UINT64_MAX
#include "random/rand64.h"
#elif ((UINTPTR_MAX == 0xFFFFFFFF))
#define SKIP_LIST_MAX_LEVEL 32
// (1 << 32) - 1
#define SKIP_LIST_MAX_LEVEL_MASK UINT32_MAX
#include "random/rand32.h"
#else
#error "Unknown pointer size"
#endif

#if SKIP_LIST_MAX_LEVEL == 64
static inline size_t skip_list_random_level(pcg64_random_t *rng) {
    return (size_t)(1 + clz(rand64_gen_random(rng) & SKIP_LIST_MAX_LEVEL_MASK));
}
#elif SKIP_LIST_MAX_LEVEL == 32
static inline size_t skip_list_random_level(pcg32_random_t *rng) {
    return (size_t)(1 + clz(rand32_gen_random(rng) & SKIP_LIST_MAX_LEVEL_MASK));
}
#endif

#endif // SKIP_LIST_H

#ifndef SKIP_LIST_NAME
#error "Must define SKIP_LIST_NAME"
#endif

#ifndef SKIP_LIST_KEY_TYPE
#error "Must define SKIP_LIST_TYPE"
#endif

#ifndef SKIP_LIST_VALUE_TYPE
#error "Must define SKIP_LIST_VALUE_TYPE"
#endif

#ifdef SKIP_LIST_THREAD_SAFE
#include <stdatomic.h>
#endif

#define SKIP_LIST_CONCAT_(a, b) a ## b
#define SKIP_LIST_CONCAT(a, b) SKIP_LIST_CONCAT_(a, b)
#define SKIP_LIST_TYPED(name) SKIP_LIST_CONCAT(SKIP_LIST_NAME, _##name)
#define SKIP_LIST_FUNC(func) SKIP_LIST_CONCAT(SKIP_LIST_NAME, _##func)

#ifndef SKIP_LIST_KEY_LESS_THAN
#define SKIP_LIST_KEY_LESS_THAN(key, node_key) ((key) < (node_key))
#endif

#ifndef SKIP_LIST_KEY_EQUALS
#define SKIP_LIST_KEY_EQUALS(key, node_key) ((key) == (node_key))
#endif

typedef struct SKIP_LIST_TYPED(node) {
    SKIP_LIST_KEY_TYPE key;
    #ifdef SKIP_LIST_THREAD_SAFE
    _Atomic(struct SKIP_LIST_TYPED(node) *) next;
    #else
    struct SKIP_LIST_TYPED(node) *next;
    #endif
    struct SKIP_LIST_TYPED(node) *down;
} SKIP_LIST_TYPED(node_t);

#ifdef SKIP_LIST_THREAD_SAFE
#if SKIP_LIST_MAX_LEVEL == 64
#define SKIP_LIST_MAX_LEVEL_BITS 6
#elif SKIP_LIST_MAX_LEVEL == 32
#define SKIP_LIST_MAX_LEVEL_BITS 5
#else
#error "Could not determine pointer size"
#endif

#define SKIP_LIST_VERSION_BITS 8 * sizeof(size_t) - SKIP_LIST_MAX_LEVEL_BITS
#define SKIP_LIST_DELETED ((void *)-1)

#define SKIP_LIST_HEAD SKIP_LIST_TYPED(head)
/* A pointer to the head node, the max level, and the version
 * can be stored in a two words. On most systems, where DWCAS (double word compare and swap)
 * is supported, the entire struct can be atomically updated, preventing the ABA problem
 * and allowing the max_level to be updated at the same time as the head node pointer.
 */
typedef struct SKIP_LIST_HEAD {
    SKIP_LIST_NODE *node;
    size_t max_level:SKIP_LIST_MAX_LEVEL_BITS;
    size_t version:SKIP_LIST_VERSION_BITS;
} SKIP_LIST_HEAD;


#undef SKIP_LIST_MAX_LEVEL_BITS
#undef SKIP_LIST_VERSION_BITS
#endif

#define SKIP_LIST_NODE SKIP_LIST_TYPED(node_t)


#define SKIP_LIST_NODE_MEMORY_POOL_NAME SKIP_LIST_TYPED(node_memory_pool)

#define MEMORY_POOL_NAME SKIP_LIST_NODE_MEMORY_POOL_NAME
#define MEMORY_POOL_TYPE SKIP_LIST_NODE
#ifdef SKIP_LIST_THREAD_SAFE
#define MEMORY_POOL_THREAD_SAFE
#endif
#include "memory_pool/memory_pool.h"
#undef MEMORY_POOL_NAME
#undef MEMORY_POOL_TYPE
#ifdef SKIP_LIST_THREAD_SAFE
#undef MEMORY_POOL_THREAD_SAFE
#endif

#define SKIP_LIST_NODE_MEMORY_POOL_FUNC(name) SKIP_LIST_CONCAT(SKIP_LIST_NODE_MEMORY_POOL_NAME, _##name)

typedef struct {
    #ifdef SKIP_LIST_THREAD_SAFE
    _Atomic(SKIP_LIST_HEAD) head;
    tss_t random;
    atomic_size_t size;
    #else
    SKIP_LIST_NODE *head;
    size_t max_level;
    #if SKIP_LIST_MAX_LEVEL == 64
    pcg64_random_t random;
    #elif SKIP_LIST_MAX_LEVEL == 32
    pcg32_random_t random;
    #endif
    size_t size;
    #endif
    SKIP_LIST_NODE_MEMORY_POOL_NAME *pool;
} SKIP_LIST_NAME;

SKIP_LIST_NAME *SKIP_LIST_FUNC(new_pool)(SKIP_LIST_NODE_MEMORY_POOL_NAME *pool) {
    if (pool == NULL) return NULL;
    SKIP_LIST_NAME *list = calloc(1, sizeof(SKIP_LIST_NAME));
    if (list == NULL) return NULL;
    list->pool = pool;

    #ifdef SKIP_LIST_THREAD_SAFE
    SKIP_LIST_NODE *head_node = SKIP_LIST_NODE_MEMORY_POOL_FUNC(get)(list->pool);
    if (head_node == NULL) {
        SKIP_LIST_NODE_MEMORY_POOL_FUNC(destroy)(list->pool);
        free(list);
        return NULL;
    }
    head_node->next = NULL;
    head_node->down = NULL;
    head_node->key = 0;

    SKIP_LIST_HEAD head = (SKIP_LIST_HEAD){
        .node = head_node,
        .max_level = 0,
        .version = 0
    };
    atomic_init(&list->head, head);
    if (thrd_success != tss_create(&list->random, free)) {
        SKIP_LIST_NODE_MEMORY_POOL_FUNC(destroy)(list->pool);
        free(list);
        return NULL;
    }
    atomic_init(&list->size, 0);
    #else
    SKIP_LIST_NODE *head = SKIP_LIST_NODE_MEMORY_POOL_FUNC(get)(list->pool);
    if (head == NULL) {
        SKIP_LIST_NODE_MEMORY_POOL_FUNC(destroy)(list->pool);
        free(list);
        return NULL;
    }
    head->next = NULL;
    head->down = NULL;
    list->head = head;
    #if SKIP_LIST_MAX_LEVEL == 64
    list->random = rand64_gen_init();
    rand64_gen_seed(&list->random, 123422334, 1);//_os(&list->random);
    #elif SKIP_LIST_MAX_LEVEL == 32
    list->random = rand32_gen_init();
    rand32_gen_seed_os(&list->random);
    #endif
    list->size = 0;
    list->max_level = 0;
    #endif
    return list;
}

#ifdef SKIP_LIST_THREAD_SAFE
#if SKIP_LIST_MAX_LEVEL == 64
static inline rand64_gen_t *SKIP_LIST_FUNC(get_thread_random)(SKIP_LIST_NAME *list) {
    rand64_gen_t *rng = tss_get(list->random);
    if (rng == NULL) {
        rng = malloc(sizeof(rand64_gen_t));
        *rng = rand64_gen_init();
        //rand64_gen_seed_os(rng);
        rand64_gen_seed(rng, 123422334, 1);
        tss_set(list->random, rng);
    }
    return rng;
}
#elif SKIP_LIST_MAX_LEVEL == 32
static inline rand32_gen_t *SKIP_LIST_FUNC(get_thread_random)(SKIP_LIST_NAME *list) {
    rand32_gen_t *rng = tss_get(list->random);
    if (rng == NULL) {
        rng = malloc(sizeof(rand32_gen_t));
        *rng = rand32_gen_init();
        //rand32_gen_seed_os(rng);
        rand32_gen_seed(rng, 123422334, 1);
        tss_set(list->random, rng);
    }
    return rng;
}
#endif
#endif

SKIP_LIST_NAME *SKIP_LIST_FUNC(new)(void) {
    SKIP_LIST_NODE_MEMORY_POOL_NAME *pool = SKIP_LIST_NODE_MEMORY_POOL_FUNC(new)();
    if (pool == NULL) {
        return NULL;
    }
    SKIP_LIST_NAME *list = SKIP_LIST_FUNC(new_pool)(pool);
    if (list == NULL) {
        SKIP_LIST_NODE_MEMORY_POOL_FUNC(destroy)(pool);
        return NULL;
    }
    return list;
}


#ifdef SKIP_LIST_THREAD_SAFE

size_t SKIP_LIST_FUNC(size)(SKIP_LIST_NAME *list) {
    if (list == NULL) return 0;
    return atomic_load(&list->size);
}

void *SKIP_LIST_FUNC(get)(SKIP_LIST_NAME *list, SKIP_LIST_KEY_TYPE key) {
    if (list == NULL) return NULL;
    SKIP_LIST_HEAD head = atomic_load(&list->head);
    if (head.node == NULL || head.node->next == NULL) return NULL;
    bool beyond_placeholder = false;
    SKIP_LIST_NODE *current = head.node;
    SKIP_LIST_NODE *next_node = NULL;
    if (!current) return NULL;
    while (current && current->down != NULL && current->down != SKIP_LIST_DELETED) {
        while ((next_node = atomic_load(&current->next)) != NULL && (
                SKIP_LIST_KEY_LESS_THAN(next_node->key, key) || SKIP_LIST_KEY_EQUALS(next_node->key, key))) {
            current = next_node;
            beyond_placeholder = true;
        }
        current = current->down;
    }
    if (beyond_placeholder && current->down != SKIP_LIST_DELETED && SKIP_LIST_KEY_EQUALS(current->key, key)) {
        void *value = (void *)atomic_load(&current->next);
        return value;
    }
    return NULL;
}

bool SKIP_LIST_FUNC(insert)(SKIP_LIST_NAME *list, SKIP_LIST_KEY_TYPE key, SKIP_LIST_VALUE_TYPE value) {
    SKIP_LIST_NODE *new_node = SKIP_LIST_NODE_MEMORY_POOL_FUNC(get)(list->pool);
    if (new_node == NULL) return false;
    // Set up the node
    new_node->key = key;
    new_node->down = NULL;
    new_node->next = (SKIP_LIST_NODE *)value;
    /* Set up node tower. Down pointers will be inverted since we're connecting the tower from the bottom up.
     * For now this is simply serving as a stack of nodes. We'll connect new_node (containing the value,
     * which lives on level "0" below the lowest linked list) as a down pointer when conencting the first tower on level 1.
     */

    // The new node should be connected to the down pointer of the first level of the tower.
    SKIP_LIST_NODE *lower = new_node;

    SKIP_LIST_NODE *top_node = NULL;

    size_t new_node_level = skip_list_random_level(SKIP_LIST_FUNC(get_thread_random)(list));

    for (size_t i = 0; i < new_node_level; i++) {
        top_node = SKIP_LIST_NODE_MEMORY_POOL_FUNC(get)(list->pool);
        if (top_node == NULL) {
            return false;
        }
        top_node->key = key;
        top_node->down = lower;
        top_node->next = NULL;
        if (lower != new_node) {
            // Retain a (temporary) pointer to the upper node
            lower->next = top_node;
        }
        lower = top_node;
    }

    SKIP_LIST_NODE *current_node = top_node;

    /* After this loop, regardless of the number of levels,
     * current_node should be set to the first node in the tower
     * (i.e. the node above new_node, which contains the value)
    */
    for (size_t level = 1; level < new_node_level; level++) {
        current_node = current_node->down;
    }

    /* In this loop, we need to connect each node in the tower to the list
     * at the correct level. This is done without locks by simply setting the
     * next pointer for the preceding node in the list at that level to the
     * tower node. Here if the node does go all the way up to 5
    */
    for (size_t level = 1; level <= new_node_level; level++) {
        SKIP_LIST_NODE *upper = current_node->next;

        SKIP_LIST_HEAD head = atomic_load(&list->head);

        SKIP_LIST_NODE *prev_node = head.node;
        SKIP_LIST_NODE *next_node = NULL;

        size_t current_level = head.max_level;
        while (current_level >= level) {
            while ((next_node = atomic_load(&prev_node->next)) != NULL && (SKIP_LIST_KEY_LESS_THAN(next_node->key, key))) {
                prev_node = next_node;
            }
            if (current_level == level) {
                break;
            }
            prev_node = prev_node->down;
            current_level--;
        }

        // Can set this non-atomically because we're not connected to the list at this level yet
        current_node->next = next_node;

        if (head.max_level < level) {
            SKIP_LIST_NODE *new_head_node = SKIP_LIST_NODE_MEMORY_POOL_FUNC(get)(list->pool);
            new_head_node->next = current_node;
            new_head_node->down = head.node;
            new_head_node->key = head.node->key;

            SKIP_LIST_HEAD new_head = (SKIP_LIST_HEAD){
                .node = new_head_node,
                .max_level = level,
                .version = head.version + 1
            };
            while (!atomic_compare_exchange_weak(&list->head, &head, new_head)) {
                head = atomic_load(&list->head);
                if (head.max_level < level) {
                    new_head.version = head.version + 1;
                } else {
                    SKIP_LIST_NODE_MEMORY_POOL_FUNC(release)(list->pool, new_head_node);
                    break;
                }
            }
            head = new_head;
        } else {
            if (!atomic_compare_exchange_strong(&prev_node->next, &next_node, current_node)) {
                // keep trying until we succeed
                current_node->next = upper;
                level--;
                continue;
            }
        }

        current_node = upper;
    }

    atomic_fetch_add(&list->size, 1);
    return true;
}


#else

size_t SKIP_LIST_FUNC(size)(SKIP_LIST_NAME *list) {
    if (list == NULL) return 0;
    return list->size;
}

void *SKIP_LIST_FUNC(get)(SKIP_LIST_NAME *list, SKIP_LIST_KEY_TYPE key) {
    if (list == NULL || list->head == NULL || list->head->next == NULL) return NULL;
    bool beyond_placeholder = false;
    SKIP_LIST_NODE *current = list->head;
    while (current->down != NULL) {
        while (current->next != NULL && (
                    (SKIP_LIST_KEY_LESS_THAN(current->next->key, key))
                 || (SKIP_LIST_KEY_EQUALS(current->next->key, key)))) {
            current = current->next;
            beyond_placeholder = true;
        }
        current = current->down;
    }

    if (beyond_placeholder && SKIP_LIST_KEY_EQUALS(current->key, key)) {
        return (void *)current->next;
    }
    return NULL;
}


void *SKIP_LIST_FUNC(get_prev)(SKIP_LIST_NAME *list, SKIP_LIST_KEY_TYPE key) {
    if (list == NULL || list->head == NULL || list->head->next == NULL) return NULL;
    SKIP_LIST_NODE *current = list->head;
    while (current->down != NULL) {
        while (current->next != NULL && (SKIP_LIST_KEY_LESS_THAN(current->next->key, key))) {
            current = current->next;
        }
        current = current->down;
    }
    if (current->next != NULL) {
        return (void *)current->next;
    }
    return NULL;
}

void *SKIP_LIST_FUNC(get_next)(SKIP_LIST_NAME *list, SKIP_LIST_KEY_TYPE key) {
    if (list == NULL || list->head == NULL || list->head->next == NULL) return NULL;
    bool beyond_placeholder = false;
    SKIP_LIST_NODE *current = list->head;
    SKIP_LIST_NODE *upper_node = NULL;
    while (current->down != NULL) {
        while (current->next != NULL && (
                    (SKIP_LIST_KEY_LESS_THAN(current->next->key, key))
                 || (SKIP_LIST_KEY_EQUALS(current->next->key, key)))) {
            current = current->next;
        }
        upper_node = current;
        current = current->down;
    }
    if (upper_node != NULL && upper_node->next != NULL) {
        return (void *)upper_node->next->down->next;
    }
    return NULL;
}

bool SKIP_LIST_FUNC(insert)(SKIP_LIST_NAME *list, SKIP_LIST_KEY_TYPE key, SKIP_LIST_VALUE_TYPE value) {
    SKIP_LIST_NODE *tmp_node = NULL;
    SKIP_LIST_NODE *new_node = SKIP_LIST_NODE_MEMORY_POOL_FUNC(get)(list->pool);
    if (new_node == NULL) return false;
    new_node->key = key;
    new_node->down = NULL;
    new_node->next = (SKIP_LIST_NODE *)value;
    size_t new_node_level = skip_list_random_level(&list->random);
    for (size_t i = 0; i < new_node_level; i++) {
        tmp_node = SKIP_LIST_NODE_MEMORY_POOL_FUNC(get)(list->pool);
        if (tmp_node == NULL) {
            return false;
        }
        tmp_node->key = key;
        tmp_node->down = new_node;
        tmp_node->next = NULL;
        new_node = tmp_node;
    }

    SKIP_LIST_NODE *head = list->head;
    tmp_node = head;
    while (list->max_level < new_node_level) {
        tmp_node = SKIP_LIST_NODE_MEMORY_POOL_FUNC(get)(list->pool);
        if (tmp_node == NULL) {
            for (size_t i = 0; i <= new_node_level && new_node != NULL; i++) {
                tmp_node = new_node->down;
                SKIP_LIST_NODE_MEMORY_POOL_FUNC(release)(list->pool, new_node);
                new_node = tmp_node;
            }
            return false;
        }
        tmp_node->down = head->down;
        tmp_node->next = head->next;
        tmp_node->key = head->key;
        head->down = tmp_node;
        head->next = NULL;
        list->max_level++;
    }

    SKIP_LIST_NODE *current_node = list->head;
    size_t current_level = list->max_level;
    while (current_level >= 1) {
        while (current_node->next != NULL && (
                SKIP_LIST_KEY_LESS_THAN(current_node->next->key, key))) {
            current_node = current_node->next;
        }
        if (current_level <= new_node_level) {
            new_node->next = current_node->next;
            current_node->next = new_node;
            new_node = new_node->down;
        }
        if (current_level >= 2) {
            current_node = current_node->down;
        }
        current_level--;
    }
    list->size++;
    return true;
}

void *SKIP_LIST_FUNC(delete)(SKIP_LIST_NAME *list, SKIP_LIST_KEY_TYPE key) {
    if (list == NULL || list->head == NULL) return NULL;
    void *deleted = NULL;
    SKIP_LIST_NODE *current_node = list->head;
    SKIP_LIST_NODE *tmp_node = NULL;
    while (current_node->down != NULL) {
        while (current_node->next != NULL && (
            SKIP_LIST_KEY_LESS_THAN(current_node->next->key, key))) {
            current_node = current_node->next;
        }
        if (current_node->next != NULL && SKIP_LIST_KEY_EQUALS(current_node->next->key, key)) {
            tmp_node = current_node->next;
            // unlink node
            current_node->next = tmp_node->next;
            if (tmp_node->down->down == NULL) {
                // delete leaf
                deleted = (void *)tmp_node->down->next;
                SKIP_LIST_NODE_MEMORY_POOL_FUNC(release)(list->pool, tmp_node->down);
            }
            SKIP_LIST_NODE_MEMORY_POOL_FUNC(release)(list->pool, tmp_node);
        }
        current_node = current_node->down;
    }
    // remove empty levels in placeholder
    while (list->head->down != NULL && list->head->next == NULL) {
        tmp_node = list->head->down;
        list->head->down = tmp_node->down;
        list->head->next = tmp_node->next;
        list->max_level--;
        SKIP_LIST_NODE_MEMORY_POOL_FUNC(release)(list->pool, tmp_node);
    }
    list->size--;
    return deleted;
}
#endif
void SKIP_LIST_FUNC(destroy)(SKIP_LIST_NAME *list) {
    if (list == NULL) return;
    if (list->pool != NULL) {
        SKIP_LIST_NODE_MEMORY_POOL_FUNC(destroy)(list->pool);
    }
    free(list);
}


#undef SKIP_LIST_CONCAT_
#undef SKIP_LIST_CONCAT
#undef SKIP_LIST_FUNC
#undef SKIP_LIST_TYPED