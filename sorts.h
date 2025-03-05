#include "queue.h"

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

struct list_head *merge(struct list_head *a, struct list_head *b, bool descend);

void merge_final(struct list_head *head,
                 struct list_head *a,
                 struct list_head *b,
                 bool descend);

void list_sort(struct list_head *head, bool descend);



