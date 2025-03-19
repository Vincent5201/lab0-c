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

static int get_minrun(size_t size)
{
    size_t add = size & 0x3F;
    while (size >= 64)
        size >>= 1;
    return add ? size + 1 : size;
}

/* insert the node to a sorted singly linked list without head */
struct list_head *insert_sort(struct list_head *list,
                              struct list_head *node,
                              bool descend)
{
    if (!node)
        return list;
    if (!list)
        return node;
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

/* insert a node to a sorted linked list from specific node */
struct list_head *insert_sort_behind(struct list_head *node,
                                     struct list_head *behind,
                                     bool descend)
{
    struct list_head *pos = NULL, **p = &pos, *tmp;
    tmp = behind->next;
    behind->next = pos;
    behind = tmp;
    while (behind && cmp_xor_order(behind, node, element_t, list, descend)) {
        *p = behind;
        p = &behind->next;
        behind = behind->next;
    }
    *p = node;
    node->next = behind;
    return node;
}

void insertion_sort(struct list_head *head, bool descend)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;

    /* insert nodes one-by-one*/
    struct list_head *node, *list = NULL, *lists = head->next;
    head->prev->next = NULL;
    while (lists) {
        node = lists;
        lists = lists->next;
        node->next = NULL;
        list = insert_sort(list, node, descend);
    }

    /* rebuild link */
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

static void find_runs(struct list_head *head, bool descend, int minrun)
{
    int last_runs_len = 0;
    bool descend_now = false;
    struct list_head *lists = head->next, runs_last;
    (&runs_last)->next = (&runs_last)->prev = NULL;
    head->prev->next = NULL;
    head->next->prev = NULL;
    head->next = head->prev = NULL;
    while (lists) {
        /* for last node */
        if (unlikely(!(lists->next))) {
            lists->prev = head->prev;
            head->prev = lists;
            break;
        }
        int runs_len = 1;
        struct list_head *node = lists, *runs_now = lists;
        lists = lists->next;
        descend_now = cmp_xor_order(node, lists, element_t, list, descend_now)
                          ? descend_now
                          : (!descend_now);
        while (lists &&
               cmp_xor_order(node, lists, element_t, list, descend_now)) {
            node = lists;
            lists = lists->next;
            runs_len++;
        }
        node->next = NULL;
        runs_now->prev = NULL;
        last_runs_len += runs_len;
        if (!((&runs_last)->next)) {
            (&runs_last)->next = runs_now;
            if (descend != descend_now) {
                runs_now->prev = (&runs_last);
                (&runs_last)->prev = node;
                node->next = (&runs_last);
                q_reverse(&runs_last);
                (&runs_last)->next->prev = NULL;
                (&runs_last)->prev->next = NULL;
                (&runs_last)->prev = NULL;
            }
        } else {
            struct list_head *behind = (&runs_last);
            if (descend == descend_now) {
                while (runs_now && behind->next) {
                    node = runs_now;
                    runs_now = runs_now->next;
                    node->next = NULL;
                    behind = insert_sort_behind(node, behind, descend);
                }
                if (runs_now)
                    behind->next = runs_now;
            } else {
                runs_now = node;
                while (runs_now && behind->next) {
                    node = runs_now;
                    runs_now = runs_now->prev;
                    node->next = NULL;
                    behind = insert_sort_behind(node, behind, descend);
                }
                while (runs_now) {
                    node = runs_now;
                    runs_now = runs_now->prev;
                    node->next = NULL;
                    behind->next = node;
                    behind = node;
                }
                behind->next = NULL;
            }
        }
        if (last_runs_len > minrun) {
            (&runs_last)->next->prev = head->prev;
            head->prev = (&runs_last)->next;
            last_runs_len = 0;
            (&runs_last)->next = (&runs_last)->prev = NULL;
            // printf("add1\n");
        }
    }
    int tmp = 0;
    lists = head->prev;
    while (lists) {
        tmp++;
        lists = lists->prev;
    }
    // printf("endf%d\n", tmp);
    if (likely((&runs_last)->next)) {
        (&runs_last)->next->prev = head->prev;
        head->prev = (&runs_last)->next;
        // printf("add2\n");
    }
    tmp = 0;
    lists = head->prev;
    while (lists) {
        tmp++;
        lists = lists->prev;
    }
    // printf("endf%d\n", tmp);
}

void hybrid_sort(struct list_head *head, bool descend)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    /* if too short use insertion sort */
    size_t size = q_size(head);
    if (size <= 64) {
        insertion_sort(head, descend);
        return;
    }

    /* generate runs */
    find_runs(head, descend, get_minrun(size));

    /* initate list_sort() to merge runs */
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
        // printf("count%ld\n", count);
    } while (lists);
    // printf("endm\n");
    list = pending;
    pending = pending->prev;
    for (;;) {
        struct list_head *next = pending->prev;
        if (!next)
            break;
        list = merge(pending, list, descend);
        pending = next;
    }
    // printf("endmm\n");
    merge_final(head, pending, list, descend);
}

void qk_sort(struct list_head *head, bool descend)
{
    // printf("loopf %d\n", q_size(head));
    struct list_head list_less, list_greater;
    element_t *pivot, *item = NULL, *is = NULL;

    if (list_empty(head) || list_is_singular(head))
        return;

    INIT_LIST_HEAD(&list_less);
    INIT_LIST_HEAD(&list_greater);

    pivot = list_first_entry(head, element_t, list);
    list_del(&pivot->list);

    list_for_each_entry_safe(item, is, head, list) {
        if (cmp_xor_order(&item->list, &pivot->list, element_t, list,
                          descend)) {
            list_move_tail(&item->list, &list_less);
        } else {
            list_move_tail(&item->list, &list_greater);
        }
    }

    qk_sort(&list_less, descend);
    qk_sort(&list_greater, descend);
    list_move_tail(&pivot->list, head);
    list_splice(&list_less, head);
    list_splice_tail(&list_greater, head);
    // printf("loope %d\n", q_size(head));
}