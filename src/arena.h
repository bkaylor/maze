
typedef struct {
    unsigned char *buffer;
    size_t buffer_length;
    size_t offset;
} Arena;

void arena_init(Arena *arena, void *memory, size_t memory_length)
{
    arena->buffer = memory;
    arena->buffer_length = memory_length;
    arena->offset = 0;
}

void *arena_alloc(Arena *arena, size_t size)
{
    void *address_to_return = arena->buffer + arena->offset;
    arena->offset += size;

    if (arena->offset > arena->buffer_length)
    {
        return NULL;
    }

    memset(address_to_return, 0, size);
    return address_to_return;
}

/*
void arena_resize(Arena *arena, void *old_memory, size_t old_memory_length, void *new_memory_length)
{
}
*/

void arena_free_all(Arena *arena)
{
    arena->offset = 0;
}

void arena_debug(Arena *arena)
{
    printf("Arena size: %zd, Current offset: %zd\n", arena->buffer_length, arena->offset);
}
