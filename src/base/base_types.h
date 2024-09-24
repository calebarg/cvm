/////////////////////////////////////////////////////////////////////////////////////////
//  COPYRIGHT (c) 2024 Schweitzer Engineering Laboratories, Inc.
//  SEL Confidential
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef BASE_TYPES_H

// Foreign includes

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Keywords

#define internal static
#define global static
#define local_persist static

#ifdef WIN32
// FIXME:
//#define thread_local __declspec(thread)
#else
//#define thread_local __thread
#endif

#define nil 0

// Primitive types

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;
typedef S8 B8;
typedef S16 B16;
typedef S32 B32;
typedef S64 B64;
typedef float F32;
typedef double F64;

// Basic types

typedef struct Slice Slice;
struct Slice
{
   U8* ptr;
   U64 len; 
};

#define AlignPow2(a, b) ((a + b - 1) & ~(b - 1))

#define AssertBreak(m) (*((volatile U32*)0) = 0xCA1EB)
#define AssertMessage(m) AssertBreak(m)
#define InvalidPath AssertMessage("Invalid path")
#define InvalidCase \
   default:         \
      InvalidPath

#define KB(n) (n * 1024)
#define MB(n) (KB(n) * 1024)
#define GB(n) (MB(n) * 1024)

#define Max(a, b) (a > b ? a : b)
#define Min(a, b) (a < b ? a : b)

#define ArrayCount(a) (sizeof(a) / sizeof(a[0]))

#define BASE_TYPES_H
#endif
