//
// Intel 8080 CPU emulation on an Arduino Nano
//            derived from Z80PACK
// Copyright 2024, Udo Munk & Thomas Eberhardt
//
// History:
// 04-MAY-2024 Release 1.0 implements a very basic 8080 system
// 06-MAY-2024 Release 1.1 add support for a ROM in flash
// 07-MAY-2024 Release 1.2 move 8080 memory into a FRAM
// 13-MAY-2024 Release 1.2.1 can run MITS Altair BASIC
// 17-MAY-2024 Release 1.3 switch to code minimized CPU from Thomas Eberhardt
// 18-MAY-2024 Release 1.4 read 8080 code from a file on SD into FRAM
//

//#define DEBUG // enables some debug messages

#include <SPI.h>
#include "Adafruit_FRAM_SPI.h"
#include "SdFat.h"

// unused analog pin to seed random generator
#define UAP 7
// chip select pin for FRAM (default)
#define FRAM_CS 10
// chip select pin for SD card
#define SD_CS 9

// data types for the 8080 CPU
typedef unsigned char BYTE;
typedef unsigned int  WORD;

// project includes
#include "simcore.h"
#include "memsim.h"
#include "iosim.h"
#include "config.h"

// 8080 CPU registers
BYTE A, B, C, D, E, H, L, F, IFF;
WORD PC8, SP8; // funny names because SP already is a macro here

// other global variables
CPUState State = Running;         // CPU state
unsigned long tstates = 0;        // executed T-states

// Insure FAT16/FAT32 only.
SdFat32 SD;

// Precomputed table for fast sign, zero and parity flag calculation
#define _ 0
#define S S_FLAG
#define Z Z_FLAG
#define P P_FLAG
static const BYTE szp_flags[256] = {
/*00*/	Z | P,    _,    _,    P,    _,    P,    P,    _,
/*08*/	    _,    P,  	P,    _,    P,    _,    _,    P,
/*10*/	    _,    P,    P,    _,    P,    _,    _,    P,
/*18*/	    P,    _,    _,    P,    _,    P,    P,    _,
/*20*/	    _,    P,    P,    _,    P,    _,    _,    P,
/*28*/	    P,    _,    _,    P,    _,    P,    P,    _,
/*30*/	    P,    _,    _,    P,    _,    P,    P,    _,
/*38*/	    _,    P,    P,    _,    P,    _,    _,    P,
/*40*/	    _,    P,    P,    _,    P,    _,    _,    P,
/*48*/	    P,    _,    _,    P,    _,    P,    P,    _,
/*50*/	    P,    _,    _,    P,    _,    P,    P,    _,
/*58*/	    _,    P,    P,    _,    P,    _,    _,    P,
/*60*/	    P,    _,    _,    P,    _,    P,    P,    _,
/*68*/	    _,    P,    P,    _,    P,    _,    _,    P,
/*70*/	    _,    P,    P,    _,    P,    _,    _,    P,
/*78*/	    P,    _,    _,    P,    _,    P,    P,    _,
/*80*/	    S,S | P,S | P,    S,S | P,    S,    S,S | P,
/*88*/	S | P,    S,    S,S | P,    S,S | P,S | P,    S,
/*90*/	S | P,    S,    S,S | P,    S,S | P,S | P,    S,
/*98*/	    S,S | P,S | P,    S,S | P,    S,    S,S | P,
/*a0*/	S | P,    S,    S,S | P,    S,S | P,S | P,    S,
/*a8*/	    S,S | P,S | P,    S,S | P,    S,    S,S | P,
/*b0*/	    S,S | P,S | P,    S,S | P,    S,    S,S | P,
/*b8*/	S | P,    S,    S,S | P,    S,S | P,S | P,    S,
/*c0*/	S | P,    S,    S,S | P,    S,S | P,S | P,    S,
/*c8*/	    S,S | P,S | P,    S,S | P,    S,    S,S | P,
/*d0*/	    S,S | P,S | P,    S,S | P,    S,    S,S | P,
/*d8*/	S | P,    S,    S,S | P,    S,S | P,S | P,    S,
/*e0*/	    S,S | P,S | P,    S,S | P,    S,    S,S | P,
/*e8*/	S | P,    S,    S,S | P,    S,S | P,S | P,    S,
/*f0*/	S | P,    S,    S,S | P,    S,S | P,S | P,    S,
/*f8*/	    S,S | P,S | P,    S,S | P,    S,    S,S | P
};
#undef _
#undef S
#undef Z
#undef P

#define SZP_FLAGS (S_FLAG | Z_FLAG | P_FLAG)

// This function initializes the CPU into the documented
// state. Prevents assumptions about register contents
// of a just powered CPU, like HL is always 0 in the
// beginning, which is not the case with the silicon.
static void init_cpu(void)
{
  PC8 = 0;
  SP8 = random(65535);
  A = random(255);
  B = random(255);
  C = random(255);
  D = random(255);
  E = random(255);
  H = random(255);
  L = random(255);
  F = random(255);
  F &= ~(X_FLAG | Y_FLAG);
  F |= N_FLAG;
}

// This function builds the 8080 central processing unit.
// The opcode where PC points to is fetched from the memory
// and PC incremented by one. The opcode is then dispatched
// to execute code, which emulates this 8080 opcode.
void cpu_8080(void)
{
  int t, i, carry, old_c_flag;
  BYTE tmp, P, spl, sph;
  WORD addr;

  do {
    switch (memrdr(PC8++)) { // execute next opcode

    case 0x00:			/* NOP */
      t = 4;
      break;

    case 0x01:			/* LXI B,nn */
      C = memrdr(PC8++);
      B = memrdr(PC8++);
      t = 10;
      break;

    case 0x02:			/* STAX B */
      memwrt((B << 8) + C, A);
      t = 7;
      break;

    case 0x03:			/* INX B */
      C++;
      if (!C) B++;
      t = 5;
      break;

    case 0x04:			/* INR B */
      B++;
      ((B & 0xf) == 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      F = (F & ~SZP_FLAGS) | szp_flags[B];
      t = 5;
      break;

    case 0x05:			/* DCR B */
      B--;
      ((B & 0xf) == 0xf) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      F = (F & ~SZP_FLAGS) | szp_flags[B];
      t = 5;
      break;

    case 0x06:			/* MVI B,n */
      B = memrdr(PC8++);
      t = 7;
      break;

    case 0x07:			/* RLC */
      i = (A & 128) ? 1 : 0;
      (i) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A <<= 1;
      A |= i;
      t = 4;
      break;

    case 0x08:			/* NOP* */
      t = 4;
      break;

    case 0x09:			/* DAD B */
      carry = (L + C > 255) ? 1 : 0;
      L += C;
      (H + B + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      H += B + carry;
      t = 10;
      break;

    case 0x0a:			/* LDAX B */
      A = memrdr((B << 8) + C);
      t = 7;
      break;

    case 0x0b:			/* DCX B */
      C--;
      if (C == 0xff) B--;
      t = 5;
      break;

    case 0x0c:			/* INR C */
      C++;
      ((C & 0xf) == 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      F = (F & ~SZP_FLAGS) | szp_flags[C];
      t = 5;
      break;

    case 0x0d:			/* DCR C */
      C--;
      ((C & 0xf) == 0xf) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      F = (F & ~SZP_FLAGS) | szp_flags[C];
      t = 5;
      break;

    case 0x0e:			/* MVI C,n */
      C = memrdr(PC8++);
      t = 7;
      break;

    case 0x0f:			/* RRC */
      i = A & 1;
      (i) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A >>= 1;
      if (i) A |= 128;
      t = 4;
      break;

    case 0x10:			/* NOP* */
      t = 4;
      break;

    case 0x11:			/* LXI D,nn */
      E = memrdr(PC8++);
      D = memrdr(PC8++);
      t = 10;
      break;

    case 0x12:			/* STAX D */
      memwrt((D << 8) + E, A);
      t = 7;
      break;

    case 0x13:			/* INX D */
      E++;
      if (!E) D++;
      t = 5;
      break;

    case 0x14:			/* INR D */
      D++;
      ((D & 0xf) == 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      F = (F & ~SZP_FLAGS) | szp_flags[D];
      t = 5;
      break;

    case 0x15:			/* DCR D */
      D--;
      ((D & 0xf) == 0xf) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      F = (F & ~SZP_FLAGS) | szp_flags[D];
      t = 5;
      break;

    case 0x16:			/* MVI D,n */
      D = memrdr(PC8++);
      t = 7;
      break;

    case 0x17:			/* RAL */
      old_c_flag = F & C_FLAG;
      (A & 128) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A <<= 1;
      if (old_c_flag) A |= 1;
      t = 4;
      break;

    case 0x18:			/* NOP* */
      t = 4;
      break;

    case 0x19:			/* DAD D */
      carry = (L + E > 255) ? 1 : 0;
      L += E;
      (H + D + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      H += D + carry;
      t = 10;
      break;

    case 0x1a:			/* LDAX D */
      A = memrdr((D << 8) + E);
      t = 7;
      break;

    case 0x1b:			/* DCX D */
      E--;
      if (E == 0xff) D--;
      t = 5;
      break;

    case 0x1c:			/* INR E */
      E++;
      ((E & 0xf) == 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      F = (F & ~SZP_FLAGS) | szp_flags[E];
      t = 5;
      break;

    case 0x1d:			/* DCR E */
      E--;
      ((E & 0xf) == 0xf) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      F = (F & ~SZP_FLAGS) | szp_flags[E];
      t = 5;
      break;

    case 0x1e:			/* MVI E,n */
      E = memrdr(PC8++);
      t = 7;
      break;

    case 0x1f:			/* RAR */
      old_c_flag = F & C_FLAG;
      i = A & 1;
      (i) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A >>= 1;
      if (old_c_flag) A |= 128;
      t = 4;
      break;

    case 0x20:			/* NOP* */
      t = 4;
      break;

    case 0x21:			/* LXI H,nn */
      L = memrdr(PC8++);
      H = memrdr(PC8++);
      t = 10;
      break;

    case 0x22:			/* SHLD nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      memwrt(addr, L);
      memwrt(addr + 1, H);
      t = 16;
      break;

    case 0x23:			/* INX H */
      L++;
      if (!L) H++;
      t = 5;
      break;

    case 0x24:			/* INR H */
      H++;
      ((H & 0xf) == 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      F = (F & ~SZP_FLAGS) | szp_flags[H];
      t = 5;
      break;

    case 0x25:			/* DCR H */
      H--;
      ((H & 0xf) == 0xf) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      F = (F & ~SZP_FLAGS) | szp_flags[H];
      t = 5;
      break;

    case 0x26:			/* MVI H,n */
      H = memrdr(PC8++);
      t = 7;
      break;

    case 0x27:			/* DAA */
      i = A;
      if (((A & 0xf) > 9) || (F & H_FLAG)) {
        ((A & 0xf) > 9) ? (F |= H_FLAG) : (F &= ~H_FLAG);
        i += 6;
      }
      if (((i & 0x1f0) > 0x90) || (F & C_FLAG)) {
        i += 0x60;
      }
      if (i & 0x100) (F |= C_FLAG);
      A = i & 0xff;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x28:			/* NOP* */
      t = 4;
      break;

    case 0x29:			/* DAD H */
      carry = (L << 1 > 255) ? 1 : 0;
      L <<= 1;
      (H + H + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      H += H + carry;
      t = 10;
      break;

    case 0x2a:			/* LHLD nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      L = memrdr(addr);
      H = memrdr(addr + 1);
      t = 16;
      break;

    case 0x2b:			/* DCX H */
      L--;
      if (L == 0xff) H--;
      t = 5;
      break;

    case 0x2c:			/* INR L */
      L++;
      ((L & 0xf) == 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      F = (F & ~SZP_FLAGS) | szp_flags[L];
      t = 5;
      break;

    case 0x2d:			/* DCR L */
      L--;
      ((L & 0xf) == 0xf) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      F = (F & ~SZP_FLAGS) | szp_flags[L];
      t = 5;
      break;

    case 0x2e:			/* MVI L,n */
      L = memrdr(PC8++);
      t = 7;
      break;

    case 0x2f:			/* CMA */
      A = ~A;
      t = 4;
      break;

    case 0x30:			/* NOP* */
      t = 4;
      break;

    case 0x31:			/* LXI SP,nn */
      SP8 = memrdr(PC8++);
      SP8 += memrdr(PC8++) << 8;
      t = 10;
      break;

    case 0x32:			/* STA nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      memwrt(addr, A);
      t = 13;
      break;

    case 0x33:			/* INX SP */
      SP8++;
      t = 5;
      break;

    case 0x34:			/* INR M */
      addr = (H << 8) + L;
      P = memrdr(addr);
      P++;
      memwrt(addr, P);
      ((P & 0xf) == 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      F = (F & ~SZP_FLAGS) | szp_flags[P];
      t = 10;
      break;

    case 0x35:			/* DCR M */
      addr = (H << 8) + L;
      P = memrdr(addr);
      P--;
      memwrt(addr, P);
      ((P & 0xf) == 0xf) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      F = (F & ~SZP_FLAGS) | szp_flags[P];
      t = 10;
      break;

    case 0x36:			/* MVI M,n */
      memwrt((H << 8) + L, memrdr(PC8++));
      t = 10;
      break;

    case 0x37:			/* STC */
      F |= C_FLAG;
      t = 4;
      break;

    case 0x38:			/* NOP* */
      t = 4;
      break;

    case 0x39:			/* DAD SP */
      spl = SP8 & 0xff;
      sph = SP8 >> 8;
      carry = (L + spl > 255) ? 1 : 0;
      L += spl;
      (H + sph + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      H += sph + carry;
      t = 10;
      break;

    case 0x3a:			/* LDA nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      A = memrdr(addr);
      t = 13;
      break;

    case 0x3b:			/* DCX SP */
      SP8--;
      t = 5;
      break;

    case 0x3c:			/* INR A */
      A++;
      ((A & 0xf) == 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 5;
      break;

    case 0x3d:			/* DCR A */
      A--;
      ((A & 0xf) == 0xf) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 5;
      break;

    case 0x3e:			/* MVI A,n */
      A = memrdr(PC8++);
      t = 7;
      break;

    case 0x3f:			/* CMC */
      if (F & C_FLAG) F &= ~C_FLAG;
      else F |= C_FLAG;
      t = 4;
      break;

    case 0x40:			/* MOV B,B */
      t = 5;
      break;

    case 0x41:			/* MOV B,C */
      B = C;
      t = 5;
      break;

    case 0x42:			/* MOV B,D */
      B = D;
      t = 5;
      break;

    case 0x43:			/* MOV B,E */
      B = E;
      t = 5;
      break;

    case 0x44:			/* MOV B,H */
      B = H;
      t = 5;
      break;

    case 0x45:			/* MOV B,L */
      B = L;
      t = 5;
      break;

    case 0x46:			/* MOV B,M */
      B = memrdr((H << 8) + L);
      t = 7;
      break;

    case 0x47:			/* MOV B,A */
      B = A;
      t = 5;
      break;

    case 0x48:			/* MOV C,B */
      C = B;
      t = 5;
      break;

    case 0x49:			/* MOV C,C */
      t = 5;
      break;

    case 0x4a:			/* MOV C,D */
      C = D;
      t = 5;
      break;

    case 0x4b:			/* MOV C,E */
      C = E;
      t = 5;
      break;

    case 0x4c:			/* MOV C,H */
      C = H;
      t = 5;
      break;

    case 0x4d:			/* MOV C,L */
      C = L;
      t = 5;
      break;

    case 0x4e:			/* MOV C,M */
      C = memrdr((H << 8) + L);
      t = 7;
      break;

    case 0x4f:			/* MOV C,A */
      C = A;
      t = 5;
      break;

    case 0x50:			/* MOV D,B */
      D = B;
      t = 5;
      break;

    case 0x51:			/* MOV D,C */
      D = C;
      t = 5;
      break;

    case 0x52:			/* MOV D,D */
      t = 5;
      break;

    case 0x53:			/* MOV D,E */
      D = E;
      t = 5;
      break;

    case 0x54:			/* MOV D,H */
      D = H;
      t = 5;
      break;

    case 0x55:			/* MOV D,L */
      D = L;
      t = 5;
      break;

    case 0x56:			/* MOV D,M */
      D = memrdr((H << 8) + L);
      t = 7;
      break;

    case 0x57:			/* MOV D,A */
      D = A;
      t = 5;
      break;

    case 0x58:			/* MOV E,B */
      E = B;
      t = 5;
      break;

    case 0x59:			/* MOV E,C */
      E = C;
      t = 5;
      break;

    case 0x5a:			/* MOV E,D */
      E = D;
      t = 5;
      break;

    case 0x5b:			/* MOV E,E */
      t = 5;
      break;

    case 0x5c:			/* MOV E,H */
      E = H;
      t = 5;
      break;

    case 0x5d:			/* MOV E,L */
      E = L;
      t = 5;
      break;

    case 0x5e:			/* MOV E,M */
      E = memrdr((H << 8) + L);
      t = 7;
      break;

    case 0x5f:			/* MOV E,A */
      E = A;
      t = 5;
      break;

    case 0x60:			/* MOV H,B */
      H = B;
      t = 5;
      break;

    case 0x61:			/* MOV H,C */
      H = C;
      t = 5;
      break;

    case 0x62:			/* MOV H,D */
      H = D;
      t = 5;
      break;

    case 0x63:			/* MOV H,E */
      H = E;
      t = 5;
      break;

    case 0x64:			/* MOV H,H */
      t = 5;
      break;

    case 0x65:			/* MOV H,L */
      H = L;
      t = 5;
      break;

    case 0x66:			/* MOV H,M */
      H = memrdr((H << 8) + L);
      t = 7;
      break;

    case 0x67:			/* MOV H,A */
      H = A;
      t = 5;
      break;

    case 0x68:			/* MOV L,B */
      L = B;
      t = 5;
      break;

    case 0x69:			/* MOV L,C */
      L = C;
      t = 5;
      break;

    case 0x6a:			/* MOV L,D */
      L = D;
      t = 5;
      break;

    case 0x6b:			/* MOV L,E */
      L = E;
      t = 5;
      break;

    case 0x6c:			/* MOV L,H */
      L = H;
      t = 5;
      break;

    case 0x6d:			/* MOV L,L */
      t = 5;
      break;

    case 0x6e:			/* MOV L,M */
      L = memrdr((H << 8) + L);
      t = 7;
      break;

    case 0x6f:			/* MOV L,A */
      L = A;
      t = 5;
      break;

    case 0x70:			/* MOV M,B */
      memwrt((H << 8) + L, B);
      t = 7;
      break;

    case 0x71:			/* MOV M,C */
      memwrt((H << 8) + L, C);
      t = 7;
      break;

    case 0x72:			/* MOV M,D */
      memwrt((H << 8) + L, D);
      t = 7;
      break;

    case 0x73:			/* MOV M,E */
      memwrt((H << 8) + L, E);
      t = 7;
      break;

    case 0x74:			/* MOV M,H */
      memwrt((H << 8) + L, H);
      t = 7;
      break;

    case 0x75:			/* MOV M,L */
      memwrt((H << 8) + L, L);
      t = 7;
      break;

    case 0x76:			/* HLT */
      // cry and die, no interrupts yet
      Serial.println();
      Serial.print(F("HLT @ PC = 0x"));
      Serial.println(PC8, HEX);
      State = Halted;
      t = 7;
      break;

    case 0x77:			/* MOV M,A */
      memwrt((H << 8) + L, A);
      t = 7;
      break;

    case 0x78:			/* MOV A,B */
      A = B;
      t = 5;
      break;

    case 0x79:			/* MOV A,C */
      A = C;
      t = 5;
      break;

    case 0x7a:			/* MOV A,D */
      A = D;
      t = 5;
      break;

    case 0x7b:			/* MOV A,E */
      A = E;
      t = 5;
      break;

    case 0x7c:			/* MOV A,H */
      A = H;
      t = 5;
      break;

    case 0x7d:			/* MOV A,L */
      A = L;
      t = 5;
      break;

    case 0x7e:			/* MOV A,M */
      A = memrdr((H << 8) + L);
      t = 7;
      break;

    case 0x7f:			/* MOV A,A */
      t = 5;
      break;

    case 0x80:			/* ADD B */
      ((A & 0xf) + (B & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      (A + B > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A + B;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x81:			/* ADD C */
      ((A & 0xf) + (C & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      (A + C > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A + C;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x82:			/* ADD D */
      ((A & 0xf) + (D & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      (A + D > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A + D;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x83:			/* ADD E */
      ((A & 0xf) + (E & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      (A + E > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A + E;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x84:			/* ADD H */
      ((A & 0xf) + (H & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      (A + H > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A + H;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x85:			/* ADD L */
      ((A & 0xf) + (L & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      (A + L > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A + L;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x86:			/* ADD M */
      P = memrdr((H << 8) + L);
      ((A & 0xf) + (P & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      (A + P > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A + P;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 7;
      break;

    case 0x87:			/* ADD A */
      ((A & 0xf) + (A & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      ((A << 1) > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A << 1;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x88:			/* ADC B */
      carry = (F & C_FLAG) ? 1 : 0;
      ((A & 0xf) + (B & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      (A + B + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A + B + carry;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x89:			/* ADC C */
      carry = (F & C_FLAG) ? 1 : 0;
      ((A & 0xf) + (C & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      (A + C + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A + C + carry;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x8a:			/* ADC D */
      carry = (F & C_FLAG) ? 1 : 0;
      ((A & 0xf) + (D & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      (A + D + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A + D + carry;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x8b:			/* ADC E */
      carry = (F & C_FLAG) ? 1 : 0;
      ((A & 0xf) + (E & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      (A + E + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A + E + carry;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x8c:			/* ADC H */
      carry = (F & C_FLAG) ? 1 : 0;
      ((A & 0xf) + (H & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      (A + H + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A + H + carry;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x8d:			/* ADC L */
      carry = (F & C_FLAG) ? 1 : 0;
      ((A & 0xf) + (L & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      (A + L + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A + L + carry;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x8e:			/* ADC M */
      P = memrdr((H << 8) + L);
      carry = (F & C_FLAG) ? 1 : 0;
      ((A & 0xf) + (P & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      (A + P + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A + P + carry;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 7;
      break;

    case 0x8f:			/* ADC A */
      carry = (F & C_FLAG) ? 1 : 0;
      ((A & 0xf) + (A & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      ((A << 1) + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = (A << 1) + carry;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x90:			/* SUB B */
      ((B & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (B > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A - B;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x91:			/* SUB C */
      ((C & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (C > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A - C;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x92:			/* SUB D */
      ((D & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (D > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A - D;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x93:			/* SUB E */
      ((E & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (E > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A - E;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x94:			/* SUB H */
      ((H & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (H > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A - H;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x95:			/* SUB L */
      ((L & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (L > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A - L;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x96:			/* SUB M */
      P = memrdr((H << 8) + L);
      ((P & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (P > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A - P;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 7;
      break;

    case 0x97:			/* SUB A */
      A = 0;
      F &= ~(S_FLAG | C_FLAG);
      F |= Z_FLAG | H_FLAG | P_FLAG;
      t = 4;
      break;

    case 0x98:			/* SBB B */
      carry = (F & C_FLAG) ? 1 : 0;
      ((B & 0xf) + carry > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (B + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A - B - carry;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x99:			/* SBB C */
      carry = (F & C_FLAG) ? 1 : 0;
      ((C & 0xf) + carry > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (C + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A - C - carry;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x9a:			/* SBB D */
      carry = (F & C_FLAG) ? 1 : 0;
      ((D & 0xf) + carry > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (D + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A - D - carry;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x9b:			/* SBB E */
      carry = (F & C_FLAG) ? 1 : 0;
      ((E & 0xf) + carry > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (E + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A - E - carry;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x9c:			/* SBB H */
      carry = (F & C_FLAG) ? 1 : 0;
      ((H & 0xf) + carry > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (H + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A - H - carry;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x9d:			/* SBB L */
      carry = (F & C_FLAG) ? 1 : 0;
      ((L & 0xf) + carry > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (L + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A - L - carry;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 4;
      break;

    case 0x9e:			/* SBB M */
      P = memrdr((H << 8) + L);
      carry = (F & C_FLAG) ? 1 : 0;
      ((P & 0xf) + carry > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (P + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A - P - carry;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 7;
      break;

    case 0x9f:			/* SBB A */
      if (F & C_FLAG) {
        A = 255;
        F |= S_FLAG | C_FLAG | P_FLAG;
        F &= ~(Z_FLAG | H_FLAG);
      } else {
        A = 0;
        F |= Z_FLAG | H_FLAG | P_FLAG;
        F &= ~(S_FLAG | C_FLAG);
      }
      t = 4;
      break;

    case 0xa0:			/* ANA B */
      ((A | B) & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      A &= B;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~C_FLAG;
      t = 4;
      break;

    case 0xa1:			/* ANA C */
      ((A | C) & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      A &= C;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~C_FLAG;
      t = 4;
      break;

    case 0xa2:			/* ANA D */
      ((A | D) & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      A &= D;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~C_FLAG;
      t = 4;
      break;

    case 0xa3:			/* ANA E */
      ((A | E) & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      A &= E;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~C_FLAG;
      t = 4;
      break;

    case 0xa4:			/* ANA H */
      ((A | H) & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      A &= H;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~C_FLAG;
      t = 4;
      break;

    case 0xa5:			/* ANA L */
      ((A | L) & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      A &= L;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~C_FLAG;
      t = 4;
      break;

    case 0xa6:			/* ANA M */
      P = memrdr((H << 8) + L);
      ((A | P) & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      A &= P;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~C_FLAG;
      t = 7;
      break;

    case 0xa7:			/* ANA A */
      (A & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~C_FLAG;
      t = 4;
      break;

    case 0xa8:			/* XRA B */
      A ^= B;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~(H_FLAG | C_FLAG);
      t = 4;
      break;

    case 0xa9:			/* XRA C */
      A ^= C;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~(H_FLAG | C_FLAG);
      t = 4;
      break;

    case 0xaa:			/* XRA D */
      A ^= D;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~(H_FLAG | C_FLAG);
      t = 4;
      break;

    case 0xab:			/* XRA E */
      A ^= E;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~(H_FLAG | C_FLAG);
      t = 4;
      break;

    case 0xac:			/* XRA H */
      A ^= H;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~(H_FLAG | C_FLAG);
      t = 4;
      break;

    case 0xad:			/* XRA L */
      A ^= L;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~(H_FLAG | C_FLAG);
      t = 4;
      break;

    case 0xae:			/* XRA M */
      A ^= memrdr((H << 8) + L);
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~(H_FLAG | C_FLAG);
      t = 7;
      break;

    case 0xaf:			/* XRA A */
      A = 0;
      F &= ~(S_FLAG | H_FLAG | C_FLAG);
      F |= Z_FLAG | P_FLAG;
      t = 4;
      break;

    case 0xb0:			/* ORA B */
      A |= B;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~(C_FLAG | H_FLAG);
      t = 4;
      break;

    case 0xb1:			/* ORA C */
      A |= C;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~(C_FLAG | H_FLAG);
      t = 4;
      break;

    case 0xb2:			/* ORA D */
      A |= D;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~(C_FLAG | H_FLAG);
      t = 4;
      break;

    case 0xb3:			/* ORA E */
      A |= E;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~(C_FLAG | H_FLAG);
      t = 4;
      break;

    case 0xb4:			/* ORA H */
      A |= H;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~(C_FLAG | H_FLAG);
      t = 4;
      break;

    case 0xb5:			/* ORA L */
      A |= L;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~(C_FLAG | H_FLAG);
      t = 4;
      break;

    case 0xb6:			/* ORA M */
      A |= memrdr((H << 8) + L);
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~(C_FLAG | H_FLAG);
      t = 7;
      break;

    case 0xb7:			/* ORA A */
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~(C_FLAG | H_FLAG);
      t = 4;
      break;

    case 0xb8:			/* CMP B */
      ((B & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (B > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      tmp = A - B;
      F = (F & ~SZP_FLAGS) | szp_flags[tmp];
      t = 4;
      break;

    case 0xb9:			/* CMP C */
      ((C & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (C > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      tmp = A - C;
      F = (F & ~SZP_FLAGS) | szp_flags[tmp];
      t = 4;
      break;

    case 0xba:			/* CMP D */
      ((D & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (D > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      tmp = A - D;
      F = (F & ~SZP_FLAGS) | szp_flags[tmp];
      t = 4;
      break;

    case 0xbb:			/* CMP E */
      ((E & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (E > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      tmp = A - E;
      F = (F & ~SZP_FLAGS) | szp_flags[tmp];
      t = 4;
      break;

    case 0xbc:			/* CMP H */
      ((H & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (H > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      tmp = A - H;
      F = (F & ~SZP_FLAGS) | szp_flags[tmp];
      t = 4;
      break;

    case 0xbd:			/* CMP L */
      ((L & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (L > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      tmp = A - L;
      F = (F & ~SZP_FLAGS) | szp_flags[tmp];
      t = 4;
      break;

    case 0xbe:			/* CMP M */
      P = memrdr((H << 8) + L);
      ((P & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (P > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      tmp = A - P;
      F = (F & ~SZP_FLAGS) | szp_flags[tmp];
      t = 7;
      break;

    case 0xbf:			/* CMP A */
      F &= ~(S_FLAG | C_FLAG);
      F |= Z_FLAG | H_FLAG | P_FLAG;
      t = 4;
      break;

    case 0xc0:			/* RNZ */
      t = 5;
      if (!(F & Z_FLAG)) {
        addr = memrdr(SP8++);
        addr += memrdr(SP8++) << 8;
        PC8 = addr;
        t += 6;
      }
      break;

    case 0xc1:			/* POP B */
      C = memrdr(SP8++);
      B = memrdr(SP8++);
      t = 10;
      break;

    case 0xc2:			/* JNZ nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      if (!(F & Z_FLAG))
        PC8 = addr;
      t = 10;
      break;

    case 0xc3:			/* JMP nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8) << 8;
      PC8 = addr;
      t = 10;
      break;

    case 0xc4:			/* CNZ nn */
      t = 11;
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      if (!(F & Z_FLAG)) {
        memwrt(--SP8, PC8 >> 8);
        memwrt(--SP8, PC8);
        PC8 = addr;
        t += 6;
      }
      break;

    case 0xc5:			/* PUSH B */
      memwrt(--SP8, B);
      memwrt(--SP8, C);
      t = 11;
      break;

    case 0xc6:			/* ADI n */
      P = memrdr(PC8++);
      ((A & 0xf) + (P & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      (A + P > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A + P;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 7;
      break;

    case 0xc7:			/* RST 0 */
      memwrt(--SP8, PC8 >> 8);
      memwrt(--SP8, PC8);
      PC8 = 0;
      t = 11;
      break;

    case 0xc8:			/* RZ */
      t = 5;
      if (F & Z_FLAG) {
        addr = memrdr(SP8++);
        addr += memrdr(SP8++) << 8;
        PC8 = addr;
        t += 6;
      }
      break;

    case 0xc9:			/* RET */
      addr = memrdr(SP8++);
      addr += memrdr(SP8++) << 8;
      PC8 = addr;
      t = 10;
      break;

    case 0xca:			/* JZ nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      if (F & Z_FLAG)
        PC8 = addr;
      t = 10;
      break;

    case 0xcb:			/* JMP* nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8) << 8;
      PC8 = addr;
      t = 10;
      break;

    case 0xcc:			/* CZ nn */
      t = 11;
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      if (F & Z_FLAG) {
        memwrt(--SP8, PC8 >> 8);
        memwrt(--SP8, PC8);
        PC8 = addr;
        t += 6;
      }
      break;

    case 0xcd:			/* CALL nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      memwrt(--SP8, PC8 >> 8);
      memwrt(--SP8, PC8);
      PC8 = addr;
      t = 17;
      break;

    case 0xce:			/* ACI n */
      carry = (F & C_FLAG) ? 1 : 0;
      P = memrdr(PC8++);
      ((A & 0xf) + (P & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      (A + P + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A + P + carry;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 7;
      break;

    case 0xcf:			/* RST 1 */
      memwrt(--SP8, PC8 >> 8);
      memwrt(--SP8, PC8);
      PC8 = 0x08;
      t = 11;
      break;

    case 0xd0:			/* RNC */
      t = 5;
      if (!(F & C_FLAG)) {
        addr = memrdr(SP8++);
        addr += memrdr(SP8++) << 8;
        PC8 = addr;
        t += 6;
      }
      break;

    case 0xd1:			/* POP D */
      E = memrdr(SP8++);
      D = memrdr(SP8++);
      t = 10;
      break;

    case 0xd2:			/* JNC nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      if (!(F & C_FLAG))
        PC8 = addr;
      t = 10;
      break;

    case 0xd3:			/* OUT n */
      P = memrdr(PC8++);
      io_out(P, P, A);
      t = 10;
      break;

    case 0xd4:			/* CNC nn */
      t = 11;
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      if (!(F & C_FLAG)) {
        memwrt(--SP8, PC8 >> 8);
        memwrt(--SP8, PC8);
        PC8 = addr;
        t += 6;
      }
      break;

    case 0xd5:			/* PUSH D */
      memwrt(--SP8, D);
      memwrt(--SP8, E);
      t = 11;
      break;

    case 0xd6:			/* SUI n */
      P = memrdr(PC8++);
      ((P & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (P > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A - P;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 7;
      break;

    case 0xd7:			/* RST 2 */
      memwrt(--SP8, PC8 >> 8);
      memwrt(--SP8, PC8);
      PC8 = 0x10;
      t = 11;
      break;

    case 0xd8:			/* RC */
      t = 5;
      if (F & C_FLAG) {
        addr = memrdr(SP8++);
        addr += memrdr(SP8++) << 8;
        PC8 = addr;
        t += 6;
      }
      break;

    case 0xd9:			/* RET* */
      addr = memrdr(SP8++);
      addr += memrdr(SP8++) << 8;
      PC8 = addr;
      t = 10;
      break;

    case 0xda:			/* JC nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      if (F & C_FLAG)
        PC8 = addr;
      t = 10;
      break;

    case 0xdb:			/* IN n */
      P = memrdr(PC8++);
      A = io_in(P, P);
      t = 10;
      break;

    case 0xdc:			/* CC nn */
      t = 11;
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      if (F & C_FLAG) {
        memwrt(--SP8, PC8 >> 8);
        memwrt(--SP8, PC8);
        PC8 = addr;
        t += 6;
      }
      break;

    case 0xdd:			/* CALL* nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      memwrt(--SP8, PC8 >> 8);
      memwrt(--SP8, PC8);
      PC8 = addr;
      t = 17;
      break;

    case 0xde:			/* SBI n */
      P = memrdr(PC8++);
      carry = (F & C_FLAG) ? 1 : 0;
      ((P & 0xf) + carry > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (P + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      A = A - P - carry;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      t = 7;
      break;

    case 0xdf:			/* RST 3 */
      memwrt(--SP8, PC8 >> 8);
      memwrt(--SP8, PC8);
      PC8 = 0x18;
      t = 11;
      break;

    case 0xe0:			/* RPO */
      t = 5;
      if (!(F & P_FLAG)) {
        addr = memrdr(SP8++);
        addr += memrdr(SP8++) << 8;
        PC8 = addr;
        t += 6;
      }
      break;

    case 0xe1:			/* POP H */
      L = memrdr(SP8++);
      H = memrdr(SP8++);
      t = 10;
      break;

    case 0xe2:			/* JPO nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      if (!(F & P_FLAG))
        PC8 = addr;
      t = 10;
      break;

    case 0xe3:			/* XTHL */
      P = memrdr(SP8);
      memwrt(SP8, L);
      L = P;
      P = memrdr(SP8 + 1);
      memwrt(SP8 + 1, H);
      H = P;
      t = 18;
      break;

    case 0xe4:			/* CPO nn */
      t = 11;
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      if (!(F & P_FLAG)) {
        memwrt(--SP8, PC8 >> 8);
        memwrt(--SP8, PC8);
        PC8 = addr;
        t += 6;
      }
      break;

    case 0xe5:			/* PUSH H */
      memwrt(--SP8, H);
      memwrt(--SP8, L);
      t = 11;
      break;

    case 0xe6:			/* ANI n */
      P = memrdr(PC8++);
      ((A | P) & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
      A &= P;
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~C_FLAG;
      t = 7;
      break;

    case 0xe7:			/* RST 4 */
      memwrt(--SP8, PC8 >> 8);
      memwrt(--SP8, PC8);
      PC8 = 0x20;
      t = 11;
      break;

    case 0xe8:			/* RPE */
      t = 5;
      if (F & P_FLAG) {
        addr = memrdr(SP8++);
        addr += memrdr(SP8++) << 8;
        PC8 = addr;
        t += 6;
      }
      break;

    case 0xe9:			/* PCHL */
      PC8 = (H << 8) + L;
      t = 5;
      break;

    case 0xea:			/* JPE nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      if (F & P_FLAG)
        PC8 = addr;
      t = 10;
      break;

    case 0xeb:			/* XCHG */
      P = D;
      D = H;
      H = P;
      P = E;
      E = L;
      L = P;
      t = 4;
      break;

    case 0xec:			/* CPE nn */
      t = 11;
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      if (F & P_FLAG) {
        memwrt(--SP8, PC8 >> 8);
        memwrt(--SP8, PC8);
        PC8 = addr;
        t += 6;
      }
      break;

    case 0xed:			/* CALL* nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      memwrt(--SP8, PC8 >> 8);
      memwrt(--SP8, PC8);
      PC8 = addr;
      t = 17;
      break;

    case 0xee:			/* XRI n */
      A ^= memrdr(PC8++);
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~(H_FLAG | C_FLAG);
      t = 7;
      break;

    case 0xef:			/* RST 5 */
      memwrt(--SP8, PC8 >> 8);
      memwrt(--SP8, PC8);
      PC8 = 0x28;
      t = 11;
      break;

    case 0xf0:			/* RP */
      t = 5;
      if (!(F & S_FLAG)) {
        addr = memrdr(SP8++);
        addr += memrdr(SP8++) << 8;
        PC8 = addr;
        t += 6;
      }
      break;

    case 0xf1:			/* POP PSW */
      F = memrdr(SP8++);
      F &= ~(Y_FLAG | X_FLAG);
      F |= N_FLAG;
      A = memrdr(SP8++);
      t = 10;
      break;

    case 0xf2:			/* JP nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      if (!(F & S_FLAG))
        PC8 = addr;
      t = 10;
      break;

    case 0xf3:			/* DI */
      IFF = 0;
      t = 4;
      break;

    case 0xf4:			/* CP nn */
      t = 11;
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      if (!(F & S_FLAG)) {
        memwrt(--SP8, PC8 >> 8);
        memwrt(--SP8, PC8);
        PC8 = addr;
        t += 6;
      }
      break;

    case 0xf5:			/* PUSH PSW */
      memwrt(--SP8, A);
      memwrt(--SP8, F);
      t = 11;
      break;

    case 0xf6:			/* ORI n */
      A |= memrdr(PC8++);
      F = (F & ~SZP_FLAGS) | szp_flags[A];
      F &= ~(C_FLAG | H_FLAG);
      t = 7;
      break;

    case 0xf7:			/* RST 6 */
      memwrt(--SP8, PC8 >> 8);
      memwrt(--SP8, PC8);
      PC8 = 0x30;
      t = 11;
      break;

    case 0xf8:			/* RM */
      t = 5;
      if (F & S_FLAG) {
        addr = memrdr(SP8++);
        addr += memrdr(SP8++) << 8;
        PC8 = addr;
        t += 6;
      }
      break;

    case 0xf9:			/* SPHL */
      SP8 = (H << 8) + L;
      t = 5;
      break;

    case 0xfa:			/* JM nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      if (F & S_FLAG)
        PC8 = addr;
      t = 10;
      break;

    case 0xfb:			/* EI */
      IFF = 3;
      t = 4;
      break;

    case 0xfc:			/* CM nn */
      t = 11;
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      if (F & S_FLAG) {
        memwrt(--SP8, PC8 >> 8);
        memwrt(--SP8, PC8);
        PC8 = addr;
        t += 6;
      }
      break;

    case 0xfd:			/* CALL* nn */
      addr = memrdr(PC8++);
      addr += memrdr(PC8++) << 8;
      memwrt(--SP8, PC8 >> 8);
      memwrt(--SP8, PC8);
      PC8 = addr;
      t = 17;
      break;

    case 0xfe:			/* CPI n */
      P = memrdr(PC8++);
      ((P & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
      (P > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
      tmp = A - P;
      F = (F & ~SZP_FLAGS) | szp_flags[tmp];
      t = 7;
      break;

    case 0xff:			/* RST 7 */
      memwrt(--SP8, PC8 >> 8);
      memwrt(--SP8, PC8);
      PC8 = 0x38;
      t = 11;
      break;

    default:
      t = 0;        /* silence compiler */
      break;
    }

    tstates += t; // account for the executed instruction

  } while (State == Running);
}

#ifdef DEBUG
// from the Arduino Memory Guide
void display_freeram()
{
  Serial.print(F("SRAM left: "));
  Serial.println(freeRam());
}

int freeRam()
{
  extern int __heap_start, *__brkval;
  int v;

  return (int) &v - (__brkval == 0 ?  
         (int) &__heap_start : (int) __brkval);  
}
#endif

void setup()
{
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  while (!Serial)
    ; // Wait for serial port to connect. Needed for native USB
  randomSeed(analogRead(UAP));

  SPI.setClockDivider(SPI_CLOCK_DIV2);

  if (!fram.begin()) {
    Serial.println(F("No FRAM found"));
    exit(1);
  }
  fram.writeEnable(true);
  
  if (!SD.begin(SD_CS)) {
    Serial.println(F("SD failed"));
    exit(1);
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
  // variables for measuring the run time
  unsigned long start, stop;

  Serial.println(F("\f8080-SIM v1.5\n"));

  init_cpu();
  init_memory();
  config();

  // run the 8080 CPU with whatever code is in memory
  start = millis();
  cpu_8080();
  stop = millis();
  
  // 8080 CPU stopped working, signal this on builtin LED
  digitalWrite(LED_BUILTIN, true);

  // print some execution statistics
  Serial.println();
  Serial.print(F("8080 ran "));
  Serial.print(stop - start, DEC);
  Serial.print(F(" ms and executed "));
  Serial.print(tstates, DEC);
  Serial.println(F(" t-states"));
  Serial.print(F("Clock frequency "));
  Serial.print(float(tstates) / float(stop - start) / 1000.0, 2);
  Serial.println(F(" MHz"));
#ifdef DEBUG
  display_freeram();
#endif
  Serial.println();
  Serial.flush();
  exit(0);
}
