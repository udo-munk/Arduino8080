//
// Intel 8080 CPU emulation on an Arduino Nano
//            derived from Z80PACK
// Copyright 2024, Udo Munk
//
// History:
// 04-MAY-2024 Release 1.0
// 06-MAY-2024 Release 1.1 add support for a ROM in flash
// 07-MAY-2024 Release 1.2 move 8080 memory into a FRAM
//

#define MEMSIZE 2048

// get preloaded code for the 8080 CPU
#include "8080code.h"

// copy our code into FRAM
void init_memory(void)
{
//  register int i;

//  for (i = 0; i < code_length; i++)
//    fram.write8(i, pgm_read_byte(code + i));
}

// read a byte from 8080 CPU memory address addr
static inline BYTE memrdr(WORD addr)
{
  if (addr < MEMSIZE)
    return pgm_read_byte(code + addr);
  else
    return fram.read8(addr);
}

// write a byte data into 8080 CPU RAM at address addr 
static inline void memwrt(WORD addr, BYTE data)
{
  if (addr >= MEMSIZE)
    fram.write8(addr, data);
}
