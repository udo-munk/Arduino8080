//
// Intel 8080 CPU emulation on an Arduino Nano
//            derived from Z80PACK
// Copyright 2024, Udo Munk
//
// History:
// 04-MAY-2024 Release 1.0 implements a very basic 8080 system
// 06-MAY-2024 Release 1.1 add support for a ROM in flash
// 07-MAY-2024 Release 1.2 move 8080 memory into a FRAM
//

// 64 KB unbanked memory in FRAM
// we want hardware SPI, the 2 and 4 MBit modules can be clocked
// with 40 MHz, the smaller ones with 20 MHz
Adafruit_FRAM_SPI fram = Adafruit_FRAM_SPI(FRAM_CS, 40000000);

// get preloaded code for the 8080 CPU
#define MEMSIZE 4096  // this much memory with 8080 code we can have in flash
#include "8080code.h"

// copy our 8080 code into FRAM
void init_memory(void)
{
  register int i;

  for (i = 0; i < code_length; i++)
    fram.write8(i, pgm_read_byte(code + i));
}

// read a byte from 8080 CPU memory address addr
static inline BYTE memrdr(WORD addr)
{
  if (addr < 0xff00)
    return fram.read8(addr);
  else
    return 0xff; // top memory page write protected for MITS BASIC
}

// write a byte data into 8080 CPU RAM at address addr 
static inline void memwrt(WORD addr, BYTE data)
{
  if (addr < 0xff00) // top memory page write protected for MITS BASIC
    fram.write8(addr, data);
}
