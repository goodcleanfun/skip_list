#ifndef SKIP_LIST_H
#define SKIP_LIST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

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
    struct SKIP_LIST_TYPED(node) *next;
    struct SKIP_LIST_TYPED(node) *down;
} SKIP_LIST_TYPED(node_t);

#define SKIP_LIST_NODE SKIP_LIST_TYPED(node_t)

#define SKIP_LIST_NODE_MEMORY_POOL_NAME SKIP_LIST_TYPED(node_memory_pool)

#define MEMORY_POOL_NAME SKIP_LIST_NODE_MEMORY_POOL_NAME
#define MEMORY_POOL_TYPE SKIP_LIST_NODE
#include "memory_pool/memory_pool.h"
#undef MEMORY_POOL_NAME
#undef MEMORY_POOL_TYPE

#define SKIP_LIST_NODE_MEMORY_POOL_FUNC(name) SKIP_LIST_CONCAT(SKIP_LIST_NODE_MEMORY_POOL_NAME, _##name)

typedef struct {
    SKIP_LIST_NODE *head;
    size_t max_level;
    SKIP_LIST_NODE_MEMORY_POOL_NAME *pool;
} SKIP_LIST_NAME;

SKIP_LIST_NAME *SKIP_LIST_FUNC(new)(void) {
    SKIP_LIST_NAME *list = calloc(1, sizeof(SKIP_LIST_NAME));
    if (list == NULL) return NULL;
    list->pool = SKIP_LIST_NODE_MEMORY_POOL_FUNC(new)();
    if (list->pool == NULL) {
        free(list);
        return NULL;
    }
    SKIP_LIST_NODE *head = SKIP_LIST_NODE_MEMORY_POOL_FUNC(get)(list->pool);
    if (head == NULL) {
        SKIP_LIST_NODE_MEMORY_POOL_FUNC(destroy)(list->pool);
        free(list);
        return NULL;
    }
    list->max_level = 0;
    head->next = NULL;
    head->down = NULL;
    list->head = head;

    return list;
}

void *SKIP_LIST_FUNC(get)(SKIP_LIST_NAME *list, SKIP_LIST_KEY_TYPE key) {
    if (list == NULL || list->head == NULL) return NULL;
    bool beyond_placeholder = false;
    if (list->head->next == NULL) return NULL;
    SKIP_LIST_NODE *current = list->head;
    size_t current_level = list->max_level;
    while (current->down != NULL) {
        while (current->next != NULL && (
                    (SKIP_LIST_KEY_LESS_THAN(current->next->key, key))
                 || (SKIP_LIST_KEY_EQUALS(current->next->key, key)))) {
            current = current->next;
            beyond_placeholder = true;
        }
        current = current->down;
        current_level--;
    }

    if (beyond_placeholder && SKIP_LIST_KEY_EQUALS(current->key, key)) {
        return (void *)current->next;
    }
    return NULL;
}


void *SKIP_LIST_FUNC(get_prev)(SKIP_LIST_NAME *list, SKIP_LIST_KEY_TYPE key) {
    if (list == NULL || list->head == NULL) return NULL;
    if (list->head->next == NULL) return NULL;
    SKIP_LIST_NODE *current = list->head;
    SKIP_LIST_NODE *prev = NULL;
    size_t current_level = list->max_level;
    while (current_level >= 1) {
        while (current->next != NULL && (SKIP_LIST_KEY_LESS_THAN(current->next->key, key))) {
            current = current->next;
        }
        current = current->down;
        current_level--;
    }
    if (current_level == 0 && current->next != NULL) {
        return (void *)current->next;
    }
    return NULL;
}

void *SKIP_LIST_FUNC(get_next)(SKIP_LIST_NAME *list, SKIP_LIST_KEY_TYPE key) {
    if (list == NULL || list->head == NULL) return NULL;
    bool beyond_placeholder = false;
    if (list->head->next == NULL) return NULL;
    SKIP_LIST_NODE *current = list->head;
    size_t current_level = list->max_level;
    while (current_level >= 1) {
        while (current->next != NULL && (
                    (SKIP_LIST_KEY_LESS_THAN(current->next->key, key))
                 || (SKIP_LIST_KEY_EQUALS(current->next->key, key)))) {
            current = current->next;
        }
        if (current_level > 1) {
            current = current->down;
        }
        current_level--;
    }
    if (current_level == 0 && current != NULL && current->next != NULL) {
        return (void *)current->next->down->next;
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
    size_t new_node_level = 0;
    do {
        tmp_node = SKIP_LIST_NODE_MEMORY_POOL_FUNC(get)(list->pool);
        if (tmp_node == NULL) {
            return false;
        }
        tmp_node->key = key;
        tmp_node->down = new_node;
        tmp_node->next = NULL;
        new_node = tmp_node;
        new_node_level++;
    } while ((rand() % 2) == 0);

    SKIP_LIST_NODE *head = list->head;
    tmp_node = head;
    while (list->max_level < new_node_level) {
        tmp_node = SKIP_LIST_NODE_MEMORY_POOL_FUNC(get)(list->pool);
        if (tmp_node == NULL) {
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
        if (SKIP_LIST_KEY_EQUALS(current_node->next->key, key)) {
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
    return deleted;
}

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