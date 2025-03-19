#include "queue.h"

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/* if True select node_a */
#define cmp_xor_order(node_a, node_b, type, member, descend)    \
    (descend ^ (strcmp(list_entry(node_a, type, member)->value, \
                       list_entry(node_b, type, member)->value) <= 0))

struct list_head *merge(struct list_head *a, struct list_head *b, bool descend);

void merge_final(struct list_head *head,
                 struct list_head *a,
                 struct list_head *b,
                 bool descend);

void list_sort(struct list_head *head, bool descend);

void hybrid_sort(struct list_head *head, bool descend);

void qk_sort(struct list_head *head, bool descend);