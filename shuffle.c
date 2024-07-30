#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "random.h"
#include "shuffle.h"

uint64_t qrandom()
{
    uint64_t tmp;
    randombytes((uint8_t *) &tmp, sizeof(tmp));
    return tmp;
}

void q_shuffle(struct list_head *head)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    srand(time(NULL));
    int len = q_size(head);
    struct list_head **entries = malloc(sizeof(*entries) * len);
    struct list_head *node;
    int i = 0;
    list_for_each (node, head)
        entries[i++] = node;
    for (i = len - 1; i > 0; i--) {
        int n = qrandom() % (i + 1);
        struct list_head *tmp = entries[i];
        entries[i] = entries[n];
        entries[n] = tmp;
    }
    INIT_LIST_HEAD(head);
    for (i = 0; i < len; i++)
        list_add_tail(entries[i], head);
    free(entries);
}