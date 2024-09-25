//
// os_inc.cpp
//
// Caleb Barger
// 09/24/24
//

internal void os_pushd(Arena* arena, OS_PathStack* path_stack, String8 rel_path)
{
   String8Node* cwd_node;
   if (path_stack->free_list != 0)
   {
      cwd_node = path_stack->free_list;
      path_stack->free_list = path_stack->free_list->next;
   }
   else
   {
      cwd_node = (String8Node*)arena_push(arena, sizeof(String8Node));
      cwd_node->val = String8{(U8*)arena_push(arena, 256), 256}; 
   }
   cwd_node->next = 0;

   os_get_cwd(cwd_node->val);
   SLLStackPush(path_stack->stack, cwd_node);

   os_chdir(rel_path);
}

internal void os_popd(OS_PathStack* path_stack)
{
   String8Node* last_dir = path_stack->stack;
   os_chdir(last_dir->val);
   
   path_stack->stack = path_stack->stack->next;
   last_dir->next = 0;
   SLLStackPush(path_stack->free_list, last_dir);
}


#ifdef WIN32
#include "win32/win32_stuff.cpp"
#elif __GNUC__
#include "linux/linux_stuff.cpp"
#endif
