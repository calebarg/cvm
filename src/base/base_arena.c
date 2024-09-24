//
// base_arena.c
//
// Caleb Barger
// 02/11/24
//

internal void* arena_push(Arena* a, U64 size)
{
   void* result;

   U64 aligned_size = AlignPow2(size, 8);
   if (a->offset + aligned_size > a->cap)
   {
      if (a->growable)
      {
         InvalidPath; //- TODO(cabarger): Grow arena
      }
      else
      {
         AssertMessage("OUT OF SPACE");
      }
   }
   result = a->base_ptr + a->offset;
   a->offset += aligned_size;

   return result;
}

internal Arena* arena_alloc_sized(U64 size)
{
   U64 aligned_size = AlignPow2(size + sizeof(Arena), 8);
   void* memory     = malloc(aligned_size);

   Arena* arena    = (Arena*)memory;
   arena->base_ptr = (U8*)memory;
   arena->offset   = sizeof(Arena);
   arena->cap      = aligned_size;
   arena->growable = 0;

   return arena;
}

internal Arena* arena_alloc()
{
   return arena_alloc_sized(MB(100));
};

internal Arena* arena_sub(Arena* parent_arena, U64 size)
{
   U64 aligned_size = AlignPow2(size + sizeof(Arena), 8);
   void* memory     = arena_push(parent_arena, aligned_size);

   Arena* arena    = (Arena*)memory;
   arena->base_ptr = (U8*)memory;
   arena->offset   = sizeof(Arena);
   arena->cap      = aligned_size;
   arena->growable = 0;

   return arena;
}

internal void arena_release(Arena* arena)
{
   free(arena->base_ptr);
}

internal Temp scratch_begin(Arena* arena)
{
   Temp result;
   result.arena  = arena;
   result.offset = arena->offset;
   return result;
}

internal void scratch_end(Temp temp)
{
   temp.arena->offset = temp.offset;
}
