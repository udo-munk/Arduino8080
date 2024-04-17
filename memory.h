//
// Intel 8080 CPU emulation on an Arduino Nano
//            derived from Z80PACK
// Copyright 2024, Udo Munk
//

// trying to give 0.5KB memory from the available 2KB to the 8080 CPU
#define MEMSIZE 512

// get preloaded memory for the 8080 CPU
#include "8080code.h"

// read a byte from 8080 CPU memory address addr
static inline BYTE memrdr(WORD addr)
{
  if (addr < MEMSIZE)
    return(code[addr]);
  else
    return(0xff);
}

// write a byte data into 8080 CPU memory address addr 
static inline void memwrt(WORD addr, BYTE data)
{
  if (addr < MEMSIZE)
    code[addr] = data;
}
