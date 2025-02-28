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

/* This macro in list.h cannot be detected by cppchecker */
#define list_for_each_entry_safe(entry, safe, head, member)            \
    for (entry = list_entry((head)->next, typeof(*entry), member),     \
        safe = list_entry(entry->member.next, typeof(*entry), member); \
         &entry->member != (head); entry = safe,                       \
        safe = list_entry(safe->member.next, typeof(*entry), member))

/* Free all storage used by queue */
void q_free(struct list_head *head)
{
    if (!head)
        return;
    if (!list_empty(head)) {
        element_t *entry, *safe;
        list_for_each_entry_safe (entry, safe, head, list)
            q_release_element(entry);
    }
    free(head);
}

element_t *q_new_element(char *s)
{
    element_t *e = malloc(sizeof(element_t));
    if (!e)
        return NULL;
    e->value = strdup(s);
    if (!e->value) {
        free(e);
        return NULL;
    }
    return e;
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *e = q_new_element(s);
    if (!e)
        return false;
    list_add(&e->list, head);
    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *e = q_new_element(s);
    if (!e)
        return false;
    list_add_tail(&e->list, head);
    return true;
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *e = list_entry(head->next, element_t, list);
    strncpy(sp, e->value, bufsize);
    sp[bufsize - 1] = '\0';
    list_del(head->next);
    return e;
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *e = list_entry(head->prev, element_t, list);
    strncpy(sp, e->value, bufsize);
    sp[bufsize - 1] = '\0';
    list_del(head->prev);
    return e;
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    if (!head || list_empty(head))
        return 0;
    size_t count = 0;
    struct list_head *node;
    list_for_each (node, head)
        count++;
    return count;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    if (!head || list_empty(head))
        return false;
    struct list_head *nt = head->next, *pv = head->prev;
    while (nt != pv && nt->prev != pv) {
        nt = nt->next;
        pv = pv->prev;
    }
    list_del(nt);
    q_release_element(list_entry(nt, element_t, list));
    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    if (!head || list_empty(head))
        return false;
    element_t *entry, *safe, *del;
    list_for_each_entry_safe (entry, safe, head, list) {
        while (&safe->list != head && !strcmp(entry->value, safe->value)) {
            del = safe;
            safe = list_entry(safe->list.next, element_t, list);
            list_del(&del->list);
            q_release_element(del);
        }
    }
    return true;
}

void q_connect(struct list_head *a, struct list_head *b)
{
    a->next = b;
    b->prev = a;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    struct list_head *odd = head->next, *even = head->next->next;
    while (odd != head && even != head) {
        q_connect(odd->prev, even);
        q_connect(odd, even->next);
        q_connect(even, odd);
        odd = odd->next;
        even = odd->next;
    }
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    struct list_head *tail = head->prev, *last = head;
    head->next->prev = NULL;
    while (tail->prev) {
        last->next = tail;
        tail = tail->prev;
        last->next->prev = last;
        last = last->next;
    }
    q_connect(last, tail);
    q_connect(tail, head);
}

/* Reverse the nodes of the list k at a time */
void q_reverseK(struct list_head *head, int k)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    struct list_head *last = head, *r_head = head->next, *r_tail = head->next;
    size_t count = 1;
    LIST_HEAD(head2);
    INIT_LIST_HEAD(&head2);
    while (r_head != head) {
        while (r_tail != head && count < k) {
            r_tail = r_tail->next;
            count++;
        }
        if (count < k)
            break;
        r_tail = r_tail->next;
        q_connect(&head2, r_head);
        q_connect(r_tail->prev, &head2);
        q_reverse(&head2);
        q_connect(last, (&head2)->next);
        q_connect((&head2)->prev, r_tail);
        last = r_tail->prev;
        r_head = r_tail;
        count = 1;
    }
}

/* Merge 2 lists to head1 and init head2 */
void q_merge_2list(struct list_head *head1,
                   struct list_head *head2,
                   bool descend)
{
    struct list_head *node, *list1 = head1->next, *list2 = head2->next;
    head1->prev->next = NULL;
    head2->next->prev = NULL;
    LIST_HEAD(tmp);
    INIT_LIST_HEAD(&tmp);
    while (list1 && list2) {
        if (descend ^ (strcmp(list_entry(list1, element_t, list)->value,
                              list_entry(list2, element_t, list)->value) > 0)) {
            node = list2;
            list2 = list2->next;
        } else {
            node = list1;
            list1 = list1->next;
        }
        list_add_tail(node, &tmp);
    }
    if (list1) {
        q_connect((&tmp)->prev, list1);
        q_connect(head1->prev, &tmp);
    } else if (list2) {
        q_connect((&tmp)->prev, list2);
        q_connect(head2->prev, &tmp);
    }
    INIT_LIST_HEAD(head1);
    INIT_LIST_HEAD(head2);
    list_splice_tail(&tmp, head1);
}

/* Merge sort for a list at least two nodes */
void q_mergesort(struct list_head *head, bool descend)
{
    if (list_is_singular(head))
        return;
    // Divide
    struct list_head *nt = head->next, *pv = head->prev;
    while (nt != pv && nt->prev != pv) {
        nt = nt->next;
        pv = pv->prev;
    }

    LIST_HEAD(head2);
    INIT_LIST_HEAD(&head2);
    q_connect(&head2, pv->next);
    q_connect(head->prev, &head2);
    q_connect(pv, head);

    q_mergesort(head, descend);
    q_mergesort(&head2, descend);

    // Conquer
    q_merge_2list(head, &head2, descend);
}

/* Sort elements of queue in ascending/descending order */
void q_sort(struct list_head *head, bool descend)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    q_mergesort(head, descend);
}

/* Remove every node which has a node with a strictly less value anywhere to
 * the right side of it */
int q_ascend(struct list_head *head)
{
    if (!head || list_empty(head))
        return 0;
    if (list_is_singular(head))
        return 1;
    char const *bound = list_last_entry(head, element_t, list)->value;
    struct list_head *node = head->prev->prev;
    element_t *e;
    size_t count = 1;
    while (node != head) {
        e = list_entry(node, element_t, list);
        node = node->prev;
        if (strcmp(e->value, bound) > 0) {
            list_del(&e->list);
            q_release_element(e);
        } else {
            count++;
            bound = e->value;
        }
    }
    return count;
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    if (!head || list_empty(head))
        return 0;
    if (list_is_singular(head))
        return 1;
    char const *bound = list_last_entry(head, element_t, list)->value;
    struct list_head *node = head->prev->prev;
    element_t *e;
    size_t count = 1;
    while (node != head) {
        e = list_entry(node, element_t, list);
        node = node->prev;
        if (strcmp(e->value, bound) < 0) {
            list_del(&e->list);
            q_release_element(e);
        } else {
            count++;
            bound = e->value;
        }
    }
    return count;
}

/* Merge all the queues into one sorted queue, which is in ascending/descending
 * order */
int q_merge(struct list_head *head, bool descend)
{
    if (!head || list_empty(head))
        return 0;
    if (list_is_singular(head))
        return q_size(list_first_entry(head, queue_contex_t, chain)->q);

    struct list_head *l = list_first_entry(head, queue_contex_t, chain)->q;
    queue_contex_t *qt = list_last_entry(head, queue_contex_t, chain);
    while (qt->q != l) {
        q_merge_2list(l, qt->q, descend);
        qt->q = NULL;
        list_del_init(&qt->chain);
        qt = list_last_entry(head, queue_contex_t, chain);
    }
    return q_size(l);
}
