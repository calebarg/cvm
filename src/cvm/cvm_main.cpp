//
// cvm_main.cpp
//
// Caleb Barger
// 09/24/24
//

#include "base/base_inc.h"
#include "os/os_inc.h"

// TODO(calebarg): Handle MSVC compilation
// Either gernate a compile error here or
// Create wrappers for the linux specific stuff.
#include <sys/time.h>
#include <signal.h>
#include <termios.h>

#include "base/base_inc.cpp"
#include "os/os_inc.cpp"

typedef U8 Instr;
enum
{
////////////////////////////////////////////////////////////////
//~ Data processing immediate

  // Add subtract immediate

  Instr_sub_imm = 0,
  Instr_subs_imm,

  // Move wide immediate

  Instr_movz,

////////////////////////////////////////////////////////////////
//~ Branches exception and system

  // Conditional branch (immediate)

  Instr_b_cond,

  // Unconditional branch register

  Instr_ret,

  // Unconditional branch (immediate)

  Instr_b,
  Instr_bl,

  // Compare and branch (immediate)

  Instr_cbz,
  Instr_cbnz,

////////////////////////////////////////////////////////////////
//~ Loads and stores

  // Load register (literal)

  Instr_lr_ldr,
   
  // Load/Store register (register offset)

  Instr_lsr_strb,
  Instr_lsr_str,
  Instr_lsr_ldrb,
  Instr_lsr_ldr,

//////////////////////////////////////////////////////////////////
//~ Data processing register

  // Logical (shifted register)
 
  Instr_logical_mov,

  // Add/Subtract (shifted register)

  Instr_as_add,
  Instr_as_sub,

  // Data-processing (3 source)

  Instr_madd,
};

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

typedef U8 ShiftType;
enum
{
  ShiftType_lsl = 0,
  ShiftType_lsr,
  ShiftType_asr
};

typedef U8 BranchType;
enum
{
  BranchType_dir = 0,
  BranchType_dir_call,
  BranchType_ret
};

typedef U8 El;
enum
{
  El_0 = 0, // Application level
  El_1, // OS level "privleged"
  El_2, // Hypervisor (Not implemented)
  El_3, // Monitor (Not implemented)
};

struct PSTATE
{
  U64 n: 1;
  U64 z: 1;
  U64 c: 1;
  U64 v: 1;

  U64 pad0: 3;
  U64 el: 2;

  U64 pad1: 55;
};

inline U64* gp_reg_u64(U8* gp_regs, U8 reg_idx)
{
  U64* gp_regs_ptrx = (U64*)gp_regs;
  return (gp_regs_ptrx + reg_idx);
}

inline U32* gp_reg_u32(U8* gp_regs, U8 reg_idx)
{
  U64* gp_regs_ptrx = (U64*)gp_regs;
  return (U32*)(gp_regs_ptrx + reg_idx);
}

// TODO(calebarg): Join these sign extend functions together.

inline U64 se_u32_to_u64(U32 val)
{
  U32 result = val;
  if (val & 0x80000000)
  {
    result |= 0xffffffff00000000;
  }
  return result;
}

inline U32 se_imm19_to_u32(U32 imm19)
{
  U32 result = imm19;
  if (imm19 & 0x40000)
  {
    result |= 0xfff80000;
  }
  return result;
}

// Just need the 32 bit varient of this.

inline U64 se_imm26_to_u64(U32 imm26)
{
  U64 result = 0;
  result |= imm26;
  if (imm26 & 0x2000000)
  {
    result |= 0xfffffffffc000000;
  }
  return result;
}

// FIXME(calebarg): I can probably just loose these two funcs.
// Or at least just have a single imm12 to 32 bit func 

inline U64 ze_imm12_to_u64(U16 imm12)
{
  U64 result = 0;
  result |= imm12;
  return result;
}

inline U32 ze_imm12_to_u32(U16 imm12)
{
  U32 result = 0;
  result |= imm12;
  return result;
}

// TODO(calebarg): After finishing the decode/execute instruction
// refactor hand add_with_carry a bit size either 32 or 64 
// And roll these two functions into one.

internal U32 add_with_carry_u32(
  PSTATE* pstate,
  U32 lhs,
  U32 rhs,
  U8 carry_in)
{
  U32 unsigned_sum = lhs + rhs + (U32)(carry_in);
  S32 signed_sum = (S32)(lhs) + (S32)(rhs) + (S32)(carry_in);

  U32 result = unsigned_sum;

  // N flag - negative
  (result & 0x80000000) ? (pstate->n = 1) : (pstate->n = 0);

  // Z flag - zero
  (result == 0) ? (pstate->z = 1) : (pstate->z = 0);

  // C flag - carry
  ((U32)(result) == unsigned_sum) ? (pstate->c = 0) : (pstate->c = 1);

  // V flag - signed overflow
  ((S32)(result) == signed_sum) ?  (pstate->v = 0) : (pstate->v = 1);

  return result;
}
internal U64 add_with_carry_u64(
  PSTATE* pstate,
  U64 lhs,
  U64 rhs,
  U8 carry_in)
{
  U64 unsigned_sum = lhs + rhs + (U64)(carry_in);
  S64 signed_sum = (S64)(lhs) + (S64)(rhs) + (S64)(carry_in);

  U64 result = unsigned_sum;

  // N flag - negative
  (result & 0x8000000000000000) ? (pstate->n = 1) : (pstate->n = 0);

  // Z flag - zero
  (result == 0) ? (pstate->z = 1) : (pstate->z = 0);

  // C flag - carry
  ((U32)(result) == unsigned_sum) ? (pstate->c = 0) : (pstate->c = 1);

  // V flag - signed overflow
  ((S32)(result) == signed_sum) ?  (pstate->v = 0) : (pstate->v = 1);
  return result;
}

internal U32 shift_reg_u32(U32 val, ShiftType shift_type, U8 shift_amount)
{
  U32 result = val;

  switch (shift_type)
  {
    case ShiftType_lsl:
    {
      result = result << shift_amount;
    } break;
    case ShiftType_lsr:
    {
      result = result >> shift_amount;
    } break;
    case ShiftType_asr:
    {
      U32 result_copy = result;
      result = result >> shift_amount;
      result |= (result_copy & 0x80000000);
    } break;
    default: InvalidPath;
  }

  return result;
}

internal U64 shift_reg_u64(U64 val, ShiftType shift_type, U8 shift_amount)
{
  U64 result = val;

  switch (shift_type)
  {
    case ShiftType_lsl:
    {
      result = result << shift_amount;
    } break;
    case ShiftType_lsr:
    {
      result = result >> shift_amount;
    } break;
    case ShiftType_asr:
    {
      U64 result_copy = result;
      result = result >> shift_amount;
      result |= (result_copy & 0x8000000000000000);
    } break;
    default: InvalidPath;
  }

  return result;
}

El s1_translation_regime(El exception_level)
{
  if (exception_level != El_0)
  {
    return exception_level;
  }
  return El_1;

  // I don't implement the other exception levels
}

// HACK(Caleb): Fix this at some point.
U8 effective_tbi(U64 address, B8 is_instr, El el)
{
  return 0;
}

// Return the MSB number of a virtual address in the stage 1 translation regime
// for "el".
U64 addr_top(U64 address, B8 is_instr, El el)
{
  El regime = s1_translation_regime(el);
  if (effective_tbi(address, is_instr, regime) == 1)
  {
    return 55;
  }
  return 63;
}

U64 branch_addr(U64 vaddress, El el)
{
  U64 msb = addr_top(vaddress, 1, el);
  if (msb == 63)
  {
    return vaddress;
  }
  InvalidPath;
  return 69;
}

// FIXME(calebarg): No reason to pass pstate as a pointer here.
// (In fact don't need to pass it at all atm).
internal void branch_to(
    U64* pc,
    PSTATE* pstate,
    U64 target,
    BranchType branch_type,
    B8 branch_conditional)
{
  U64 target_vaddress = branch_addr(target, (El)pstate->el);
  *pc = target_vaddress;
}

internal B8 condition_holds(U8 cond, PSTATE pstate)
{
  B8 result = 0;
  switch(cond >> 1) {
    case 0x0: // EQ or NE
    {
      result = (B8)(pstate.z == 1);
    } break;
    case 0x5: // GE or LT
    {
      result = (B8)(pstate.n == pstate.v);
    } break;
    case 0x6: // GT or LE
    {
      result = (B8)((pstate.n == pstate.v) && (pstate.z == 0));
    } break;
    default: InvalidPath;
  }
  // Condition flag values in the set '111x' indicate always true
  // Otherwise, invert condition if necessary.
  if ((cond & 0x1) && (cond != 0xf))
  {
    result = !result;
  }

  return result;
}

F64 read_wall_clock()
{
  struct timeval tv;
  gettimeofday(&tv, 0);

  return (F64)tv.tv_sec + (F64)tv.tv_usec * .000001;
}

B8 running = 1;

internal void quit_on_ctrlc(int)
{
  running = 0;
}

typedef struct DecodedInstruction DecodedInstruction;
struct DecodedInstruction
{
  // The idea was/is to save time during the decode phase.
  // Encoded instructions are 4 bytes. I'm not going to be able
  // To beat that.
  // (Bitfields are an option here as well)

  // This struct is 8 bytes at this point.

  Instr id;
  B8 varient_64;
  union
  {
#pragma pack(push, 1)  // NOTE(calebarg): Don't let compiler pad these structs 
    struct 
    {
      U16 imm12;
      U8 rn;
      U8 rd;
    } add_sub_imm;
    struct
    {
      U16 imm16;
      U8 rd;
    } move_wide_imm;
    struct
    {
      U32 imm19; 
      U8 cond;
    } cond_branch_imm;
    struct
    {
      U8 rn;
    } uncond_branch_reg;
    struct
    {
      U32 imm26;
    } uncond_branch_imm;
    struct
    {
      U32 imm19;
      U8 rt;
    } comp_and_branch_imm;
    struct 
    {
      U32 imm19;
      U8 rt;
    } load_reg;
    struct 
    {
      U8 rt;
      U8 rn;
    } load_store_reg;
    struct 
    {
      U8 rm;
      U8 rn;
      U8 rd;
      U8 imm6;
    } logical;
    struct 
    {
      U8 shift;
      U8 rm;
      U8 imm6;
      U8 rn;
      U8 rd;
    } add_sub_shifted;
    struct 
    {
      U8 rm;
      U8 ra;
      U8 rn;
      U8 rd;
    } data_proc;
#pragma pack(pop)
  };
  B8 _pad;
};

internal DecodedInstruction decode_instruction(U32 instruction)
{
  DecodedInstruction result{};

  // NOTE(calebarg): I don't want to store this value on this
  // struct.

  U8 decode_group = (instruction >> 25) & 0xf;
  if ((decode_group & 0x8) && // Data processing -- immediate
      !(decode_group & 0x6))
  {
    U8 op0 = (instruction >> 22) & 0xf;
    if (!(op0 & 0x8) && (op0 & 4) && !(op0 & 2)) // Add/Subtract (immediate)
    {
      result.varient_64 = (B8)((instruction >> 31) & 0x1); 
      U8 op = (instruction >> 30) & 0x1;
      U8 s = (instruction >> 29) & 0x1;

      if (op) // SUB/SUBS (CMP alias)
      {
        result.id = (s) ? Instr_subs_imm : Instr_sub_imm;
        result.add_sub_imm.imm12 = (instruction >> 10) & 0xfff;
        result.add_sub_imm.rn = (instruction >> 5) & 0x1f;
        result.add_sub_imm.rd = instruction & 0x1f;
        if (result.add_sub_imm.rn == 31)
        {
          InvalidPath; // Not implemented
        }
      }
      else
      {
        InvalidPath; // Not implemented
      }
    }
    else if ((op0 & 0xa) && !(op0 & 0x4)) // Move wide (immediate)
    {
      result.varient_64 = (B8)((instruction >> 31) & 0x1); 
      U8 opc = (instruction >> 29) & 0x3;
      if ((opc & 0x2) && !(opc & 0x1)) // MOVZ
      {
        result.id = Instr_movz;
        result.move_wide_imm.imm16 = (instruction >> 5) & 0xffff;
        result.move_wide_imm.rd = instruction & 0xf;
      }
      else
      {
        InvalidPath;
      }
    }
    else
    {
      InvalidPath;
    }
  }
  else if ((decode_group & 0xa) && // Branches, exception generating and system instructions
          !(decode_group & 0x4))
  {
    U8 op0 = (instruction >> 29) & 0x7;
    U16 op1 = (instruction >> 12) & 0x3fff;
    if (!(op0 & 0x4) &&  // Conditional branch (immediate)
        (op0 & 0x2) &&
        !(op0 & 0x1) &&
        !(op1 & 0x2000))
    {
      U8 o1 = (instruction >> 24) & 0x1;
      U8 o0 = (instruction >> 4) & 0x1;
      if (!o1 && !o0) // B.cond
      {
        result.id =
          Instr_b_cond;
        result.cond_branch_imm.imm19 = (instruction >> 5) & 0x7ffff;
        result.cond_branch_imm.cond = instruction & 0xf;
      }
      else
      {
        InvalidPath;
      }
    }
    else if ((op0 & 0x4) && (op0 & 0x2) && !(op0 & 0x1) && (op1 & 0x2000)) // Unconditional branch (register)
    {
      U8 opc = (instruction >> 21) & 0xf;
      U8 op2 = (instruction >> 16) & 0x1f;
      U8 op3 = (instruction >> 10) & 0x3f;
      U8 op4 = instruction & 0x1f;
      if ((opc & 0x2) && (op2 & 0x1f) && !op3 && !op4) // RET
      {
        result.id = Instr_ret;
        result.uncond_branch_reg.rn = (instruction >> 5) & 0x1f;
      }
      else
      {
        InvalidPath;
      }
    }
    else if (!(op0 & 0x1) && !(op0 & 0x2)) // Unconditional branch (immediate) B/BL
    {
      B8 op = (B8)((instruction >> 31) & 0x1);
      result.id = (op) ? Instr_bl : Instr_b;
      result.uncond_branch_imm.imm26 = instruction & 0x3ffffff;
    }
    else if ((op0 & 0x1) && (~op1 & 0x2000)) // Compare and branch (immediate)
    {
      result.varient_64 = (B8)((instruction >> 31) & 0x1); 
      U8 op = (instruction >> 24) & 0x1;
      result.id = (op) ? Instr_cbnz : Instr_cbz;
      result.comp_and_branch_imm.imm19 = (instruction >> 5) & 0x7ffff;
      result.comp_and_branch_imm.rt = instruction & 0x1f;
    }
    else
    {
      InvalidPath;
    }
  }
  else if ((decode_group & 0x4) &&  // Loads and Stores
          !(decode_group & 0x1))
  {
    U8 op0 = (instruction >> 28) & 0xf;
    U8 op1 = (instruction >> 26) & 0x1;

    if ((op0 & 0x1) && !(op0 & 0x2)) // Load register (literal)
    {
      U8 opc = (instruction >> 30) & 0x3;
      U8 vr = (instruction >> 26) & 0x1;

      switch (opc)
      {
        case 0x1: // LDR
        {
          result.id = Instr_lr_ldr;
          result.load_reg.imm19 = (instruction >> 5) & 0x7ffff;
          result.load_reg.rt = instruction & 0x1f;
        } break;
        default: InvalidPath;
      }
    }
    else if ((op0 & 0x1) && (op0 & 0x2)) // Load/Store register (register offset)
    {
      U8 size = (instruction >> 30) & 0x3;
      U8 vr = (instruction >> 26) & 0x1;
      U8 opc = (instruction >> 22) & 0x3;
      switch (opc)
      {
        case 0x0: // STR/STRB
        {
          result.load_store_reg.rt = instruction & 0x1f;
          result.load_store_reg.rn = (instruction >> 5) & 0x1f;
          U8 option = (instruction >> 13) & 0x7;
          if (option != 0x0)
          {
            InvalidPath; // Unxexpected option value
          }
          switch (size)
          {
            case 0x0: // STRB
            {
              result.id = Instr_lsr_strb;
            } break;
            case 0x3: // STR
            {
              result.id = Instr_lsr_str;
            } break;
            default: InvalidPath;
          }
        } break;
        case 0x1: // LDR/LDRB
        {
          result.load_store_reg.rt = instruction & 0x1f;
          result.load_store_reg.rn = (instruction >> 5) & 0x1f;
          U8 option = (instruction >> 13) & 0x7;
          if (option != 0x0)
          {
            InvalidPath; // Unxexpected option value
          }
          switch (size)
          {
            case 0x0: // LDRB
            {
              result.id = Instr_lsr_ldrb;
            } break;
            case 0x3: // LDR
            {
              result.id = Instr_lsr_ldr;
            } break;
            default: InvalidPath;
          }
        } break;
        default: InvalidPath;
      }
    }
    else
    {
      InvalidPath; // Unhandled decode group
    }
  }
  else if ((decode_group & 0x5) && // Data processing -- register
          !(decode_group & 0x2))
  {
    U8 op0 = (instruction >> 30) & 0x1;
    U8 op1 = (instruction >> 28) & 0x1;
    U8 op2 = (instruction >> 21) & 0xf;
    if (!op1 && !(op2 & 0x8)) // Logical (shifted register)
    {
      result.varient_64 = (B8)((instruction >> 31) & 0x1); 
      U8 opc = (instruction >> 29) & 0x3;
      U8 shift = (instruction >> 22) & 0x3;
      U8 n = (instruction >> 21) & 0x1;
      result.logical.rm = (instruction >> 16) & 0x1f;
      result.logical.imm6 = (instruction >> 10) & 0x1f;
      result.logical.rn = (instruction >> 5) & 0x1f;
      result.logical.rd = instruction & 0x1f;

      switch (opc)
      {
        case 0x1: // ORR/ORN (MOV alias)
        {
          if ((shift != 0) || (result.logical.imm6 != 0) || (n != 0))
          {
            InvalidPath; // Not a MOV alias
          }
          result.id = Instr_logical_mov; 
        } break;
        default: InvalidPath;
      }
    }
    else if (!op1 && (op2 & 0x8) && !(op2 & 0x1)) // Add/Subtract (shifted register)
    {
      result.varient_64 = (B8)((instruction >> 31) & 0x1); 
      U8 s = (instruction >> 29) & 0x1;
      result.add_sub_shifted.shift = (instruction >> 22) & 0x3;
      result.add_sub_shifted.rm = (instruction >> 16) & 0x1f;
      result.add_sub_shifted.imm6 = (instruction >> 10) & 0x1f;
      result.add_sub_shifted.rn = (instruction >> 5) & 0x1f;
      result.add_sub_shifted.rd = instruction & 0x1f;

      if (!op0 && !s) // ADD
      {
        result.id = Instr_as_add; 
      }
      else if (op0 && !s) // SUB
      {
        result.id = Instr_as_sub; 
      }
      else 
      {
        InvalidPath;
      }
    }
    else if (op1 && (op2 & 0x8)) // Data-processing (3 source)
    {
      result.varient_64 = (B8)((instruction >> 31) & 0x1); 
      U8 op54 = (instruction >> 29) & 0x3;
      U8 op31 = (instruction >> 21) & 0x7;
      result.data_proc.rm = (instruction >> 16) & 0x1f;
      U8 o0 = (instruction >> 15) & 0x1;
      result.data_proc.ra = (instruction >> 10) & 0x1f;
      result.data_proc.rn = (instruction >> 5) & 0x1f;
      result.data_proc.rd = instruction & 0x1f;

      if (!op54 && !op31 && !o0) // MADD (aliased to MUL when ra is the zero register)
      {
        result.id = Instr_madd;
      }
      else
      {
        InvalidPath;
      }
    }
    else
    {
      InvalidPath; // Unhandled data process register group encoding
    }
  }
  else
  {
    printf("Unhandled decode group %b\n", decode_group);
  }
  return result;
}

internal void evaluate_decoded_instruction(
  DecodedInstruction decd_instr,
  U8* gp_regs,
  U8* memory,
  PSTATE* pstate,
  U64* pc_ptr,
  B8* modified_pc)
{
  switch(decd_instr.id)
  {
    case Instr_sub_imm:
    case Instr_subs_imm:
    {
      PSTATE pstate_tmp{};
      if (decd_instr.varient_64)
      {
        U64* xn_ptr = gp_reg_u64(gp_regs, decd_instr.add_sub_imm.rn);
        U64* xd_ptr = gp_reg_u64(gp_regs, decd_instr.add_sub_imm.rd);
        U64 imm = ze_imm12_to_u64(decd_instr.add_sub_imm.imm12);
        *xd_ptr = add_with_carry_u64(&pstate_tmp, *xn_ptr, ~imm, 1);
      }
      else
      {
        U32* wn_ptr = gp_reg_u32(gp_regs, decd_instr.add_sub_imm.rn);
        U32* wd_ptr = gp_reg_u32(gp_regs, decd_instr.add_sub_imm.rd);
        U32 imm = ze_imm12_to_u32(decd_instr.add_sub_imm.imm12);
        *wd_ptr = add_with_carry_u32(&pstate_tmp, *wn_ptr, ~imm, 1);
      }
      if (decd_instr.id == Instr_subs_imm)
      {
        *pstate = pstate_tmp;
      }
    } break;
    case Instr_movz:
    {
      if (decd_instr.varient_64)
      {
        U64* xd_ptr = gp_reg_u64(gp_regs, decd_instr.move_wide_imm.rd);
        *xd_ptr = decd_instr.move_wide_imm.imm16;
      }
      else
      {
        U32* wd_ptr = gp_reg_u32(gp_regs, decd_instr.move_wide_imm.rd);
        *wd_ptr = decd_instr.move_wide_imm.imm16;
      }
    } break;

    case Instr_b_cond:
    {
      if (condition_holds(decd_instr.cond_branch_imm.cond, *pstate))
      {
        S32 offset = (S32)se_imm19_to_u32(decd_instr.cond_branch_imm.imm19);
        U64 target_address = *pc_ptr + offset;
        branch_to(pc_ptr, pstate, target_address, BranchType_dir, 1);
        *modified_pc = 1;
      }
    } break;
    case Instr_ret:
    {
      U64 target = *gp_reg_u64(gp_regs, decd_instr.uncond_branch_reg.rn);
      branch_to(pc_ptr, pstate, target, BranchType_ret, 0);
      *modified_pc = 1;
    } break;
    case Instr_b:
    {
      U64 offset = se_imm26_to_u64(decd_instr.uncond_branch_imm.imm26);
      branch_to(pc_ptr, pstate, *pc_ptr + offset, BranchType_dir, 0);
      *modified_pc = 1;
    } break;
    case Instr_bl:
    {
      U64 offset = se_imm26_to_u64(decd_instr.uncond_branch_imm.imm26);
      *gp_reg_u64(gp_regs, 30) = *pc_ptr + 1;
      branch_to(pc_ptr, pstate, *pc_ptr + offset, BranchType_dir_call, 0);
      *modified_pc = 1;
    } break;
    case Instr_cbz:
    {
      U64 val;
      if (decd_instr.varient_64)
      {
        U64* xt_ptr = gp_reg_u64(gp_regs, decd_instr.comp_and_branch_imm.rt);
        val = *xt_ptr;
      }
      else
      {
        U32* wt_ptr = gp_reg_u32(gp_regs, decd_instr.comp_and_branch_imm.rt);
        val = *wt_ptr;
      }
      if (val == 0)
      {
        S32 offset = (S32)se_imm19_to_u32(decd_instr.comp_and_branch_imm.imm19);
        U64 target_address = *pc_ptr + offset;
        branch_to(pc_ptr, pstate, target_address, BranchType_dir, 1);
        *modified_pc = 1;
      }
    } break;
    case Instr_cbnz:
    {
      U64 val;
      if (decd_instr.varient_64)
      {
        U64* xt_ptr = gp_reg_u64(gp_regs, decd_instr.comp_and_branch_imm.rt);
        val = *xt_ptr;
      }
      else
      {
        U32* wt_ptr = gp_reg_u32(gp_regs, decd_instr.comp_and_branch_imm.rt);
        val = *wt_ptr;
      }
      if (val != 0)
      {
        S32 offset = (S32)se_imm19_to_u32(decd_instr.comp_and_branch_imm.imm19);
        U64 target_address = *pc_ptr + offset;
        branch_to(pc_ptr, pstate, target_address, BranchType_dir, 1);
        *modified_pc = 1;
      }
    } break;
    case Instr_lr_ldr: 
    {
      S32 offset = (S32)se_imm19_to_u32(decd_instr.load_reg.imm19);
      U32 address = *pc_ptr + offset;

      U64* xt_ptr = gp_reg_u64(gp_regs, decd_instr.load_reg.rt);
      *xt_ptr = se_u32_to_u64(memory[address]);
    } break;

    case Instr_lsr_strb:
    {
      U32* wt_ptr = gp_reg_u32(gp_regs, decd_instr.load_store_reg.rt);
      U64* xn_ptr = gp_reg_u64(gp_regs, decd_instr.load_store_reg.rn);
      memory[*xn_ptr] = (U8)*wt_ptr;
    } break;
    case Instr_lsr_str:
    {
      U64* xt_ptr = gp_reg_u64(gp_regs, decd_instr.load_store_reg.rt);
      U64* xn_ptr = gp_reg_u64(gp_regs, decd_instr.load_store_reg.rn);
      *(U64*)(&memory[*xn_ptr]) = *xt_ptr;
    } break;
    case Instr_lsr_ldrb:
    {
      U32* wt_ptr = gp_reg_u32(gp_regs, decd_instr.load_store_reg.rt);
      U64* xn_ptr = gp_reg_u64(gp_regs, decd_instr.load_store_reg.rn);
      *wt_ptr = memory[*xn_ptr];
    } break;
    case Instr_lsr_ldr:
    {
      U64* xt_ptr = gp_reg_u64(gp_regs, decd_instr.load_store_reg.rt);
      U64* xn_ptr = gp_reg_u64(gp_regs, decd_instr.load_store_reg.rn);
      *xt_ptr = *(U64*)(&memory[*xn_ptr]);
    } break;

    case Instr_logical_mov: 
    {
      if (decd_instr.varient_64)
      {
        U64* xn_ptr = gp_reg_u64(gp_regs, decd_instr.logical.rn);
        U64* xm_ptr = gp_reg_u64(gp_regs, decd_instr.logical.rm);
        U64* xd_ptr = gp_reg_u64(gp_regs, decd_instr.logical.rd);
        *xd_ptr = (*xn_ptr | (*xm_ptr << decd_instr.logical.imm6));
      }
      else
      {
        U32* wn_ptr = gp_reg_u32(gp_regs, decd_instr.logical.rn);
        U32* wm_ptr = gp_reg_u32(gp_regs, decd_instr.logical.rm);
        U32* wd_ptr = gp_reg_u32(gp_regs, decd_instr.logical.rd);
        *wd_ptr = (*wn_ptr | (*wm_ptr << decd_instr.logical.imm6));
      }
    } break;
    case Instr_as_add: 
    {
      if (decd_instr.varient_64)
      {
        U64* xm_ptr = gp_reg_u64(gp_regs, decd_instr.add_sub_shifted.rm);
        U64* xn_ptr = gp_reg_u64(gp_regs, decd_instr.add_sub_shifted.rn);
        U64* xd_ptr = gp_reg_u64(gp_regs, decd_instr.add_sub_shifted.rd);
        U64 rhs = shift_reg_u64(*xm_ptr, decd_instr.add_sub_shifted.shift, decd_instr.add_sub_shifted.imm6);
        *xd_ptr = add_with_carry_u64(pstate, *xn_ptr, rhs, 0);
      }
      else
      {
        U32* wm_ptr = gp_reg_u32(gp_regs, decd_instr.add_sub_shifted.rm);
        U32* wn_ptr = gp_reg_u32(gp_regs, decd_instr.add_sub_shifted.rn);
        U32* wd_ptr = gp_reg_u32(gp_regs, decd_instr.add_sub_shifted.rd);
        U32 rhs = shift_reg_u32(*wm_ptr, decd_instr.add_sub_shifted.shift, decd_instr.add_sub_shifted.imm6);
        *wd_ptr = add_with_carry_u32(pstate, *wn_ptr, rhs, 0);
      }
    } break;
    case Instr_as_sub:
    {
      if (decd_instr.varient_64)
      {
        U64* xm_ptr = gp_reg_u64(gp_regs, decd_instr.add_sub_shifted.rm);
        U64* xn_ptr = gp_reg_u64(gp_regs, decd_instr.add_sub_shifted.rn);
        U64* xd_ptr = gp_reg_u64(gp_regs, decd_instr.add_sub_shifted.rd);
        U64 rhs = ~shift_reg_u64(*xm_ptr, decd_instr.add_sub_shifted.shift, decd_instr.add_sub_shifted.imm6);
        *xd_ptr = add_with_carry_u64(pstate, *xn_ptr, rhs, 1);
      }
      else
      {
        U32* wm_ptr = gp_reg_u32(gp_regs, decd_instr.add_sub_shifted.rm);
        U32* wn_ptr = gp_reg_u32(gp_regs, decd_instr.add_sub_shifted.rn);
        U32* wd_ptr = gp_reg_u32(gp_regs, decd_instr.add_sub_shifted.rd);
        U32 rhs = ~shift_reg_u32(*wm_ptr, decd_instr.add_sub_shifted.shift, decd_instr.add_sub_shifted.imm6);
        *wd_ptr = add_with_carry_u32(pstate, *wn_ptr, rhs, 1);
      }
    } break;
    case Instr_madd: 
    {
      if (decd_instr.varient_64) // FIXME(calebarg): Update PSTATE. See MADD psudeo code.
      {
        U64* xn_ptr = gp_reg_u64(gp_regs, decd_instr.data_proc.rn);
        U64* xm_ptr = gp_reg_u64(gp_regs, decd_instr.data_proc.rm);
        U64* xa_ptr = gp_reg_u64(gp_regs, decd_instr.data_proc.ra);
        U64* xd_ptr = gp_reg_u64(gp_regs, decd_instr.data_proc.rd);

        *xd_ptr = *xa_ptr + *xn_ptr * *xm_ptr;
      }
      else
      {
        U32* wn_ptr = gp_reg_u32(gp_regs, decd_instr.data_proc.rn);
        U32* wm_ptr = gp_reg_u32(gp_regs, decd_instr.data_proc.rm);
        U32* wa_ptr = gp_reg_u32(gp_regs, decd_instr.data_proc.ra);
        U32* wd_ptr = gp_reg_u32(gp_regs, decd_instr.data_proc.rd);

        *wd_ptr = *wa_ptr + *wn_ptr * *wm_ptr;
      }
    } break;
    default: InvalidPath;
  }
}

int main(int argc, char* argv[])
{
  if (sizeof(DecodedInstruction) != 8)
  {
    printf("Sizeof DecodedInstruction (%ld) bytes\n", sizeof(DecodedInstruction));
    InvalidPath;
  }

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
    for (U16 section_idx=0;
         section_idx < elf_header->shnum;
         ++section_idx)
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

  U8* gp_regs = (U8*)arena_push(arena, 32*8); // NOTE(caleb): R31 is reserved as the zero register.
  memset(gp_regs, 0, 32*8);

  U64 pc = 0;
  U64* pc_ptr = &pc;

  // Special purpose registers
  PSTATE pstate{};
  if (sizeof(pstate) != 8)
  {
    InvalidPath;
  }

  // NOTE(calebarg): Not sure how I'm going to do this long term.
  U8* memory = (U8*)arena_push(arena, 512);
  for (U64 byte_idx=0; byte_idx < 512; ++byte_idx)
  {
    memory[byte_idx] = 0;
    if (!(byte_idx % 40))
    {
      memory[byte_idx] = 2;
    }
  }

  // Catch ctrl-c so the terminal so the terminal can be restored.
  struct sigaction sa;
  sa.sa_handler = quit_on_ctrlc;
  if (sigaction(SIGINT, &sa, 0) == -1)
  {
    InvalidPath;
  }

  struct termios old_term_attribs;
  if (tcgetattr(STDIN_FILENO, &old_term_attribs) == -1)
  {
    InvalidPath;
  }

  struct termios new_term_attribs = old_term_attribs;
  new_term_attribs.c_lflag &= ~(ECHO | ICANON);
  new_term_attribs.c_cc[VMIN] = 0; // Minimum number of characters for noncanonical read
  new_term_attribs.c_cc[VTIME] = 0; // Timout in deciseconds for noncanonical read
  if (tcsetattr(STDIN_FILENO, TCSANOW, &new_term_attribs) == -1)
  {
    InvalidPath;
  }

  alternate_screen_buffer();

  // Initial screen clear
  for (U32 row_idx=0; row_idx < 100; ++row_idx)
  {
    move_cursor(row_idx + 1, 1);
    clear_line();
  }

  F64 instructions_per_second = (1000.0 / 1000.0); // <-- Clock speed (1MHz)
  F64 last_time = read_wall_clock();

  U64 instruction_count = machine_code.len / 4;
  U32* instructions = (U32*)machine_code.data;
  while ((*pc_ptr < instruction_count) && (running))
  {
    F64 time_now = read_wall_clock();
    F64 time_since_last_frame_ms = (time_now * 1000) - (last_time * 1000);
    if (time_since_last_frame_ms >= instructions_per_second)
    {
      last_time = time_now;

      // Pull decoded instruction from a queue

      U8 input_byte = 0;
      if (read(STDIN_FILENO, &input_byte, 1) != 0)
      {
        memory[310] = input_byte;
      }

      B8 modified_pc = 0;
      U32 curr_instruction = instructions[*pc_ptr];

#if 1
      DecodedInstruction decd_instr = decode_instruction(curr_instruction);
      evaluate_decoded_instruction(decd_instr, gp_regs, memory, &pstate, pc_ptr, &modified_pc);
#else
      U8 decode_group = (curr_instruction >> 25) & 0xf;
      B8 sf = (curr_instruction >> 31) & 0x1;
      if ((decode_group & 0x8) && // Data processing -- immediate
          !(decode_group & 0x6))
      {
        U8 op0 = (curr_instruction >> 22) & 0xf;
        if (!(op0 & 0x8) && (op0 & 4) && !(op0 & 2)) // Add/Subtract (immediate)
        {
          U8 op = (curr_instruction >> 30) & 0x1;
          U8 s = (curr_instruction >> 29) & 0x1;

          if (op) // SUB/SUBS (CMP alias)
          {
            U8 sh = (curr_instruction >> 22) & 0x1;
            U16 imm12 = (curr_instruction >> 10) & 0xfff;
            U8 rn = (curr_instruction >> 5) & 0x1f;
            U8 rd = curr_instruction & 0x1f;

            if (rn == 31)
            {
              InvalidPath; // Not implemented
            }

            PSTATE pstate_for_op{};
            if (sf)
            {
              U64* xn_ptr = gp_reg_u64(gp_regs, rn);
              U64* xd_ptr = gp_reg_u64(gp_regs, rd);
              U64 imm = ze_imm12_to_u64(imm12);
              *xd_ptr = add_with_carry_u64(&pstate_for_op, *xn_ptr, ~imm, 1);
            }
            else
            {
              U32* wn_ptr = gp_reg_u32(gp_regs, rn);
              U32* wd_ptr = gp_reg_u32(gp_regs, rd);
              U32 imm = ze_imm12_to_u32(imm12);
              *wd_ptr = add_with_carry_u32(&pstate_for_op, *wn_ptr, ~imm, 1);
            }
            if (s)
            {
              pstate = pstate_for_op;
            }
          }
          else
          {
            InvalidPath; // Not implemented
          }
        }
        else if (op0 & 0xa && !(op0 & 4)) // Move wide (immediate)
        {
          U8 op_code = (curr_instruction >> 29) & 0x3;
          switch (op_code)
          {
            case 0x2: // MOVZ
            {
              U16 immediate_val = (curr_instruction >> 5) & 0xffff;
              U8 dest_reg = curr_instruction & 0xf;
              if (sf)
              {
                ((U64*)gp_regs)[dest_reg] = immediate_val;
              }
              else
              {
                U32* wd_ptr = gp_reg_u32(gp_regs, dest_reg);
                *wd_ptr = immediate_val;
              }
            } break;
            default: break;
          }
        }
        else
        {
          InvalidPath; // Unsupported instruction class
        }
      }
      else if ((decode_group & 0xa) && // Branches, exception generating and system instructions
              !(decode_group & 0x4))
      {
        U8 op0 = (curr_instruction >> 29) & 0x7;
        U16 op1 = (curr_instruction >> 12) & 0x3fff;
        if (!(op0 & 0x4) &&  // Conditional branch (immediate)
            (op0 & 0x2) &&
            !(op0 & 0x1) &&
            !(op1 & 0x2000))
        {
          U8 o1 = (curr_instruction >> 24) & 0x1;
          U8 o0 = (curr_instruction >> 4) & 0x1;
          if (!o1 && !o0) // B.cond
          {
            U8 imm19 = (curr_instruction >> 5) & 0x7ffff;
            U8 cond = curr_instruction & 0xf;
            if (condition_holds(cond, pstate))
            {
              S32 offset = (S32)se_imm19_to_u32(imm19);
              U64 target_address = *pc_ptr + offset;
              branch_to(pc_ptr, &pstate, target_address, BranchType_dir, 1);
              modified_pc = 1;
            }
          }
          else
          {
            InvalidPath; // Not Implemented
          }
        }
        else if ((op0 & 0x4) && (op0 & 0x2) && !(op0 & 0x1) && (op1 & 0x2000)) // Unconditional branch (register)
        {
          U8 opc = (curr_instruction >> 21) & 0xf;
          U8 op2 = (curr_instruction >> 16) & 0x1f;
          U8 op3 = (curr_instruction >> 10) & 0x3f;
          U8 rn = (curr_instruction >> 5) & 0x1f;
          U8 op4 = curr_instruction & 0x1f;

          if ((opc & 0x2) && (op2 & 0x1f) && !op3 && !op4) // RET
          {
            U64 target = *gp_reg_u64(gp_regs, rn);
            branch_to(pc_ptr, &pstate, target, BranchType_ret, 0);
            modified_pc = 1;
          }
          else
          {
            InvalidPath;
          }
        }
        else if (!(op0 & 0x1) && !(op0 & 0x2)) // Unconditional branch (immediate) B/BL
        {
          U8 op = (curr_instruction >> 31) & 0x1;
          U32 imm26 = curr_instruction & 0x3ffffff;
          U64 offset = se_imm26_to_u64(imm26);
          BranchType branch_type = BranchType_dir;
          if (op) // Store return address in r30 (BL)
          {
            *gp_reg_u64(gp_regs, 30) = *pc_ptr + 1;
            branch_type = BranchType_dir_call;
          }
          branch_to(pc_ptr, &pstate, *pc_ptr + offset, branch_type, 0);
          modified_pc = 1;
        }
        else if ((op0 & 0x1) && (~op1 & 0x2000)) // Compare and branch (immediate)
        {
          U8 op = (curr_instruction >> 24) & 0x1;
          U32 imm19 = (curr_instruction >> 5) & 0x7ffff;
          U8 rt = curr_instruction & 0x1f;
          B8 take_branch = 0;

          if (sf) // CBZ/CBNZ 64 bit
          {
            U64* xt_ptr = gp_reg_u64(gp_regs, rt);
            if (((*xt_ptr == 0) && (op == 0)) ||  // CBZ
                ((*xt_ptr != 0) && (op == 1)))  // CBNZ
            {
              take_branch = 1;
            }
          }
          else // CBZ/CBNZ 32 bit
          {
            U32* wt_ptr = gp_reg_u32(gp_regs, rt);
            if (((*wt_ptr == 0) && (op == 0)) ||  // CBZ
                ((*wt_ptr != 0) && (op == 1)))  // CBNZ
            {
              take_branch = 1;
            }
          }

          if (take_branch)
          {
            S32 offset = (S32)se_imm19_to_u32(imm19);
            U64 target_address = *pc_ptr + offset;
            branch_to(pc_ptr, &pstate, target_address, BranchType_dir, 1);
            modified_pc = 1;
          }
        }
        else
        {
          InvalidPath;
        }
      }
      else if ((decode_group & 0x4) &&  // Loads and Stores
              !(decode_group & 0x1))
      {
        U8 op0 = (curr_instruction >> 28) & 0xf;
        U8 op1 = (curr_instruction >> 26) & 0x1;

        if ((op0 & 0x1) && !(op0 & 0x2)) // Load/Store register literal
        {
          U8 opc = (curr_instruction >> 30) & 0x3;
          U8 vr = (curr_instruction >> 26) & 0x1;

          switch (opc)
          {
            case 0x1: // LDR
            {
              U32 imm19 = (curr_instruction >> 5) & 0x7ffff;
              U8 rt = curr_instruction & 0x1f;

              S32 offset = (S32)se_imm19_to_u32(imm19);
              U32 address = *pc_ptr + offset;

              U64* xt_ptr = gp_reg_u64(gp_regs, rt);
              *xt_ptr = se_u32_to_u64(memory[address]);
            } break;
            default: InvalidPath;
          }
        }
        else if ((op0 & 0x1) && (op0 & 0x2)) // Load/Store register (register offset)
        {
          U8 size = (curr_instruction >> 30) & 0x3;
          U8 vr = (curr_instruction >> 26) & 0x1;
          U8 opc = (curr_instruction >> 22) & 0x3;
          switch (opc)
          {
            case 0x0: // STR/STRB
            {
              U8 rt = curr_instruction & 0x1f;
              U8 rn = (curr_instruction >> 5) & 0x1f;
              U8 option = (curr_instruction >> 13) & 0x7;
              if (option != 0x0)
              {
                InvalidPath; // Unxexpected option value
              }
              switch (size)
              {
                case 0x0: // STRB
                {
                  U32* wt_ptr = gp_reg_u32(gp_regs, rt);
                  U64* xn_ptr = gp_reg_u64(gp_regs, rn);
                  memory[*xn_ptr] = (U8)*wt_ptr;
                } break;
                case 0x3: // STR
                {
                  U64* xt_ptr = gp_reg_u64(gp_regs, rt);
                  U64* xn_ptr = gp_reg_u64(gp_regs, rn);
                  *(U64*)(&memory[*xn_ptr]) = *xt_ptr;
                } break;
                default: InvalidPath;
              }
            } break;
            case 0x1: // LDR/LDRB
            {
              U8 rt = curr_instruction & 0x1f;
              U8 rn = (curr_instruction >> 5) & 0x1f;
              U8 option = (curr_instruction >> 13) & 0x7;

              if (option != 0x0)
              {
                InvalidPath; // Unxexpected option value
              }
              switch (size)
              {
                case 0x0: // LDRB
                {
                  U32* wt_ptr = gp_reg_u32(gp_regs, rt);
                  U64* xn_ptr = gp_reg_u64(gp_regs, rn);
                  *wt_ptr = memory[*xn_ptr];
                } break;
                case 0x3: // LDR
                {
                  U64* xt_ptr = gp_reg_u64(gp_regs, rt);
                  U64* xn_ptr = gp_reg_u64(gp_regs, rn);
                  *xt_ptr = *(U64*)(&memory[*xn_ptr]);
                } break;
                default: InvalidPath;
              }
            } break;
            default: InvalidPath;
          }
        }
        else
        {
          InvalidPath; // Unhandled decode group
        }
      }
      else if ((decode_group & 0x5) && // Data processing -- register
              !(decode_group & 0x2))
      {
        U8 op0 = (curr_instruction >> 30) & 0x1;
        U8 op1 = (curr_instruction >> 28) & 0x1;
        U8 op2 = (curr_instruction >> 21) & 0xf;
        if (!op1 && !(op2 & 0x8)) // Logical (shifted register)
        {
          U8 opc = (curr_instruction >> 29) & 0x3;
          U8 n = (curr_instruction >> 21) & 0x1;
          U8 imm6 = (curr_instruction >> 10) & 0x1f;

          switch (opc)
          {
            case 0x1: // ORR/ORN (MOV alias)
            {
              U8 shift = (curr_instruction >> 22) & 0x3;
              U8 rm = (curr_instruction >> 16) & 0x1f;
              U8 rn = (curr_instruction >> 5) & 0x1f;
              U8 rd = curr_instruction & 0x1f;

              if ((shift != 0) || (imm6 != 0) || (n != 0))
              {
                InvalidPath; // Not a MOV alias
              }

              if (sf)
              {
                U64* xn_ptr = gp_reg_u64(gp_regs, rn);
                U64* xm_ptr = gp_reg_u64(gp_regs, rm);
                U64* xd_ptr = gp_reg_u64(gp_regs, rd);
                *xd_ptr = (*xn_ptr | (*xm_ptr << imm6));
              }
              else

              {
                U32* wn_ptr = gp_reg_u32(gp_regs, rn);
                U32* wm_ptr = gp_reg_u32(gp_regs, rm);
                U32* wd_ptr = gp_reg_u32(gp_regs, rd);
                *wd_ptr = (*wn_ptr | (*wm_ptr << imm6));
              }
            } break;
            default: InvalidPath;
          }
        }
        else if (!op1 && (op2 & 0x8) && !(op2 & 0x1)) // Add/Subtract (shifted register)
        {
          U8 s = (curr_instruction >> 29) & 0x1;
          U8 shift_type = (curr_instruction >> 22) & 0x3;
          U8 rm = (curr_instruction >> 16) & 0x1f;
          U8 imm6 = (curr_instruction >> 10) & 0x1f;
          U8 rn = (curr_instruction >> 5) & 0x1f;
          U8 rd = curr_instruction & 0x1f;

          U64* xm_ptr = gp_reg_u64(gp_regs, rm);
          U64* xn_ptr = gp_reg_u64(gp_regs, rn);
          U64* xd_ptr = gp_reg_u64(gp_regs, rd);

          U32* wm_ptr = gp_reg_u32(gp_regs, rm);
          U32* wn_ptr = gp_reg_u32(gp_regs, rn);
          U32* wd_ptr = gp_reg_u32(gp_regs, rd);

          if (!op0 && !s) // ADD
          {
            if (sf)
            {
              U64 rhs = shift_reg_u64(*xm_ptr, shift_type, imm6);
              *xd_ptr = add_with_carry_u64(&pstate, *xn_ptr, rhs, 0);
            }
            else
            {
              U32 rhs = shift_reg_u32(*wm_ptr, shift_type, imm6);
              *wd_ptr = add_with_carry_u32(&pstate, *wn_ptr, rhs, 0);
            }
          }
          else if (op0 && !s) // SUB
          {
            if (sf)
            {
              U64 rhs = ~shift_reg_u64(*xm_ptr, shift_type, imm6);
              *xd_ptr = add_with_carry_u64(&pstate, *xn_ptr, rhs, 1);
            }
            else
            {
              U32 rhs = ~shift_reg_u32(*wm_ptr, shift_type, imm6);
              *wd_ptr = add_with_carry_u32(&pstate, *wn_ptr, rhs, 1);
            }
          }
        }

        else if (op1 && (op2 & 0x8)) // Data-processing (3 source)
        {
          U8 op54 = (curr_instruction >> 29) & 0x3;
          U8 op31 = (curr_instruction >> 21) & 0x7;
          U8 rm = (curr_instruction >> 16) & 0x1f;
          U8 o0 = (curr_instruction >> 15) & 0x1;
          U8 ra = (curr_instruction >> 10) & 0x1f;
          U8 rn = (curr_instruction >> 5) & 0x1f;
          U8 rd = curr_instruction & 0x1f;

          if (!op54 && !op31 && !o0) // MADD (aliased to MUL when ra is the zero register)
          {
            if (sf) // FIXME(calebarg): Update PSTATE. See MADD psudeo code.
            {
              U64* xn_ptr = gp_reg_u64(gp_regs, rn);
              U64* xm_ptr = gp_reg_u64(gp_regs, rm);
              U64* xa_ptr = gp_reg_u64(gp_regs, ra);
              U64* xd_ptr = gp_reg_u64(gp_regs, rd);

              *xd_ptr = *xa_ptr + *xn_ptr * *xm_ptr;
            }
            else
            {
              U32* wn_ptr = gp_reg_u32(gp_regs, rn);
              U32* wm_ptr = gp_reg_u32(gp_regs, rm);
              U32* wa_ptr = gp_reg_u32(gp_regs, ra);
              U32* wd_ptr = gp_reg_u32(gp_regs, rd);

              *wd_ptr = *wa_ptr + *wn_ptr * *wm_ptr;
            }
          }
          else
          {
            InvalidPath;
          }
        }
        else
        {
          InvalidPath; // Unhandled data process register group encoding
        }
      }
      else
      {
        printf("Unhandled decode group %b\n", decode_group);
      }
#endif
      *gp_reg_u64(gp_regs, 31) = 0; // FIXME(calebarg): Can't write to this register.
                                    // I can probably assert that this is zero. To figure out
                                    // which instructions are setting it if any.
      if (!modified_pc)
      {
        (*pc_ptr)++;
      }

      move_cursor(1, 1);
      for (U32 row_idx=0; row_idx < 32; ++row_idx)
      {
        printf("W%02ld: %08x", row_idx, *gp_reg_u32(gp_regs, row_idx));

        dec_character_set();
        printf("  %c  ", 97);
        ascii_character_set();

        printf("%03x: ", row_idx * 4);
        for (U32 col_idx=0; col_idx < 4; ++col_idx)
        {
          if ((row_idx * 4 + col_idx) < 128)
          {
            printf("%08x ", ((U32*)memory)[row_idx * 4 + col_idx]);
          }
        }
        printf("\n");
      }
      printf("N:%u , Z:%u , C:%u , V:%u\n", pstate.n, pstate.z, pstate.c, pstate.v);
      printf("PC: %x\n", *pc_ptr);

#if 0
      // Crash if took to long (realtime 1MHz hard req)
      F64 instruction_time = read_wall_clock() * 1000 - last_time * 1000;
      if (instruction_time > instructions_per_second)
      {
        printf("MAX TIME: %lf, TOOK: %lf, PC: %llu\n", instructions_per_second, instruction_time, pc);
        InvalidPath;
      }
#endif

      if (memory[311]) // The program will write this byte when it wants a draw call.
      {
        // Draw display 10 rows by 20 cols
        U8* display_memory = &memory[312];
        for (U32 row_idx = 0; row_idx < 10; ++row_idx)
        {
          move_cursor(row_idx + 1, 64);
          for (U32 col_idx = 0; col_idx < 20; ++col_idx)
          {
            if (display_memory[row_idx * 20 + col_idx] == 2)
            {
              putc('f', stdout);
            }
            else if (display_memory[row_idx * 20 + col_idx])
            {
              putc('#', stdout);
            }
            else
            {
              putc(' ', stdout);
            }
          }
        }
        memory[311] = 0;
      }

    }
  }

  main_screen_buffer();
  if (tcsetattr(STDIN_FILENO, TCSANOW, &old_term_attribs) == -1)
  {
    InvalidPath;
  }

  return 0;
}

