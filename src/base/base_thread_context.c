/////////////////////////////////////////////////////////////////////////////////////////
//  COPYRIGHT (c) 2024 Schweitzer Engineering Laboratories, Inc.
//  SEL Confidential
/////////////////////////////////////////////////////////////////////////////////////////
/// @file base_thread_context.c
/// @author Caleb Barger 
/// @date March 27, 2024
/////////////////////////////////////////////////////////////////////////////////////////

// FIXME(caleb): Make this thread local!!
ThreadCTX* local_thread_ctx;

internal void thread_ctx_init(ThreadCTX* thread_ctx)
{
   if (thread_ctx == nil)
   {
      InvalidPath;
   }

   for (U8 arena_idx=0; arena_idx < ArrayCount(thread_ctx->arenas); ++arena_idx)
   {
      thread_ctx->arenas[arena_idx] = arena_alloc();
   }
   local_thread_ctx  = thread_ctx;
}

internal inline Temp temp_begin(Arena** conflicts, U8 count)
{
   Temp result{0, 0};

   if (local_thread_ctx == nil)
   {
      AssertMessage("No thread context initialized");
   }
  
   U8 scratch_idx = 0;   
   for (; scratch_idx < count; ++scratch_idx)
   {
      if (conflicts[scratch_idx] != local_thread_ctx->arenas[scratch_idx])
      {
         break;
      }
   }
   if (scratch_idx < ArrayCount(local_thread_ctx->arenas))
   {
      result = scratch_begin(local_thread_ctx->arenas[scratch_idx]);
   }

   return result;
}

internal inline void temp_end(Temp temp)
{
   scratch_end(temp);
}
