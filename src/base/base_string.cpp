//
// base_string.cpp
//
// Caleb Barger
// 02/11/24
//

internal String8 str8_cat(Arena* arena, String8 a, String8 b)
{
   String8 result;
   result.str = (U8*)arena_push(arena, a.len + b.len);
   result.len = 0;

   for (U64 a_index = 0; a_index < a.len; ++a_index) result.str[result.len++] = a.str[a_index];
   for (U64 b_index = 0; b_index < b.len; ++b_index) result.str[result.len++] = b.str[b_index];

   return result;
}

internal String8 str8_catz(Arena* arena, String8 str)
{
   String8 result;
   result = str8_cat(arena, str, Str8Lit("\0"));
   return result;
}

internal String8 str8_from_mem(U8* str)
{
   String8 result;
   U64 s_len = 0;
   {
      U8* curr_ptr = str;
      for (; *curr_ptr != '\0'; ++curr_ptr)
      {
      }
      s_len = (U64)(curr_ptr - str);
   }
   result.str = str;
   result.len = s_len;
   return result;
}

internal String8 str8_sub(String8 str, U64 start, U64 end)
{
   String8 result;
   result.str = (U8*)str.str + start;
   result.len = end - start;
   return result;
}

internal String8 str8_sub(U8* str, U64 start, U64 end)
{
   String8 result;
   result.str = (U8*)str + start;
   result.len = end - start;
   return result;
}

internal String8 str8_copy(Arena* arena, String8 source)
{
   String8 dest = {
      (U8*)arena_push(arena, source.len),
      source.len,
   };
   for (U64 source_index = 0; source_index < source.len; ++source_index)
      dest.str[source_index] = source.str[source_index];
   return dest;
}

internal B8 str8_eql(String8 a, String8 b)
{
   B8 result = 0;
   if (a.len == b.len)
   {
      result = (memcmp(a.str, b.str, a.len) == 0);
   }
   return result;
}


internal void str8_list_push(Arena* arena, String8List* list, String8 source_str)
{
   String8Node* new_node{ reinterpret_cast<String8Node*>(arena_push(arena, sizeof(String8Node))) };
   new_node->next = nil;
   new_node->val  = str8_copy(arena, source_str);

   SLLQueuePush(list->head, list->tail, new_node);
}

internal void DEBUG_print_str8(String8 str)
{
   Temp temp_mem = temp_begin(0, 0);
   printf("%s\n", str8_catz(temp_mem.arena, str).str);
   temp_end(temp_mem);
}
