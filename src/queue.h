
typedef struct {
    int *items;
    int item_count;
    int max_items;
} Queue;

void queue_init(Queue *q, int max)
{
    q->items = malloc(sizeof(int) * max);
    q->item_count = 0;
    q->max_items = max;
}

bool queue_enqueue(Queue *q, int value)
{
    if (q->item_count + 1 > q->max_items) return false;

    int i = q->item_count;
    while (i >= 0)
    {
        q->items[i+1] = q->items[i];
        i -= 1;
    }

    q->items[0] = value;
    q->item_count += 1;

    return true;
}

int queue_dequeue(Queue *q)
{
    if (q->item_count < 1) {
        return -1;
    }

    int value_to_return = q->items[q->item_count-1];
    q->item_count -= 1;

    return value_to_return;
}

void queue_free(Queue *q)
{
    free(q->items);
    q->item_count = 0;
    q->max_items = 0;
}
