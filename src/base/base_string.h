/////////////////////////////////////////////////////////////////////////////////////////
//  COPYRIGHT (c) 2024 Schweitzer Engineering Laboratories, Inc.
//  SEL Confidential
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef BASE_STRING_H

typedef struct String8 String8;
struct String8
{
   U8* str;
   U64 len;
};

typedef struct String8Node String8Node;
struct String8Node
{
   String8Node* next;
   String8 val;
};

typedef struct String8List String8List;
struct String8List
{
   String8Node* head;
   String8Node* tail;
};

#define Str8Lit(a)          \
   {                        \
      (U8*)a, sizeof(a) - 1 \
   }
internal String8 str8_cat(Arena* arena, String8 a, String8 b);
internal String8 str8_from_mem(U8* str);
internal String8 str8_sub(String8 str, U64 start, U64 end);
internal String8 str8_sub(U8* str, U64 start, U64 end);
internal String8 str8_copy(Arena* arena, String8 source);
internal B8 str8_eql(String8 a, String8 b);

internal void DEBUG_print_str8(String8 str);

internal void str8_list_push(Arena* arena, String8List* list, String8 source_str);

#define BASE_STRING_H
#endif
