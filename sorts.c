#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "queue.h"
#include "sorts.h"

#define cmp_element_t_val(a, b)                     \
    strcmp(container_of(a, element_t, list)->value, \
           container_of(b, element_t, list)->value)

static void inline q_connect(struct list_head *first, struct list_head *second)
{
    first->next = second;
    second->prev = first;
    return;
}

static struct list_head *k_merge(struct list_head *a, struct list_head *b)
{
    struct list_head *head = NULL, **tail = &head;
    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp_element_t_val(a, b) <= 0) {
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

static void inline build_prev_link(struct list_head *head,
                                   struct list_head *tail,
                                   struct list_head *list)
{
    tail->next = list;
    do {
        list->prev = tail;
        tail = list;
        list = list->next;
    } while (list);
    q_connect(tail, head);
}

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
static void k_merge_final(struct list_head *head,
                          struct list_head *a,
                          struct list_head *b)
{
    struct list_head *tail = head;
    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp_element_t_val(a, b) <= 0) {
            q_connect(tail, a);
            tail = a;
            a = a->next;
            if (!a)
                break;
        } else {
            q_connect(tail, b);
            tail = b;
            b = b->next;
            if (!b) {
                b = a;
                break;
            }
        }
    }
    /* Finish linking remainder of list b on to tail */
    build_prev_link(head, tail, b);
}

/* Copird from lib/list_sort.c then modify it */
void k_sort(struct list_head *head)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    struct list_head *list = head->next, *pending = NULL;
    size_t count = 0;
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
            a = k_merge(b, a);
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
        list = k_merge(pending, list);
        pending = next;
    }
    /* The final merge, rebuilding prev links */
    k_merge_final(head, pending, list);
}

/* Insertion Sort */
struct list_head *i_sort(struct list_head *head)
{
    struct list_head *tail = head, *lists = head->next;
    head->next = NULL;

    while (lists && lists != head) {
        if (cmp_element_t_val(lists, tail) >= 0) {
            tail->next = lists;
            lists = lists->next;
            tail = tail->next;
            tail->next = NULL;
        } else if (cmp_element_t_val(lists, head) <= 0) {
            struct list_head *node = lists;
            lists = lists->next;
            node->next = head;
            head = node;
        } else {
            struct list_head *node = head->next, *last_node = head;
            while (cmp_element_t_val(lists, node) > 0) {
                last_node = node;
                node = node->next;
            }
            last_node->next = lists;
            lists = lists->next;
            last_node->next->next = node;
        }
    }
    return head;
}
/* try to improve h_sort */
void h_sort(struct list_head *head)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    int size = q_size(head);
    if (size < 64) {
        /* insertion sort */
        struct list_head *node = head->next;
        head->prev->next = NULL;
        node = i_sort(node);
        q_connect(head, node);
        while (node->next && node->next != head) {
            node->next->prev = node;
            node = node->next;
        }
        q_connect(node, head);
        return;
    }
    /* set run size */
    bool add = false;
    while (size >= 64) {
        if (size & 1)
            add = true;
        size = size >> 1;
    }
    if (add)
        size++;
    /* put 1 node */
    struct list_head *lists = head->next;
    struct list_head *pending = lists;
    struct list_head *tail = pending;
    head->prev->next = NULL;
    lists->prev = NULL;
    lists = lists->next;
    pending->next = NULL;
    int tail_count = 1, run_count = 0;
    do {
        /* take nodes */
        while (lists && tail_count < size) {
            tail_count++;
            if (cmp_element_t_val(lists, tail) >= 0) {
                tail->next = lists;
                tail = lists;
                lists = lists->next;
                tail->next = NULL;
            } else {
                struct list_head *pos = pending, *last_pos = NULL;
                while (pos && cmp_element_t_val(pos, lists) < 0) {
                    last_pos = pos;
                    pos = pos->next;
                }
                if (last_pos) {
                    last_pos->next = lists;
                    lists = lists->next;
                    last_pos->next->next = pos;
                } else {
                    struct list_head *tmp = lists;
                    lists = lists->next;
                    tmp->next = pending;
                    tmp->prev = pending->prev;
                    pending = tmp;
                }
            }
        }
        /* merge */
        run_count++;
        size_t bits;
        struct list_head **run_tail = &pending;
        for (bits = run_count; bits & 1; bits >>= 1)
            run_tail = &(*run_tail)->prev;
        if (likely(bits)) {
            struct list_head *a = *run_tail, *b = a->prev;
            a = k_merge(b, a);
            a->prev = b->prev;
            *run_tail = a;
        }
        if (lists) {
            lists->prev = pending;
            pending = lists;
            lists = lists->next;
            pending->next = NULL;
            tail = pending;
            tail_count = 1;
        }
    } while (lists);
    /* merge all*/
    lists = pending;
    pending = pending->prev;
    for (;;) {
        struct list_head *next = pending->prev;
        if (!next)
            break;
        lists = k_merge(pending, lists);
        pending = next;
    }
    /* The final merge, rebuilding prev links */
    k_merge_final(head, pending, lists);
}
/* old version of h_sort */
/*
void h_sort(struct list_head *head)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    int size = q_size(head);
    if (size < 64) {
        struct list_head *node = head->next;
        head->prev->next = NULL;
        node = i_sort(node);
        q_connect(head, node);
        while (node->next && node->next != head) {
            node->next->prev = node;
            node = node->next;
        }
        q_connect(node, head);
        return;
    }

    bool add = false;
    while (size >= 64) {
        if (size & 1)
            add = true;
        size = size >> 1;
    }
    if (add)
        size++;

    struct list_head *lists = head->next, *pending = NULL;
    head->prev->next = NULL;
    lists->prev = pending;
    pending = lists;
    lists = lists->next;
    pending->next = NULL;
    struct list_head *tail = pending;
    int tail_count = 1, run_count = 0;
    do {
        if (unlikely(tail_count == size)) {
            run_count++;
            size_t bits;
            struct list_head **run_tail = &pending;
            for (bits = run_count; bits & 1; bits >>= 1)
                run_tail = &(*run_tail)->prev;
            if (likely(bits)) {
                struct list_head *a = *run_tail, *b = a->prev;
                a = k_merge(b, a);
                a->prev = b->prev;
                *run_tail = a;
            }
            lists->prev = pending;
            pending = lists;
            lists = lists->next;
            pending->next = NULL;
            tail = pending;
            tail_count = 1;
        } else {

            struct list_head *tmp = lists;
            lists = lists->next;
            tail_count++;
            if (unlikely(cmp_element_t_val(tmp, tail) >= 0)) {
                tail->next = tmp;
                tail = tmp;
                tail->next = NULL;
            } else {
                struct list_head *pos = pending, *last_pos = NULL;
                while (pos && cmp_element_t_val(pos, tmp) < 0) {
                    last_pos = pos;
                    pos = pos->next;
                }
                if (likely(last_pos)) {
                    last_pos->next = tmp;
                    tmp->next = pos;
                } else {
                    tmp->next = pending;
                    tmp->prev = pending->prev;
                    pending = tmp;
                }
            }
        }
    } while (lists);
    lists = pending;
    pending = pending->prev;
    for (;;) {
        struct list_head *next = pending->prev;
        if (!next)
            break;
        lists = k_merge(pending, lists);
        pending = next;
    }
    k_merge_final(head, pending, lists);
}
*/

static inline size_t run_size(struct list_head *head)
{
    if (!head)
        return 0;
    if (!head->next)
        return 1;
    return (size_t) (head->next->prev);
}

struct pair {
    struct list_head *head, *next;
};

static size_t stk_size;

static struct pair find_run(struct list_head *list)
{
    size_t len = 1;
    struct list_head *next = list->next, *head = list;
    struct pair result;

    if (!next) {
        result.head = head, result.next = next;
        return result;
    }

    if (cmp_element_t_val(list, next) > 0) {
        /* decending run, also reverse the list */
        struct list_head *prev = NULL;
        do {
            len++;
            list->next = prev;
            prev = list;
            list = next;
            next = list->next;
            head = list;
        } while (next && cmp_element_t_val(list, next) > 0);
        list->next = prev;
    } else {
        do {
            len++;
            list = next;
            next = list->next;
        } while (next && cmp_element_t_val(list, next) <= 0);
        list->next = NULL;
    }
    head->prev = NULL;
    head->next->prev = (struct list_head *) len;
    result.head = head, result.next = next;
    return result;
}

static struct list_head *merge_at(struct list_head *at)
{
    size_t len = run_size(at) + run_size(at->prev);
    struct list_head *prev = at->prev->prev;
    struct list_head *list = k_merge(at->prev, at);
    list->prev = prev;
    list->next->prev = (struct list_head *) len;
    --stk_size;
    return list;
}

static struct list_head *merge_force_collapse(struct list_head *tp)
{
    while (stk_size >= 3) {
        if (run_size(tp->prev->prev) < run_size(tp)) {
            tp->prev = merge_at(tp->prev);
        } else {
            tp = merge_at(tp);
        }
    }
    return tp;
}

static struct list_head *merge_collapse(struct list_head *tp)
{
    int n;
    while ((n = stk_size) >= 2) {
        if ((n >= 3 &&
             run_size(tp->prev->prev) <= run_size(tp->prev) + run_size(tp)) ||
            (n >= 4 && run_size(tp->prev->prev->prev) <=
                           run_size(tp->prev->prev) + run_size(tp->prev))) {
            if (run_size(tp->prev->prev) < run_size(tp)) {
                tp->prev = merge_at(tp->prev);
            } else {
                tp = merge_at(tp);
            }
        } else if (run_size(tp->prev) <= run_size(tp)) {
            tp = merge_at(tp);
        } else {
            break;
        }
    }
    return tp;
}

void t_sort(struct list_head *head)
{
    stk_size = 0;

    struct list_head *list = head->next, *tp = NULL;
    if (head == head->prev)
        return;

    /* Convert to a null-terminated singly-linked list. */
    head->prev->next = NULL;

    do {
        /* Find next run */
        struct pair result = find_run(list);
        result.head->prev = tp;
        tp = result.head;
        list = result.next;
        stk_size++;
        tp = merge_collapse(tp);
    } while (list);

    /* End of input; merge together all the runs. */
    tp = merge_force_collapse(tp);

    /* The final merge; rebuild prev links */
    struct list_head *stk0 = tp, *stk1 = stk0->prev;
    while (stk1 && stk1->prev)
        stk0 = stk0->prev, stk1 = stk1->prev;
    if (stk_size <= 1) {
        build_prev_link(head, head, stk0);
        return;
    }
    k_merge_final(head, stk1, stk0);
}