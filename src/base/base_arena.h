//
// base_arena.h
//
// Caleb Barger
// 02/11/24
//

#ifndef BASE_ARENA_H

struct Arena
{
   U8* base_ptr;
   U64 offset;
   U64 cap;
   B8 growable;
};

struct Temp
{
   Arena* arena;
   U64 offset;
};

internal void* arena_push(Arena* arena, U64 size);
internal Arena* arena_alloc_sized(U64 size);
internal Arena* arena_alloc();
internal Arena* arena_sub(Arena* arnea, U64 size);
internal void arena_release(Arena* arena);
internal Temp scratch_begin(Arena* arena);
internal void scratch_end(Temp temp);

#define BASE_ARENA_H
#endif
