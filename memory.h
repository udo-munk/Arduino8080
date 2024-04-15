//
// Intel 8080 CPU emulation on an Arduino Nano
//            derived from Z80PACK
// Copyright 2024, Udo Munk
//

// trying to give 0.5KB memory from the available 2KB to the 8080 CPU
#define MEMSIZE 512

// forward declaration for stuff needed here
extern void fatal(char *);

// memory for the 8080 CPU
BYTE memory[MEMSIZE];

// read a byte from 8080 CPU memory address addr
static inline BYTE memrdr(WORD addr)
{
  if (addr < MEMSIZE)
    return(memory[addr]);
  else
    fatal("address out of range in memrdr()");
}

// write a byte data into 8080 CPU memory address addr 
static inline void memwrt(WORD addr, BYTE data)
{
  if (addr < MEMSIZE)
    memory[addr] = data;
  else
    fatal("address out of range in memwrt()"); 
}