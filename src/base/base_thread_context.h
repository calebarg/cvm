/////////////////////////////////////////////////////////////////////////////////////////
//  COPYRIGHT (c) 2024 Schweitzer Engineering Laboratories, Inc.
//  SEL Confidential
/////////////////////////////////////////////////////////////////////////////////////////
/// @file base_thread_context.h 
/// @author Caleb Barger 
/// @date March 27, 2024
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef BASE_THREAD_CONTEXT_H

typedef struct ThreadCTX ThreadCTX;
struct ThreadCTX
{
   Arena* arenas[2];
};

internal void thread_ctx_init(ThreadCTX* thread_ctx);
internal inline Temp temp_begin(Arena** conflicts, U8 count);
internal inline void temp_end(Temp temp);

#define BASE_THREAD_CONTEXT_H
#endif
