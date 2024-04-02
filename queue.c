#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */


/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *head = malloc(sizeof(struct list_head));
    if (!head)
        return NULL;
    INIT_LIST_HEAD(head);
    return head;
}

/* Free all storage used by queue */
void q_free(struct list_head *head)
{
    if (!head)
        return;
    element_t *entry, *safe;
    list_for_each_entry_safe (entry, safe, head, list)
        q_release_element(entry);
    free(head);
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *elem = malloc(sizeof(element_t));
    if (!elem)
        return false;
    elem->value = strdup(s);
    if (!elem->value) {
        free(elem);
        return false;
    }
    /*
        (struct list_head *node)'s node: address of a (list_head node)
        entry->list: a (list_head node)
        so &(entry->list) is address of that (list_head node)
    */
    list_add(&elem->list, head);
    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *elem = malloc(sizeof(element_t));
    if (!elem)
        return false;
    elem->value = strdup(s);
    if (!elem->value) {
        free(elem);
        return false;
    }
    list_add_tail(&elem->list, head);
    return true;
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *elem = list_first_entry(head, element_t, list);
    list_del(head->next);
    if (sp && bufsize) {
        strncpy(sp, elem->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    return elem;
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *elem = list_last_entry(head, element_t, list);
    list_del(head->prev);
    if (sp && bufsize) {
        strncpy(sp, elem->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    return elem;
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    if (!head || list_empty(head))
        return 0;
    struct list_head *node = head->next;
    int ret = 0;
    while (node && node != head) {
        ret++;
        node = node->next;
    }
    return ret;
}

/* Find the middle node in queue*/
struct list_head *q_find_mid(struct list_head *head)
{
    if (!head || list_empty(head))
        return NULL;
    struct list_head *fast = head->next;
    struct list_head *slow = head->next;
    while (fast->next != head && fast->next->next != head) {
        fast = fast->next->next;
        slow = slow->next;
    }
    return slow;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    if (!head || list_empty(head))
        return false;
    struct list_head *mid_head = q_find_mid(head);
    element_t *mid = list_entry(mid_head, element_t, list);
    list_del_init(mid_head);
    q_release_element(mid);
    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    if (!head)
        return false;
    struct list_head *node;
    bool dup = false;
    list_for_each (node, head) {
        struct list_head *test = node->next;
        while (test != head &&
               strcmp(list_entry(node, element_t, list)->value,
                      list_entry(test, element_t, list)->value) == 0) {
            dup = true;
            list_del(test);
            q_release_element(list_entry(test, element_t, list));
            test = node->next;
        }
        if (dup) {
            test = node;
            node = node->prev;
            list_del_init(test);
            q_release_element(list_entry(test, element_t, list));
            dup = false;
        }
    }
    return true;
}

/* Connect two list_heads. */
void q_connect(struct list_head *first, struct list_head *second)
{
    if (!first || !second)
        return;
    first->next = second;
    second->prev = first;
    return;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    q_reverseK(head, 2);
    return;
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    struct list_head *node;
    struct list_head *safe;
    list_for_each_safe (node, safe, head)
        list_move(node, head);
    return;
}

/* Reverse the nodes of the list k at a time */
void q_reverseK(struct list_head *head, int k)
{
    // https://leetcode.com/problems/reverse-nodes-in-k-group/
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    int count = 0;
    struct list_head *node, *safe, *head_p = head;
    list_for_each_safe (node, safe, head) {
        count++;
        if (!(count % k)) {
            count = 0;
            struct list_head *back = node->prev;
            struct list_head *head_b = head_p;
            while (node != head_p) {
                q_connect(head_b, node);
                node = back;
                back = node->prev;
                head_b = head_b->next;
            }
            q_connect(head_b, safe);
            head_p = head_b;
        }
    }
    return;
}

/* Merge two queue*/
void q_merge2(struct list_head *first, struct list_head *second, bool descend)
{
    LIST_HEAD(head3);
    while (!list_empty(first) && !list_empty(second)) {
        element_t *elem1 = list_first_entry(first, element_t, list);
        element_t *elem2 = list_first_entry(second, element_t, list);
        if (descend) {
            if (strcmp(elem1->value, elem2->value) > 0) {
                list_move_tail(&elem1->list, &head3);
            } else {
                list_move_tail(&elem2->list, &head3);
            }
        } else {
            if (strcmp(elem1->value, elem2->value) > 0) {
                list_move_tail(&elem2->list, &head3);
            } else {
                list_move_tail(&elem1->list, &head3);
            }
        }
    }
    list_splice(&head3, first);
    list_splice_tail(second, first);
    return;
}

/* Sort elements of queue in ascending/descending order */
void q_sort(struct list_head *head, bool descend)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    LIST_HEAD(head2);
    list_cut_position(&head2, head, q_find_mid(head));
    q_sort(head, descend);
    q_sort(&head2, descend);
    q_merge2(head, &head2, descend);
    return;
}

/* Remove every node which has a node with a strictly less value anywhere to
 * the right side of it */
int q_ascend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    if (!head || list_empty(head) || list_is_singular(head))
        return q_size(head);
    element_t *entry, *safe;
    list_for_each_entry_safe (entry, safe, head, list) {
        for (struct list_head *node = &safe->list; node != head;
             node = node->next) {
            if (strcmp(list_entry(node, element_t, list)->value,
                       entry->value) <= 0) {
                list_del_init(&entry->list);
                q_release_element(entry);
                break;
            }
        }
    }
    return q_size(head);
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    if (!head || list_empty(head) || list_is_singular(head))
        return q_size(head);
    element_t *entry, *safe;
    list_for_each_entry_safe (entry, safe, head, list) {
        for (struct list_head *node = &safe->list; node != head;
             node = node->next) {
            if (strcmp(list_entry(node, element_t, list)->value,
                       entry->value) >= 0) {
                list_del_init(&entry->list);
                q_release_element(entry);
                break;
            }
        }
    }
    return q_size(head);
}

/* Merge all the queues into one sorted queue, which is in ascending/descending
 * order */
int q_merge(struct list_head *head, bool descend)
{
    // https://leetcode.com/problems/merge-k-sorted-lists/
    if (!head || list_empty(head))
        return 0;
    if (list_is_singular(head))
        return q_size(list_first_entry(head, queue_contex_t, chain)->q);
    queue_contex_t *first, *second;
    first = list_first_entry(head, queue_contex_t, chain);
    second = list_entry(first->chain.next, queue_contex_t, chain);
    while (second->size) {
        q_merge2(first->q, second->q, descend);
        list_move_tail(&second->chain, head);
        INIT_LIST_HEAD(second->q);
        second->size = 0;
        second = list_entry(first->chain.next, queue_contex_t, chain);
    }
    return q_size(head);
}

static struct list_head *k_merge(struct list_head *a, struct list_head *b)
{
    struct list_head *head = NULL, **tail = &head;
    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (strcmp(container_of(a, element_t, list)->value,
                   container_of(b, element_t, list)->value) <= 0) {
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

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
static void k_merge_final(struct list_head *head,
                          struct list_head *a,
                          struct list_head *b)
{
    struct list_head *tail = head;
    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (strcmp(container_of(a, element_t, list)->value,
                   container_of(b, element_t, list)->value) <= 0) {
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

/* Copird from lib/list_sort.c and modified */
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
void i_sort(struct list_head *head)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    struct list_head *node, *safe;
    list_for_each_safe (node, safe, head) {
        struct list_head *back = node->prev;
        list_del(node);
        while (back != head &&
               strcmp(container_of(node, element_t, list)->value,
                      container_of(back, element_t, list)->value) <= 0)
            back = back->prev;
        struct list_head *next = back->next;
        q_connect(back, node);
        q_connect(node, next);
    }
    return;
}

void Timsort(struct list_head *head)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    int size = q_size(head);
    if (size < 64) {
        i_sort(head);
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
    struct list_head *list = head->next, *pending = NULL;
    head->prev->next = NULL;

    list->prev = pending;
    pending = list;
    list = list->next;
    int tail_count = 1;
    struct list_head *tail = pending;
    do {
        struct list_head *node = list;
        list = list->next;
        if (strcmp(container_of(node, element_t, list)->value,
                   container_of(tail, element_t, list)->value) >= 0) {
            tail->next = node;
            node->prev = tail;
            tail = node;
            tail_count++;
        } else {
            if (tail_count > size) {
                /* I don't understand when to merge */
            } else {
                struct list_head *pos_prev = tail, *pos_next = NULL;
                while (strcmp(container_of(node, element_t, list)->value,
                              container_of(pos_prev, element_t, list)->value) <
                       0) {
                    pos_next = pos_prev;
                    if (pos_prev == pending)
                        break;
                    pos_prev = pos_prev->prev;
                }
                if (pos_next == pos_prev) {
                    node->prev = pending->prev;
                    node->next = pending;
                    pending->prev = node;
                    pending = node;
                    tail_count++;
                } else {
                    pos_prev->next = node;
                    node->prev = pos_prev;
                    node->next = pos_next;
                    pos_next->prev = node;
                    tail_count++;
                }
            }
        }
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