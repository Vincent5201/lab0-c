#include <stdio.h>
#include <string.h>

#include "sorts.h"

struct list_head *merge(struct list_head *a, struct list_head *b, bool descend)
{
    struct list_head *head = NULL, **tail = &head;
    for (;;) {
        if (cmp_xor_order(a, b, element_t, list, descend)) {
            *tail = a;
            tail = &a->next;
            a = a->next;
            if (!a) {
                *tail = b;
                break;
            }
        } else {
            *tail = b;
            tail = &b->next;
            b = b->next;
            if (!b) {
                *tail = a;
                break;
            }
        }
    }
    return head;
}

void merge_final(struct list_head *head,
                 struct list_head *a,
                 struct list_head *b,
                 bool descend)
{
    struct list_head *tail = head;
    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp_xor_order(a, b, element_t, list, descend)) {
            tail->next = a;
            a->prev = tail;
            tail = a;
            a = a->next;
            if (!a)
                break;
        } else {
            tail->next = b;
            b->prev = tail;
            tail = b;
            b = b->next;
            if (!b) {
                b = a;
                break;
            }
        }
    }

    /* Finish linking remainder of list b on to tail */
    tail->next = b;
    do {
        b->prev = tail;
        tail = b;
        b = b->next;
    } while (b);

    /* And the final links to make a circular doubly-linked list */
    tail->next = head;
    head->prev = tail;
}

void list_sort(struct list_head *head, bool descend)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    struct list_head *list = head->next, *pending = NULL;
    size_t count = 0; /* Count of pending */

    if (list == head->prev) /* Zero or one elements */
        return;

    /* Convert to a null-terminated singly-linked list. */
    head->prev->next = NULL;
    do {
        size_t bits;
        struct list_head **tail = &pending;

        /* Find the least-significant clear bit in count */
        for (bits = count; bits & 1; bits >>= 1)
            tail = &(*tail)->prev;
        /* Do the indicated merge */
        if (likely(bits)) {
            struct list_head *a = *tail, *b = a->prev;
            a = merge(b, a, descend);
            /* Install the merged result in place of the inputs */
            a->prev = b->prev;
            *tail = a;
        }

        /* Move one element from input list to pending */
        list->prev = pending;
        pending = list;
        list = list->next;
        pending->next = NULL;
        count++;
    } while (list);

    /* End of input; merge together all the pending lists. */
    list = pending;
    pending = pending->prev;
    for (;;) {
        struct list_head *next = pending->prev;

        if (!next)
            break;
        list = merge(pending, list, descend);
        pending = next;
    }
    /* The final merge, rebuilding prev links */
    merge_final(head, pending, list, descend);
}

int get_minrun(size_t size)
{
    size_t add = size & 0x3F;
    while (size >= 64)
        size >>= 1;
    return add ? size + 1 : size;
}

/* for a singly linked list without head */
struct list_head *insert_sort(struct list_head *list,
                              struct list_head *node,
                              bool descend)
{
    if (!list && !node)
        return NULL;
    if (!list || !node)
        return list ? list : node;
    struct list_head *first = NULL, **p = &first;
    while (list && cmp_xor_order(list, node, element_t, list, descend)) {
        *p = list;
        p = &list->next;
        list = list->next;
    }
    *p = node;
    node->next = list;
    return first;
}

void insertion_sort(struct list_head *head, bool descend)
{
    head->prev->next = NULL;
    struct list_head *list = head->next;
    struct list_head *node = list->next;
    struct list_head *lists = node->next;
    list->next = NULL;
    while (lists) {
        node->next = NULL;
        list = insert_sort(list, node, descend);
        node = lists;
        lists = lists->next;
    }
    list = insert_sort(list, node, descend);
    head->next = list;
    node = head;
    while (list) {
        list->prev = node;
        node = list;
        list = list->next;
    }
    node->next = head;
    head->prev = node;
}

void find_runs(struct list_head *head, bool descend, int minrun)
{
    bool descend_now = false;
    struct list_head *lists = head->next, *node = NULL;
    struct list_head *runs_now = NULL, *runs_last = NULL;
    int last_runs_len = 0;
    head->prev->next = NULL;
    head->next = NULL;
    head->prev = NULL;
    while (lists) {
        node = lists->next;
        descend_now = cmp_xor_order(lists, node, element_t, list, descend_now)
                          ? descend_now
                          : (!descend_now);
        runs_now = lists;
        int runs_len = 1;
        while (node &&
               cmp_xor_order(lists, node, element_t, list, descend_now)) {
            lists = node;
            node = node->next;
            runs_len++;
        }
        lists->next = NULL;
        if (descend_now != descend) {
            LIST_HEAD(tmp);
            INIT_LIST_HEAD(&tmp);
            lists->next = &tmp;
            (&tmp)->prev = lists;
            (&tmp)->next = runs_now;
            runs_now->prev = &tmp;
            q_reverse(&tmp);
            runs_now = (&tmp)->next;
            (&tmp)->prev->next = NULL;
        }
        lists = node;
        if (runs_len >= minrun) {
            runs_now->prev = head->prev;
            head->prev = runs_now;
        } else {
            if (runs_last) {
                last_runs_len += runs_len;
                while (runs_now) {
                    node = runs_now;
                    runs_now = runs_now->next;
                    node->next = NULL;
                    runs_last = insert_sort(runs_last, node, descend);
                }
                if (last_runs_len >= minrun) {
                    runs_last->prev = head->prev;
                    head->prev = runs_last;
                    runs_last = NULL;
                    last_runs_len = 0;
                }
            } else {
                runs_last = runs_now;
                last_runs_len = runs_len;
            }
        }
    }
    if (runs_last) {
        runs_last->prev = head->prev;
        head->prev = runs_last;
    }
}

int q_size_single(struct list_head *list)
{
    if (!list)
        return 0;
    int count = 0;
    struct list_head *node = list;
    while (node) {
        count++;
        node = node->next;
    }
    return count;
}

void Timsort(struct list_head *head, bool descend)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    size_t size = q_size(head);
    if (size <= 64) {
        insertion_sort(head, descend);
        return;
    }
    find_runs(head, descend, get_minrun(size));
    struct list_head *lists = head->prev, *pending = NULL, *list;
    size_t count = 0;
    do {
        size_t bits;
        struct list_head **tail = &pending;
        for (bits = count; bits & 1; bits >>= 1)
            tail = &(*tail)->prev;
        if (likely(bits)) {
            struct list_head *a = *tail, *b = a->prev;
            a = merge(b, a, descend);
            a->prev = b->prev;
            *tail = a;
        }
        list = lists;
        lists = lists->prev;
        list->prev = pending;
        pending = list;
        count++;
    } while (lists);
    list = pending;
    pending = pending->prev;
    for (;;) {
        struct list_head *next = pending->prev;
        if (!next)
            break;
        list = merge(pending, list, descend);
        pending = next;
    }
    merge_final(head, pending, list, descend);
}