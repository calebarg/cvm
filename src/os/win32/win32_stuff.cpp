//
// win32_stuff.cpp
//
// Caleb Barger
// 09/24/24
//

internal OS_FileContents os_read_entire_file(Arena* arena, String8 path)
{
   OS_FileContents result{};
   
   Temp temp_mem = temp_begin(0, 0);
   HANDLE file_handle = CreateFileA(
     (LPCSTR)str8_catz(temp_mem.arena, path).str,
     GENERIC_READ,
     0,
     0,
     OPEN_EXISTING,
     FILE_ATTRIBUTE_NORMAL,
     0 );
   temp_end(temp_mem);

   if (file_handle)
 	{ 
      result.len = GetFileSize(file_handle, 0);
      result.data = (U8*)arena_push(arena, result.len);
		if (!ReadFile(file_handle, result.data, result.len, 0, 0))
		{
			InvalidPath;
		} 
      CloseHandle(file_handle);
   }
   else
   {
      InvalidPath;
   }

   return result;
}

internal void os_write_entire_file(OS_FileContents file_contents, String8 path)
{
   Temp temp_mem = temp_begin(0, 0);
   HANDLE file_handle = CreateFileA(
     (LPCSTR)str8_catz(temp_mem.arena, path).str,
     GENERIC_WRITE,
     0,
     0,
     CREATE_ALWAYS,
     FILE_ATTRIBUTE_NORMAL,
     0 );
   temp_end(temp_mem);

   if (file_handle)
 	{ 
		if (!WriteFile(file_handle, file_contents.data, file_contents.len, 0, 0))
		{
			InvalidPath;
		}
      CloseHandle(file_handle);
   }
   else
   {
      InvalidPath;
   }
}

internal void os_get_cwd(String8 abs_path)
{
   // Win32 
   if (GetCurrentDirectory(abs_path.len, (LPSTR)abs_path.str) > abs_path.len)
   {
      InvalidPath;
   }
}

internal void os_chdir(String8 path)
{
   Temp temp_mem{ temp_begin(0, 0) };
   String8 pathz = str8_catz(temp_mem.arena, path);

   if (!SetCurrentDirectory((LPSTR)pathz.str))
   {
      InvalidPath;
   }
   temp_end(temp_mem);
}

internal void os_mkdir(String8 path)
{
   Temp temp_mem{ temp_begin(0, 0) };
   if (!CreateDirectory((LPCSTR)str8_catz(temp_mem.arena, path).str, 0))
   {
      InvalidPath;
   }
   temp_end(temp_mem);
}

internal B8 os_dir_exists(String8 rel_path)
{
   B8 result{ 0 };

   Temp temp_mem{ temp_begin(0, 0) };
   String8 rel_pathz = str8_catz(temp_mem.arena, rel_path);

   U32 file_attributes{ GetFileAttributes((LPCSTR)rel_pathz.str) };
   if (file_attributes != INVALID_FILE_ATTRIBUTES && (file_attributes & FILE_ATTRIBUTE_DIRECTORY))
   {
      result = 1;
   }
   temp_end(temp_mem);

   return result;
}

internal B8 os_file_exists(String8 path)
{
   B8 result{ 0 };

   Temp temp_mem{ temp_begin(0, 0) };

   String8 pathz = str8_catz(temp_mem.arena, path);
   U32 file_attributes = GetFileAttributes((LPCSTR)pathz.str);
   if (file_attributes != INVALID_FILE_ATTRIBUTES)
   {
      result = 1;
   }

   temp_end(temp_mem);

   return result;
}
