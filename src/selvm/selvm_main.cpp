#include "Windows.h"
#include "base/base_inc.h"
#include "base/base_inc.cpp"
#include "os/os_inc.cpp"

struct ELF_Header
{
   U8 ident[16];
   U16 type;
   U16 machine;
   U32 version;
   U64 entry;
   U64 phoff;
   U64 shoff;
   U32 flags;
   U16 ehsize;
   U16 phentsize;
   U16 phnum;
   U16 shentsize;
   U16 shnum;
   U16 shstrndx;
};

struct ELF_SectionHeader
{
	U32 name;
	U32 type;
	U64 flags;
	U64 addr;
	U64 offset;
	U64 size;
	U32 link;
	U32 info;
	U64 addralign;
	U64 entsize;
};

int main(int argc, char* argv[])
{
   if (argc < 2)
   {
      return 1; 
   }

   ThreadCTX thread_ctx;
   thread_ctx_init(&thread_ctx);
   Arena* arena = arena_alloc();

   OS_FileContents elf_contents = 
      os_read_entire_file(arena, str8_from_mem((U8*)argv[1]));

   if (elf_contents.len > 0)
   {
      ELF_Header* elf_header = (ELF_Header*)elf_contents.data;
      ELF_SectionHeader* str_table_section_header = 
         (ELF_SectionHeader*)(&elf_contents.data[elf_header->shstrndx * elf_header->shentsize + elf_header->shoff]);
      if (str_table_section_header->type != 0x3)
      {
         InvalidPath; // Expected table type is 3.
      }

      U64 offset = elf_header->shoff;
      for (U16 section_idx=1; section_idx < elf_header->shnum; ++section_idx)
      {
         ELF_SectionHeader* section_header = 
            (ELF_SectionHeader*)(&elf_contents.data[offset]);

         String8 section_name = str8_from_mem(&elf_contents.data[str_table_section_header->offset + section_header->name]);
         if ((section_header->type == 0x1) && (str8_eql(section_name, Str8Lit(".text")))) // PROG_BITS (Actual machine code)
         {
            OS_FileContents machine_code = 
               OS_FileContents{&elf_contents.data[section_header->offset], section_header->size}; 
#if 1 
            U64 instruction_count = section_header->size / 4; 
            U32* instructions = (U32*)machine_code.data;
            for (U64 instruction_idx=0; instruction_idx < instruction_count; ++instruction_idx)
            {
               printf("%llx: %lx\n", instruction_idx*4, instructions[instruction_idx]);
            }
#else
            os_write_entire_file(machine_code, Str8Lit("machine_code.dump"));
#endif
            break;
         }

         offset += elf_header->shentsize;
      }
   } 
   else
   {
      InvalidPath; // Issue reading input file
   }

   return 0;
}
