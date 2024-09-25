//
// cvm_main.cpp
//
// Caleb Barger
// 09/24/24
//

#include "base/base_inc.h"
#include "os/os_inc.h"

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

  OS_FileContents machine_code{};
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
    for (U16 section_idx=0; section_idx < elf_header->shnum; ++section_idx)
    {
      ELF_SectionHeader* section_header =
      (ELF_SectionHeader*)(&elf_contents.data[offset]);

      String8 section_name = str8_from_mem(&elf_contents.data[str_table_section_header->offset + section_header->name]);
      if ((section_header->type == 0x1) && (str8_eql(section_name, Str8Lit(".text")))) // PROG_BITS (Actual machine code)
      {
        machine_code =
          OS_FileContents{&elf_contents.data[section_header->offset], section_header->size};
        break;
      }
      offset += elf_header->shentsize;
    }
  }
  else
  {
    InvalidPath; // Issue reading input file
  }

  // TODO(actually read the manual)
  U8* register_memory = (U8*)arena_push(arena, 32*8);

  U64 instruction_count = machine_code.len / 4;
  U32* instructions = (U32*)machine_code.data;
  for (U64 instruction_idx=0; instruction_idx < instruction_count; ++instruction_idx)
  {
    // DEBUG print the instruction
    printf("%llx: %lx\n", instruction_idx*4, instructions[instruction_idx]);
#if 0
    U8 most_signif_byte = ((U8*)(&instructions[instruction_idx]))[3];
    printf("%b  ", most_signif_byte);
    U8 next_most_signif_byte = ((U8*)(&instructions[instruction_idx]))[2];
    printf("%b\n", next_most_signif_byte);
#endif
    // Get the decode group
    U8 decode_group = (instructions[instruction_idx] >> 25) & 0xf;
    if (decode_group & 0x8) // Data processing immediate group
    {
      U8 instruction_class = (instructions[instruction_idx] >> 22) & 0xf;
      if (instruction_class & 0xa) // Move wide immediate instruction class
      {
        // This bit determines if this is a 32 or 64 version of this operation
        B8 sf = (instructions[instruction_idx] >> 31) & 0x1;
        U8 op_code = (instructions[instruction_idx] >> 29) & 0x3;
        switch (op_code)
        {
          case 0x2: // MOVZ
          {
            U16 immediate_val = (instructions[instruction_idx] >> 5) & 0xffff;
            U8 dest_reg = instructions[instruction_idx] & 0xf;

            if (sf)
            {
              InvalidPath;
              ((U64*)register_memory)[dest_reg] = immediate_val;
            }
            else
            {
              ((U32*)(&((U64*)register_memory)[dest_reg]))[1] = immediate_val;
            }
          } break;
          default: break;
        }
      }
      else
      {
        printf("Unhandled instruction class %b\n", instruction_class);
      }
    }
    else
    {
      printf("Unhandled decode group %b\n", decode_group);
    }
  }

  // DEBUG print registers
  for (U8 reg_idx=0; reg_idx < 16; ++reg_idx)
  {
    printf("X%u: %llu\tW%u: %lu\n", reg_idx, ((U64*)register_memory)[reg_idx], reg_idx, ((U32*)(&((U64*)register_memory)[reg_idx]))[1]);
  }

  return 0;
}
