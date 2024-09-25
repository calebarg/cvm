//
// linux_stuff.cpp
//
// Caleb Barger
// 09/24/24
//

internal OS_FileContents os_read_entire_file(Arena* arena, String8 path)
{
  OS_FileContents result{};
   
  Temp temp_mem = temp_begin(0, 0);
  int file_desc = 
      open((const char*)str8_catz(temp_mem.arena, path).str, O_CREAT|O_RDWR);
  temp_end(temp_mem);

  if (file_desc != -1)
  { 
    struct stat file_stat;
    if (fstat(file_desc, &file_stat) != -1)
    {
        result.len = file_stat.st_size;
    }
    result.data = (U8*)arena_push(arena, result.len);
    if (read(file_desc, result.data, result.len) != result.len)
		{
      InvalidPath;
		} 
    close(file_desc);
  }
  else
  {
     InvalidPath; // Failed to open file
  }

  return result;
}

internal void os_write_entire_file(OS_FileContents file_contents, String8 path)
{
  InvalidPath;
}

internal void os_get_cwd(String8 abs_path)
{
  InvalidPath;
}

internal void os_chdir(String8 path)
{
  InvalidPath;
}

internal void os_mkdir(String8 path)
{
  InvalidPath;
}

internal B8 os_dir_exists(String8 rel_path)
{
  InvalidPath;
  return 0;
}

internal B8 os_file_exists(String8 path)
{
  InvalidPath;
  return 0;
}
