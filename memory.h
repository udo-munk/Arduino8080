//
// Intel 8080 CPU emulation on an Arduino Nano
//            derived from Z80PACK
// Copyright 2024, Udo Munk
//
// History:
// 04-MAY-2024 Release 1.0
// 06-MAY-2024 Release 1.1 add support for a ROM in flash
//

#define ROMSIZE 2048
#define RAMSIZE 512
#define MEMSIZE ROMSIZE

// get preloaded ROM for the 8080 CPU
#include "8080code.h"

// and some RAM for the 8080 CPU
unsigned char ram[RAMSIZE];

// read a byte from 8080 CPU memory address addr
static inline BYTE memrdr(WORD addr)
{
  if (addr < ROMSIZE)
    return pgm_read_byte(code + addr);
  else if (addr < ROMSIZE + RAMSIZE)
    return (ram[addr - ROMSIZE]);
  else
    return (0xff);
}

// write a byte data into 8080 CPU RAM at address addr 
static inline void memwrt(WORD addr, BYTE data)
{
  if ((addr >= ROMSIZE) && (addr < ROMSIZE + RAMSIZE))
    ram[addr - ROMSIZE] = data;
}
