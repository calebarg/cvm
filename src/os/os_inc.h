struct OS_PathStack
{
   String8Node* stack;   
   String8Node* free_list;
};

struct OS_FileContents
{
   U8* data;
   U64 len;
};

internal void os_pushd(Arena* arena, OS_PathStack* path_stack, String8 rel_path);
internal void os_popd(OS_PathStack* path_stack);

// ^^^^  NON SPECIFIC OS CODE ^^^^
//
// VVVV  OS SPECIFIC CODE VVVV

internal OS_FileContents os_read_entire_file(Arena* arena, String8 path);
internal void os_write_entire_file(OS_FileContents file_contents, String8 path);
internal void os_get_cwd(String8 abs_path);
internal void os_chdir(String8 path);
internal void os_mkdir(String8 path);
internal B8 os_dir_exists(String8 rel_path);
internal B8 os_file_exists(String8 path);

#ifdef _MSC_VER
#include "Windows.h"
//#include "win32/win32_stuff.h"
#elif __GNUC__
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
//#include "linux/linux_stuff.h"
#endif
