//
// Intel 8080 CPU emulation on an Arduino Nano
//            derived from Z80PACK
// Copyright 2024, Udo Munk
//
// History:
// 04-MAY-2024 Release 1.0
// 06-MAY-2024 Release 1.1, add support for a ROM in flash
//

// unused analog pin to seed random generator
#define UAP 7

// data types for the 8080 CPU
typedef unsigned char BYTE;
typedef unsigned int  WORD;

// project includes
#include "simcore.h"
#include "memory.h"
#include "iosim.h"

// 8080 CPU registers
BYTE A, B, C, D, E, H, L, F, IFF;
WORD PC8, SP8; // funny names because SP already is a macro here

// other global variables
CPUState State = Running;         // CPU state
unsigned long tstates = 0;        // executed T-states

// Precompiled parity table
const byte PROGMEM parity[256] = {
		0 /* 00000000 */, 1 /* 00000001 */, 1 /* 00000010 */,
		0 /* 00000011 */, 1 /* 00000100 */, 0 /* 00000101 */,
		0 /* 00000110 */, 1 /* 00000111 */, 1 /* 00001000 */,
		0 /* 00001001 */, 0 /* 00001010 */, 1 /* 00001011 */,
		0 /* 00001100 */, 1 /* 00001101 */, 1 /* 00001110 */,
		0 /* 00001111 */, 1 /* 00010000 */, 0 /* 00010001 */,
		0 /* 00010010 */, 1 /* 00010011 */, 0 /* 00010100 */,
		1 /* 00010101 */, 1 /* 00010110 */, 0 /* 00010111 */,
		0 /* 00011000 */, 1 /* 00011001 */, 1 /* 00011010 */,
		0 /* 00011011 */, 1 /* 00011100 */, 0 /* 00011101 */,
		0 /* 00011110 */, 1 /* 00011111 */, 1 /* 00100000 */,
		0 /* 00100001 */, 0 /* 00100010 */, 1 /* 00100011 */,
		0 /* 00100100 */, 1 /* 00100101 */, 1 /* 00100110 */,
		0 /* 00100111 */, 0 /* 00101000 */, 1 /* 00101001 */,
		1 /* 00101010 */, 0 /* 00101011 */, 1 /* 00101100 */,
		0 /* 00101101 */, 0 /* 00101110 */, 1 /* 00101111 */,
		0 /* 00110000 */, 1 /* 00110001 */, 1 /* 00110010 */,
		0 /* 00110011 */, 1 /* 00110100 */, 0 /* 00110101 */,
		0 /* 00110110 */, 1 /* 00110111 */, 1 /* 00111000 */,
		0 /* 00111001 */, 0 /* 00111010 */, 1 /* 00111011 */,
		0 /* 00111100 */, 1 /* 00111101 */, 1 /* 00111110 */,
		0 /* 00111111 */, 1 /* 01000000 */, 0 /* 01000001 */,
		0 /* 01000010 */, 1 /* 01000011 */, 0 /* 01000100 */,
		1 /* 01000101 */, 1 /* 01000110 */, 0 /* 01000111 */,
		0 /* 01001000 */, 1 /* 01001001 */, 1 /* 01001010 */,
		0 /* 01001011 */, 1 /* 01001100 */, 0 /* 01001101 */,
		0 /* 01001110 */, 1 /* 01001111 */, 0 /* 01010000 */,
		1 /* 01010001 */, 1 /* 01010010 */, 0 /* 01010011 */,
		1 /* 01010100 */, 0 /* 01010101 */, 0 /* 01010110 */,
		1 /* 01010111 */, 1 /* 01011000 */, 0 /* 01011001 */,
		0 /* 01011010 */, 1 /* 01011011 */, 0 /* 01011100 */,
		1 /* 01011101 */, 1 /* 01011110 */, 0 /* 01011111 */,
		0 /* 01100000 */, 1 /* 01100001 */, 1 /* 01100010 */,
		0 /* 01100011 */, 1 /* 01100100 */, 0 /* 01100101 */,
		0 /* 01100110 */, 1 /* 01100111 */, 1 /* 01101000 */,
		0 /* 01101001 */, 0 /* 01101010 */, 1 /* 01101011 */,
		0 /* 01101100 */, 1 /* 01101101 */, 1 /* 01101110 */,
		0 /* 01101111 */, 1 /* 01110000 */, 0 /* 01110001 */,
		0 /* 01110010 */, 1 /* 01110011 */, 0 /* 01110100 */,
		1 /* 01110101 */, 1 /* 01110110 */, 0 /* 01110111 */,
		0 /* 01111000 */, 1 /* 01111001 */, 1 /* 01111010 */,
		0 /* 01111011 */, 1 /* 01111100 */, 0 /* 01111101 */,
		0 /* 01111110 */, 1 /* 01111111 */,
		1 /* 10000000 */, 0 /* 10000001 */, 0 /* 10000010 */,
		1 /* 10000011 */, 0 /* 10000100 */, 1 /* 10000101 */,
		1 /* 10000110 */, 0 /* 10000111 */, 0 /* 10001000 */,
		1 /* 10001001 */, 1 /* 10001010 */, 0 /* 10001011 */,
		1 /* 10001100 */, 0 /* 10001101 */, 0 /* 10001110 */,
		1 /* 10001111 */, 0 /* 10010000 */, 1 /* 10010001 */,
		1 /* 10010010 */, 0 /* 10010011 */, 1 /* 10010100 */,
		0 /* 10010101 */, 0 /* 10010110 */, 1 /* 10010111 */,
		1 /* 10011000 */, 0 /* 10011001 */, 0 /* 10011010 */,
		1 /* 10011011 */, 0 /* 10011100 */, 1 /* 10011101 */,
		1 /* 10011110 */, 0 /* 10011111 */, 0 /* 10100000 */,
		1 /* 10100001 */, 1 /* 10100010 */, 0 /* 10100011 */,
		1 /* 10100100 */, 0 /* 10100101 */, 0 /* 10100110 */,
		1 /* 10100111 */, 1 /* 10101000 */, 0 /* 10101001 */,
		0 /* 10101010 */, 1 /* 10101011 */, 0 /* 10101100 */,
		1 /* 10101101 */, 1 /* 10101110 */, 0 /* 10101111 */,
		1 /* 10110000 */, 0 /* 10110001 */, 0 /* 10110010 */,
		1 /* 10110011 */, 0 /* 10110100 */, 1 /* 10110101 */,
		1 /* 10110110 */, 0 /* 10110111 */, 0 /* 10111000 */,
		1 /* 10111001 */, 1 /* 10111010 */, 0 /* 10111011 */,
		1 /* 10111100 */, 0 /* 10111101 */, 0 /* 10111110 */,
		1 /* 10111111 */, 0 /* 11000000 */, 1 /* 11000001 */,
		1 /* 11000010 */, 0 /* 11000011 */, 1 /* 11000100 */,
		0 /* 11000101 */, 0 /* 11000110 */, 1 /* 11000111 */,
		1 /* 11001000 */, 0 /* 11001001 */, 0 /* 11001010 */,
		1 /* 11001011 */, 0 /* 11001100 */, 1 /* 11001101 */,
		1 /* 11001110 */, 0 /* 11001111 */, 1 /* 11010000 */,
		0 /* 11010001 */, 0 /* 11010010 */, 1 /* 11010011 */,
		0 /* 11010100 */, 1 /* 11010101 */, 1 /* 11010110 */,
		0 /* 11010111 */, 0 /* 11011000 */, 1 /* 11011001 */,
		1 /* 11011010 */, 0 /* 11011011 */, 1 /* 11011100 */,
		0 /* 11011101 */, 0 /* 11011110 */, 1 /* 11011111 */,
		1 /* 11100000 */, 0 /* 11100001 */, 0 /* 11100010 */,
		1 /* 11100011 */, 0 /* 11100100 */, 1 /* 11100101 */,
		1 /* 11100110 */, 0 /* 11100111 */, 0 /* 11101000 */,
		1 /* 11101001 */, 1 /* 11101010 */, 0 /* 11101011 */,
		1 /* 11101100 */, 0 /* 11101101 */, 0 /* 11101110 */,
		1 /* 11101111 */, 0 /* 11110000 */, 1 /* 11110001 */,
		1 /* 11110010 */, 0 /* 11110011 */, 1 /* 11110100 */,
		0 /* 11110101 */, 0 /* 11110110 */, 1 /* 11110111 */,
		1 /* 11111000 */, 0 /* 11111001 */, 0 /* 11111010 */,
		1 /* 11111011 */, 0 /* 11111100 */, 1 /* 11111101 */,
		1 /* 11111110 */, 0 /* 11111111 */
};

// 8080 CPU instructions, return value is clock states of a real Intel 8080 CPU.
// All undocumented instructions are executed and not trapped, to keep it simpler,
// for analysis use PC version.

static int op_nop(void)                 /* NOP */
{
  return (4);
}

static int op_hlt(void)                 /* HLT */
{
  // cry and die, no interrupts yet
  Serial.println();
  Serial.print(F("HLT instruction PC = 0x"));
  Serial.println(PC8, HEX);
  //display_freeram();
  State = Halted;
  return (7);
}

static int op_stc(void)                 /* STC */
{
  F |= C_FLAG;
  return (4);
}

static int op_cmc(void)                 /* CMC */
{
  if (F & C_FLAG)
    F &= ~C_FLAG;
  else
    F |= C_FLAG;
  return (4);
}

static int op_cma(void)                 /* CMA */
{
  A = ~A;
  return (4);
}

static int op_daa(void)                 /* DAA */
{
  register int tmp_a = A;

  if (((A & 0xf) > 9) || (F & H_FLAG)) {
    ((A & 0xf) > 9) ? (F |= H_FLAG) : (F &= ~H_FLAG);
    tmp_a += 6;
  }
  if (((tmp_a & 0x1f0) > 0x90) || (F & C_FLAG)) {
    tmp_a += 0x60;
  }
  if (tmp_a & 0x100)
    (F |= C_FLAG);
  A = tmp_a & 0xff;
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  return (4);
}

static int op_ei(void)                  /* EI */
{
  IFF = 3;
  return (4);
}

static int op_di(void)                  /* DI */
{
  IFF = 0;
  return (4);
}

static int op_in(void)                  /* IN n */
{
  BYTE addr;

  addr = memrdr(PC8++);
  A = io_in(addr, addr);
  return (10);
}

static int op_out(void)                 /* OUT n */
{
  BYTE addr;

  addr = memrdr(PC8++);
  io_out(addr, addr, A);
  return (10);
}

static int op_mvian(void)               /* MVI A,n */
{
  A = memrdr(PC8++);
  return (7);
}

static int op_mvibn(void)               /* MVI B,n */
{
  B = memrdr(PC8++);
  return (7);
}

static int op_mvicn(void)               /* MVI C,n */
{
  C = memrdr(PC8++);
  return (7);
}

static int op_mvidn(void)               /* MVI D,n */
{
  D = memrdr(PC8++);
  return (7);
}

static int op_mvien(void)               /* MVI E,n */
{
  E = memrdr(PC8++);
  return (7);
}

static int op_mvihn(void)               /* MVI H,n */
{
  H = memrdr(PC8++);
  return (7);
}

static int op_mviln(void)               /* MVI L,n */
{
  L = memrdr(PC8++);
  return (7);
}

static int op_ldaxb(void)               /* LDAX B */
{
  A = memrdr((B << 8) + C);
  return (7);
}

static int op_ldaxd(void)               /* LDAX D */
{
  A = memrdr((D << 8) + E);
  return (7);
}

static int op_ldann(void)               /* LDA nn */
{
  register WORD i;

  i = memrdr(PC8++);
  i += memrdr(PC8++) << 8;
  A = memrdr(i);
  return (13);
}

static int op_staxb(void)               /* STAX B */
{
  memwrt((B << 8) + C, A);
  return (7);
}

static int op_staxd(void)               /* STAX D */
{
  memwrt((D << 8) + E, A);
  return (7);
}

static int op_stann(void)               /* STA nn */
{
  register WORD i;

  i = memrdr(PC8++);
  i += memrdr(PC8++) << 8;
  memwrt(i, A);
  return (13);
}

static int op_movma(void)               /* MOV M,A */
{
  memwrt((H << 8) + L, A);
  return (7);
}

static int op_movmb(void)               /* MOV M,B */
{
  memwrt((H << 8) + L, B);
  return (7);
}

static int op_movmc(void)               /* MOV M,C */
{
  memwrt((H << 8) + L, C);
  return (7);
}

static int op_movmd(void)               /* MOV M,D */
{
  memwrt((H << 8) + L, D);
  return (7);
}

static int op_movme(void)               /* MOV M,E */
{
  memwrt((H << 8) + L, E);
  return (7);
}

static int op_movmh(void)               /* MOV M,H */
{
  memwrt((H << 8) + L, H);
  return (7);
}

static int op_movml(void)               /* MOV M,L */
{
  memwrt((H << 8) + L, L);
  return (7);
}

static int op_mvimn(void)               /* MVI M,n */
{
  memwrt((H << 8) + L, memrdr(PC8++));
  return (10);
}

static int op_movaa(void)               /* MOV A,A */
{
  return (5);
}

static int op_movab(void)               /* MOV A,B */
{
  A = B;
  return (5);
}

static int op_movac(void)               /* MOV A,C */
{
  A = C;
  return (5);
}

static int op_movad(void)               /* MOV A,D */
{
  A = D;
  return (5);
}

static int op_movae(void)               /* MOV A,E */
{
  A = E;
  return (5);
}

static int op_movah(void)               /* MOV A,H */
{
  A = H;
  return (5);
}

static int op_moval(void)               /* MOV A,L */
{
  A = L;
  return (5);
}

static int op_movam(void)               /* MOV A,M */
{
  A = memrdr((H << 8) + L);
  return (7);
}

static int op_movba(void)               /* MOV B,A */
{
  B = A;
  return (5);
}

static int op_movbb(void)               /* MOV B,B */
{
  return (5);
}

static int op_movbc(void)               /* MOV B,C */
{
  B = C;
  return (5);
}

static int op_movbd(void)               /* MOV B,D */
{
  B = D;
  return (5);
}

static int op_movbe(void)               /* MOV B,E */
{
  B = E;
  return (5);
}

static int op_movbh(void)               /* MOV B,H */
{
  B = H;
  return (5);
}

static int op_movbl(void)               /* MOV B,L */
{
  B = L;
  return (5);
}

static int op_movbm(void)               /* MOV B,M */
{
  B = memrdr((H << 8) + L);
  return (7);
}

static int op_movca(void)               /* MOV C,A */
{
  C = A;
  return (5);
}

static int op_movcb(void)               /* MOV C,B */
{
  C = B;
  return (5);
}

static int op_movcc(void)               /* MOV C,C */
{
  return (5);
}

static int op_movcd(void)               /* MOV C,D */
{
  C = D;
  return (5);
}

static int op_movce(void)               /* MOV C,E */
{
  C = E;
  return (5);
}

static int op_movch(void)               /* MOV C,H */
{
  C = H;
  return (5);
}

static int op_movcl(void)               /* MOV C,L */
{
  C = L;
  return (5);
}

static int op_movcm(void)               /* MOV C,M */
{
  C = memrdr((H << 8) + L);
  return (7);
}

static int op_movda(void)               /* MOV D,A */
{
  D = A;
  return (5);
}

static int op_movdb(void)               /* MOV D,B */
{
  D = B;
  return (5);
}

static int op_movdc(void)               /* MOV D,C */
{
  D = C;
  return (5);
}

static int op_movdd(void)               /* MOV D,D */
{
  return (5);
}

static int op_movde(void)               /* MOV D,E */
{
  D = E;
  return (5);
}

static int op_movdh(void)               /* MOV D,H */
{
  D = H;
  return (5);
}

static int op_movdl(void)               /* MOV D,L */
{
  D = L;
  return (5);
}

static int op_movdm(void)               /* MOV D,M */
{
  D = memrdr((H << 8) + L);
  return (7);
}

static int op_movea(void)               /* MOV E,A */
{
  E = A;
  return (5);
}

static int op_moveb(void)               /* MOV E,B */
{
  E = B;
  return (5);
}

static int op_movec(void)               /* MOV E,C */
{
  E = C;
  return (5);
}

static int op_moved(void)               /* MOV E,D */
{
  E = D;
  return (5);
}

static int op_movee(void)               /* MOV E,E */
{
  return (5);
}

static int op_moveh(void)               /* MOV E,H */
{
  E = H;
  return (5);
}

static int op_movel(void)               /* MOV E,L */
{
  E = L;
  return (5);
}

static int op_movem(void)               /* MOV E,M */
{
  E = memrdr((H << 8) + L);
  return (7);
}

static int op_movha(void)               /* MOV H,A */
{
  H = A;
  return (5);
}

static int op_movhb(void)               /* MOV H,B */
{
  H = B;
  return (5);
}

static int op_movhc(void)               /* MOV H,C */
{
  H = C;
  return (5);
}

static int op_movhd(void)               /* MOV H,D */
{
  H = D;
  return (5);
}

static int op_movhe(void)               /* MOV H,E */
{
  H = E;
  return (5);
}

static int op_movhh(void)               /* MOV H,H */
{
  return (5);
}

static int op_movhl(void)               /* MOV H,L */
{
  H = L;
  return (5);
}

static int op_movhm(void)               /* MOV H,M */
{
  H = memrdr((H << 8) + L);
  return (7);
}

static int op_movla(void)               /* MOV L,A */
{
  L = A;
  return (5);
}

static int op_movlb(void)               /* MOV L,B */
{
  L = B;
  return (5);
}

static int op_movlc(void)               /* MOV L,C */
{
  L = C;
  return (5);
}

static int op_movld(void)               /* MOV L,D */
{
  L = D;
  return (5);
}

static int op_movle(void)               /* MOV L,E */
{
  L = E;
  return (5);
}

static int op_movlh(void)               /* MOV L,H */
{
  L = H;
  return (5);
}

static int op_movll(void)               /* MOV L,L */
{
  return (5);
}

static int op_movlm(void)               /* MOV L,M */
{
  L = memrdr((H << 8) + L);
  return (7);
}

static int op_lxibnn(void)              /* LXI B,nn */
{
  C = memrdr(PC8++);
  B = memrdr(PC8++);
  return (10);
}

static int op_lxidnn(void)              /* LXI D,nn */
{
  E = memrdr(PC8++);
  D = memrdr(PC8++);
  return (10);
}

static int op_lxihnn(void)              /* LXI H,nn */
{
  L = memrdr(PC8++);
  H = memrdr(PC8++);
  return (10);
}

static int op_lxispnn(void)             /* LXI SP,nn */
{
  SP8 = memrdr(PC8++);
  SP8 += memrdr(PC8++) << 8;
  return (10);
}

static int op_sphl(void)                /* SPHL */
{
  SP8 = (H << 8) + L;
  return (5);
}

static int op_lhldnn(void)              /* LHLD nn */
{
  register WORD i;

  i = memrdr(PC8++);
  i += memrdr(PC8++) << 8;
  L = memrdr(i);
  H = memrdr(i + 1);
  return (16);
}

static int op_shldnn(void)              /* SHLD nn */
{
  register WORD i;

  i = memrdr(PC8++);
  i += memrdr(PC8++) << 8;
  memwrt(i, L);
  memwrt(i + 1, H);
  return (16);
}

static int op_inxb(void)                /* INX B */
{
  C++;
  if (!C)
    B++;
  return (5);
}

static int op_inxd(void)                /* INX D */
{
  E++;
  if (!E)
    D++;
  return (5);
}

static int op_inxh(void)                /* INX H */
{
  L++;
  if (!L)
    H++;
  return (5);
}

static int op_inxsp(void)               /* INX SP */
{
  SP8++;
  return (5);
}

static int op_dcxb(void)                /* DCX B */
{
  C--;
  if (C == 0xff)
    B--;
  return (5);
}

static int op_dcxd(void)                /* DCX D */
{
  E--;
  if (E == 0xff)
    D--;
  return (5);
}

static int op_dcxh(void)                /* DCX H */
{
  L--;
  if (L == 0xff)
    H--;
  return (5);
}

static int op_dcxsp(void)               /* DCX SP */
{
  SP8--;
  return (5);
}

static int op_dadb(void)                /* DAD B */
{
  register int carry;

  carry = (L + C > 255) ? 1 : 0;
  L += C;
  (H + B + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  H += B + carry;
  return (10);
}

static int op_dadd(void)                /* DAD D */
{
  register int carry;

  carry = (L + E > 255) ? 1 : 0;
  L += E;
  (H + D + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  H += D + carry;
  return (10);
}

static int op_dadh(void)                /* DAD H */
{
  register int carry;

  carry = (L << 1 > 255) ? 1 : 0;
  L <<= 1;
  (H + H + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  H += H + carry;
  return (10);
}

static int op_dadsp(void)               /* DAD SP */
{
  register int carry;

  BYTE spl = SP8 & 0xff;
  BYTE sph = SP8 >> 8;

  carry = (L + spl > 255) ? 1 : 0;
  L += spl;
  (H + sph + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  H += sph + carry;
  return (10);
}

static int op_anaa(void)                /* ANA A */
{
  (A & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~C_FLAG;
  return (4);
}

static int op_anab(void)                /* ANA B */
{
  ((A | B) & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  A &= B;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~C_FLAG;
  return (4);
}

static int op_anac(void)                /* ANA C */
{
  ((A | C) & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  A &= C;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~C_FLAG;
  return (4);
}

static int op_anad(void)                /* ANA D */
{
  ((A | D) & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  A &= D;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~C_FLAG;
  return (4);
}

static int op_anae(void)                /* ANA E */
{
  ((A | E) & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  A &= E;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~C_FLAG;
  return (4);
}

static int op_anah(void)                /* ANA H */
{
  ((A | H) & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  A &= H;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~C_FLAG;
  return (4);
}

static int op_anal(void)                /* ANA L */
{
  ((A | L) & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  A &= L;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~C_FLAG;
  return (4);
}

static int op_anam(void)                /* ANA M */
{
  register BYTE P;

  P = memrdr((H << 8) + L);
  ((A | P) & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  A &= P;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~C_FLAG;
  return (7);
}

static int op_anin(void)                /* ANI n */
{
  register BYTE P;

  P = memrdr(PC8++);
  ((A | P) & 8) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  A &= P;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~C_FLAG;
  return (7);
}

static int op_oraa(void)                /* ORA A */
{
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(C_FLAG | H_FLAG);
  return (4);
}

static int op_orab(void)                /* ORA B */
{
  A |= B;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(C_FLAG | H_FLAG);
  return (4);
}

static int op_orac(void)                /* ORA C */
{
  A |= C;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(C_FLAG | H_FLAG);
  return (4);
}

static int op_orad(void)                /* ORA D */
{
  A |= D;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(C_FLAG | H_FLAG);
  return (4);
}

static int op_orae(void)                /* ORA E */
{
  A |= E;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(C_FLAG | H_FLAG);
  return (4);
}

static int op_orah(void)                /* ORA H */
{
  A |= H;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(C_FLAG | H_FLAG);
  return (4);
}

static int op_oral(void)                /* ORA L */
{
  A |= L;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(C_FLAG | H_FLAG);
  return (4);
}

static int op_oram(void)                /* ORA M */
{
  A |= memrdr((H << 8) + L);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(C_FLAG | H_FLAG);
  return (7);
}

static int op_orin(void)                /* ORI n */
{
  A |= memrdr(PC8++);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(C_FLAG | H_FLAG);
  return (7);
}

static int op_xraa(void)                /* XRA A */
{
  A = 0;
  F &= ~(S_FLAG | H_FLAG | C_FLAG);
  F |= Z_FLAG | P_FLAG;
  return (4);
}

static int op_xrab(void)                /* XRA B */
{
  A ^= B;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | C_FLAG);
  return (4);
}

static int op_xrac(void)                /* XRA C */
{
  A ^= C;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | C_FLAG);
  return (4);
}

static int op_xrad(void)                /* XRA D */
{
  A ^= D;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | C_FLAG);
  return (4);
}

static int op_xrae(void)                /* XRA E */
{
  A ^= E;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | C_FLAG);
  return (4);
}

static int op_xrah(void)                /* XRA H */
{
  A ^= H;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | C_FLAG);
  return (4);
}

static int op_xral(void)                /* XRA L */
{
  A ^= L;
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | C_FLAG);
  return (4);
}

static int op_xram(void)                /* XRA M */
{
  A ^= memrdr((H << 8) + L);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | C_FLAG);
  return (7);
}

static int op_xrin(void)                /* XRI n */
{
  A ^= memrdr(PC8++);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  F &= ~(H_FLAG | C_FLAG);
  return (7);
}

static int op_adda(void)                /* ADD A */
{
  ((A & 0xf) + (A & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  ((A << 1) > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A << 1;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_addb(void)                /* ADD B */
{
  ((A & 0xf) + (B & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + B > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A + B;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_addc(void)                /* ADD C */
{
  ((A & 0xf) + (C & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + C > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A + C;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_addd(void)                /* ADD D */
{
  ((A & 0xf) + (D & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + D > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A + D;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_adde(void)                /* ADD E */
{
  ((A & 0xf) + (E & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + E > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A + E;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_addh(void)                /* ADD H */
{
  ((A & 0xf) + (H & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + H > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A + H;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_addl(void)                /* ADD L */
{
  ((A & 0xf) + (L & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + L > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A + L;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_addm(void)                /* ADD M */
{
  register BYTE P;

  P = memrdr((H << 8) + L);
  ((A & 0xf) + (P & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + P > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A + P;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (7);
}

static int op_adin(void)                /* ADI n */
{
  register BYTE P;

  P = memrdr(PC8++);
  ((A & 0xf) + (P & 0xf) > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + P > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A + P;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (7);
}

static int op_adca(void)                /* ADC A */
{
  register int carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((A & 0xf) + (A & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  ((A << 1) + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = (A << 1) + carry;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_adcb(void)                /* ADC B */
{
  register int carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((A & 0xf) + (B & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + B + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A + B + carry;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_adcc(void)                /* ADC C */
{
  register int carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((A & 0xf) + (C & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + C + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A + C + carry;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_adcd(void)                /* ADC D */
{
  register int carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((A & 0xf) + (D & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + D + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A + D + carry;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_adce(void)                /* ADC E */
{
  register int carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((A & 0xf) + (E & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + E + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A + E + carry;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_adch(void)                /* ADC H */
{
  register int carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((A & 0xf) + (H & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + H + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A + H + carry;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_adcl(void)                /* ADC L */
{
  register int carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((A & 0xf) + (L & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + L + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A + L + carry;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_adcm(void)                /* ADC M */
{
  register int carry;
  register BYTE P;

  P = memrdr((H << 8) + L);
  carry = (F & C_FLAG) ? 1 : 0;
  ((A & 0xf) + (P & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + P + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A + P + carry;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (7);
}

static int op_acin(void)                /* ACI n */
{
  register int carry;
  register BYTE P;

  carry = (F & C_FLAG) ? 1 : 0;
  P = memrdr(PC8++);
  ((A & 0xf) + (P & 0xf) + carry > 0xf) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (A + P + carry > 255) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A + P + carry;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (7);
}

static int op_suba(void)                /* SUB A */
{
  A = 0;
  F &= ~(S_FLAG | C_FLAG);
  F |= Z_FLAG | H_FLAG | P_FLAG;
  return (4);
}

static int op_subb(void)                /* SUB B */
{
  ((B & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (B > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A - B;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_subc(void)                /* SUB C */
{
  ((C & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (C > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A - C;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_subd(void)                /* SUB D */
{
  ((D & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (D > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A - D;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_sube(void)                /* SUB E */
{
  ((E & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (E > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A - E;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_subh(void)                /* SUB H */
{
  ((H & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (H > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A - H;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_subl(void)                /* SUB L */
{
  ((L & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (L > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A - L;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_subm(void)                /* SUB M */
{
  register BYTE P;

  P = memrdr((H << 8) + L);
  ((P & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (P > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A - P;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (7);
}

static int op_suin(void)                /* SUI n */
{
  register BYTE P;

  P = memrdr(PC8++);
  ((P & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (P > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A - P;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (7);
}

static int op_sbba(void)                /* SBB A */
{
  if (F & C_FLAG) {
    F |= S_FLAG | C_FLAG | P_FLAG;
    F &= ~(Z_FLAG | H_FLAG);
    A = 255;
  } else {
    F |= Z_FLAG | H_FLAG | P_FLAG;
    F &= ~(S_FLAG | C_FLAG);
    A = 0;
  }
  return (4);
}

static int op_sbbb(void)                /* SBB B */
{
  register int carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((B & 0xf) + carry > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (B + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A - B - carry;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_sbbc(void)                /* SBB C */
{
  register int carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((C & 0xf) + carry > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (C + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A - C - carry;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_sbbd(void)                /* SBB D */
{
  register int carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((D & 0xf) + carry > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (D + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A - D - carry;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_sbbe(void)                /* SBB E */
{
  register int carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((E & 0xf) + carry > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (E + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A - E - carry;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_sbbh(void)                /* SBB H */
{
  register int carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((H & 0xf) + carry > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (H + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A - H - carry;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_sbbl(void)                /* SBB L */
{
  register int carry;

  carry = (F & C_FLAG) ? 1 : 0;
  ((L & 0xf) + carry > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (L + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A - L - carry;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_sbbm(void)                /* SBB M */
{
  register int carry;
  register BYTE P;

  P = memrdr((H << 8) + L);
  carry = (F & C_FLAG) ? 1 : 0;
  ((P & 0xf) + carry > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (P + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A - P - carry;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (7);
}

static int op_sbin(void)                /* SBI n */
{
  register int carry;
  register BYTE P;

  P = memrdr(PC8++);
  carry = (F & C_FLAG) ? 1 : 0;
  ((P & 0xf) + carry > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (P + carry > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A = A - P - carry;
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (7);
}

static int op_cmpa(void)                /* CMP A */
{
  F &= ~(S_FLAG | C_FLAG);
  F |= Z_FLAG | H_FLAG | P_FLAG;
  return (4);
}

static int op_cmpb(void)                /* CMP B */
{
  register BYTE i;

  ((B & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (B > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  i = A - B;
  (pgm_read_byte(i)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_cmpc(void)                /* CMP C */
{
  register BYTE i;

  ((C & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (C > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  i = A - C;
  (pgm_read_byte(i)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_cmpd(void)                /* CMP D */
{
  register BYTE i;

  ((D & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (D > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  i = A - D;
  (pgm_read_byte(i)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_cmpe(void)                /* CMP E */
{
  register BYTE i;

  ((E & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (E > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  i = A - E;
  (pgm_read_byte(i)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_cmph(void)                /* CMP H */
{
  register BYTE i;

  ((H & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (H > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  i = A - H;
  (pgm_read_byte(i)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_cmpl(void)                /* CMP L */
{
  register BYTE i;

  ((L & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (L > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  i = A - L;
  (pgm_read_byte(i)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (4);
}

static int op_cmpm(void)                /* CMP M */
{
  register BYTE i;
  register BYTE P;

  P = memrdr((H << 8) + L);
  ((P & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (P > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  i = A - P;
  (pgm_read_byte(i)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (7);
}

static int op_cpin(void)                /* CPI n */
{
  register BYTE i;
  register BYTE P;

  P = memrdr(PC8++);
  ((P & 0xf) > (A & 0xf)) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (P > A) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  i = A - P;
  (pgm_read_byte(i)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (i & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (i) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (7);
}

static int op_inra(void)                /* INR A */
{
  A++;
  ((A & 0xf) == 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (5);
}

static int op_inrb(void)                /* INR B */
{
  B++;
  ((B & 0xf) == 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (pgm_read_byte(B)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (B & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (B) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (5);
}

static int op_inrc(void)                /* INR C */
{
  C++;
  ((C & 0xf) == 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (pgm_read_byte(C)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (C & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (C) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (5);
}

static int op_inrd(void)                /* INR D */
{
  D++;
  ((D & 0xf) == 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (pgm_read_byte(D)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (D & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (D) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (5);
}

static int op_inre(void)                /* INR E */
{
  E++;
  ((E & 0xf) == 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (pgm_read_byte(E)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (E & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (E) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (5);
}

static int op_inrh(void)                /* INR H */
{
  H++;
  ((H & 0xf) == 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (pgm_read_byte(H)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (H & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (H) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (5);
}

static int op_inrl(void)                /* INR L */
{
  L++;
  ((L & 0xf) == 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (pgm_read_byte(L)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (L & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (L) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (5);
}

static int op_inrm(void)                /* INR M */
{
  register BYTE P;
  WORD addr;

  addr = (H << 8) + L;
  P = memrdr(addr);
  P++;
  memwrt(addr, P);
  ((P & 0xf) == 0) ? (F |= H_FLAG) : (F &= ~H_FLAG);
  (pgm_read_byte(P)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (P & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (P) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (10);
}

static int op_dcra(void)                /* DCR A */
{
  A--;
  ((A & 0xf) == 0xf) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (pgm_read_byte(A)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (A & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (A) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (5);
}

static int op_dcrb(void)                /* DCR B */
{
  B--;
  ((B & 0xf) == 0xf) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (pgm_read_byte(B)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (B & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (B) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (5);
}

static int op_dcrc(void)                /* DCR C */
{
  C--;
  ((C & 0xf) == 0xf) ? (F &= ~H_FLAG): (F |= H_FLAG);
  (pgm_read_byte(C)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (C & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (C) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (5);
}

static int op_dcrd(void)                /* DCR D */
{
  D--;
  ((D & 0xf) == 0xf) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (pgm_read_byte(D)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (D & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (D) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (5);
}

static int op_dcre(void)                /* DCR E */
{
  E--;
  ((E & 0xf) == 0xf) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (pgm_read_byte(E)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (E & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (E) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (5);
}

static int op_dcrh(void)                /* DCR H */
{
  H--;
  ((H & 0xf) == 0xf) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (pgm_read_byte(H)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (H & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (H) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (5);
}

static int op_dcrl(void)                /* DCR L */
{
  L--;
  ((L & 0xf) == 0xf) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (pgm_read_byte(L)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (L & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (L) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (5);
}

static int op_dcrm(void)                /* DCR M */
{
  register BYTE P;
  WORD addr;

  addr = (H << 8) + L;
  P = memrdr(addr);
  P--;
  memwrt(addr, P);
  ((P & 0xf) == 0xf) ? (F &= ~H_FLAG) : (F |= H_FLAG);
  (pgm_read_byte(P)) ? (F &= ~P_FLAG) : (F |= P_FLAG);
  (P & 128) ? (F |= S_FLAG) : (F &= ~S_FLAG);
  (P) ? (F &= ~Z_FLAG) : (F |= Z_FLAG);
  return (10);
}

static int op_rlc(void)                 /* RLC */
{
  register int i;

  i = (A & 128) ? 1 : 0;
  (i) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A <<= 1;
  A |= i;
  return (4);
}

static int op_rrc(void)                 /* RRC */
{
  register int i;

  i = A & 1;
  (i) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A >>= 1;
  if (i) A |= 128;
  return (4);
}

static int op_ral(void)                 /* RAL */
{
  register int old_c_flag;

  old_c_flag = F & C_FLAG;
  (A & 128) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A <<= 1;
  if (old_c_flag) A |= 1;
  return (4);
}

static int op_rar(void)                 /* RAR */
{
  register int i, old_c_flag;

  old_c_flag = F & C_FLAG;
  i = A & 1;
  (i) ? (F |= C_FLAG) : (F &= ~C_FLAG);
  A >>= 1;
  if (old_c_flag) A |= 128;
  return (4);
}

static int op_xchg(void)                /* XCHG */
{
  register BYTE i;

  i = D;
  D = H;
  H = i;
  i = E;
  E = L;
  L = i;
  return (4);
}

static int op_xthl(void)                /* XTHL */
{
  register BYTE i;

  i = memrdr(SP8);
  memwrt(SP8, L);
  L = i;
  i = memrdr(SP8 + 1);
  memwrt(SP8 + 1, H);
  H = i;
  return (18);
}

static int op_pushpsw(void)             /* PUSH PSW */
{
  memwrt(--SP8, A);
  memwrt(--SP8, F);
  return (11);
}

static int op_pushb(void)               /* PUSH B */
{
  memwrt(--SP8, B);
  memwrt(--SP8, C);
  return (11);
}

static int op_pushd(void)               /* PUSH D */
{
  memwrt(--SP8, D);
  memwrt(--SP8, E);
  return (11);
}

static int op_pushh(void)               /* PUSH H */
{
  memwrt(--SP8, H);
  memwrt(--SP8, L);
  return (11);
}

static int op_poppsw(void)              /* POP PSW */
{
  F = memrdr(SP8++);
  F &= ~(N2_FLAG | N1_FLAG);
  F |= N_FLAG;
  A = memrdr(SP8++);
  return (10);
}

static int op_popb(void)                /* POP B */
{
  C = memrdr(SP8++);
  B = memrdr(SP8++);
  return (10);
}

static int op_popd(void)                /* POP D */
{
  E = memrdr(SP8++);
  D = memrdr(SP8++);
  return (10);
}

static int op_poph(void)                /* POP H */
{
  L = memrdr(SP8++);
  H = memrdr(SP8++);
  return (10);
}

static int op_jmp(void)                 /* JMP */
{
  register WORD i;

  i = memrdr(PC8++);
  i += memrdr(PC8) << 8;
  PC8 = i;
  return (10);
}

static int op_pchl(void)                /* PCHL */
{
  PC8 = (H << 8) + L;
  return (5);
}

static int op_call(void)                /* CALL */
{
  register WORD i;

  i = memrdr(PC8++);
  i += memrdr(PC8++) << 8;
  memwrt(--SP8, PC8 >> 8);
  memwrt(--SP8, PC8);
  PC8 = i;
  return (17);
}

static int op_ret(void)                 /* RET */
{
  register WORD i;

  i = memrdr(SP8++);
  i += memrdr(SP8++) << 8;
  PC8 = i;
  return (10);
}

static int op_jz(void)                  /* JZ nn */
{
  register WORD i;

  if (F & Z_FLAG) {
    i = memrdr(PC8++);
    i += memrdr(PC8++) << 8;
    PC8 = i;
  } else {
    PC8 += 2;
  }
  return (10);
}

static int op_jnz(void)                 /* JNZ nn */
{
  register WORD i;

  if (!(F & Z_FLAG)) {
    i = memrdr(PC8++);
    i += memrdr(PC8++) << 8;
    PC8 = i;
  } else {
    PC8 += 2;
  }
  return (10);
}

static int op_jc(void)                  /* JC nn */
{
  register WORD i;

  if (F & C_FLAG) {
    i = memrdr(PC8++);
    i += memrdr(PC8++) << 8;
    PC8 = i;
  } else {
    PC8 += 2;
  }
  return (10);
}

static int op_jnc(void)                 /* JNC nn */
{
  register WORD i;

  if (!(F & C_FLAG)) {
    i = memrdr(PC8++);
    i += memrdr(PC8++) << 8;
    PC8 = i;
  } else {
    PC8 += 2;
  }
  return (10);
}

static int op_jpe(void)                 /* JPE nn */
{
  register WORD i;

  if (F & P_FLAG) {
    i = memrdr(PC8++);
    i += memrdr(PC8++) << 8;
    PC8 = i;
  } else {
    PC8 += 2;
  }
  return (10);
}

static int op_jpo(void)                 /* JPO nn */
{
  register WORD i;

  if (!(F & P_FLAG)) {
    i = memrdr(PC8++);
    i += memrdr(PC8++) << 8;
    PC8 = i;
  } else {
    PC8 += 2;
  }
  return (10);
}

static int op_jm(void)                  /* JM nn */
{
  register WORD i;

  if (F & S_FLAG) {
    i = memrdr(PC8++);
    i += memrdr(PC8++) << 8;
    PC8 = i;
  } else {
    PC8 += 2;
  }
  return (10);
}

static int op_jp(void)                  /* JP nn */
{
  register WORD i;

  if (!(F & S_FLAG)) {
    i = memrdr(PC8++);
    i += memrdr(PC8++) << 8;
    PC8 = i;
  } else {
    PC8 += 2;
  }
  return (10);
}

static int op_cz(void)                  /* CZ nn */
{
  register WORD i;

  if (F & Z_FLAG) {
    i = memrdr(PC8++);
    i += memrdr(PC8++) << 8;
    memwrt(--SP8, PC8 >> 8);
    memwrt(--SP8, PC8);
    PC8 = i;
    return (17);
  } else {
    PC8 += 2;
    return (11);
  }
}

static int op_cnz(void)                 /* CNZ nn */
{
  register WORD i;

  if (!(F & Z_FLAG)) {
    i = memrdr(PC8++);
    i += memrdr(PC8++) << 8;
    memwrt(--SP8, PC8 >> 8);
    memwrt(--SP8, PC8);
    PC8 = i;
    return (17);
  } else {
    PC8 += 2;
    return (11);
  }
}

static int op_cc(void)                  /* CC nn */
{
  register WORD i;

  if (F & C_FLAG) {
    i = memrdr(PC8++);
    i += memrdr(PC8++) << 8;
    memwrt(--SP8, PC8 >> 8);
    memwrt(--SP8, PC8);
    PC8 = i;
    return (17);
  } else {
    PC8 += 2;
    return (11);
  }
}

static int op_cnc(void)                 /* CNC nn */
{
  register WORD i;

  if (!(F & C_FLAG)) {
    i = memrdr(PC8++);
    i += memrdr(PC8++) << 8;
    memwrt(--SP8, PC8 >> 8);
    memwrt(--SP8, PC8);
    PC8 = i;
    return (17);
  } else {
    PC8 += 2;
    return (11);
  }
}

static int op_cpe(void)                 /* CPE nn */
{
  register WORD i;

  if (F & P_FLAG) {
    i = memrdr(PC8++);
    i += memrdr(PC8++) << 8;
    memwrt(--SP8, PC8 >> 8);
    memwrt(--SP8, PC8);
    PC8 = i;
    return (17);
  } else {
    PC8 += 2;
    return (11);
  }
}

static int op_cpo(void)                 /* CPO nn */
{
  register WORD i;

  if (!(F & P_FLAG)) {
    i = memrdr(PC8++);
    i += memrdr(PC8++) << 8;
    memwrt(--SP8, PC8 >> 8);
    memwrt(--SP8, PC8);
    PC8 = i;
    return (17);
  } else {
    PC8 += 2;
    return (11);
  }
}

static int op_cm(void)                  /* CM nn */
{
  register WORD i;

  if (F & S_FLAG) {
    i = memrdr(PC8++);
    i += memrdr(PC8++) << 8;
    memwrt(--SP8, PC8 >> 8);
    memwrt(--SP8, PC8);
    PC8 = i;
    return (17);
  } else {
    PC8 += 2;
    return (11);
  }
}

static int op_cp(void)                  /* CP nn */
{
  register WORD i;

  if (!(F & S_FLAG)) {
    i = memrdr(PC8++);
    i += memrdr(PC8++) << 8;
    memwrt(--SP8, PC8 >> 8);
    memwrt(--SP8, PC8);
    PC8 = i;
    return (17);
  } else {
    PC8 += 2;
    return (11);
  }
}

static int op_rz(void)                  /* RZ */
{
  register WORD i;

  if (F & Z_FLAG) {
    i = memrdr(SP8++);
    i += memrdr(SP8++) << 8;
    PC8 = i;
    return (11);
  } else {
    return (5);
  }
}

static int op_rnz(void)                 /* RNZ */
{
  register WORD i;

  if (!(F & Z_FLAG)) {
    i = memrdr(SP8++);
    i += memrdr(SP8++) << 8;
    PC8 = i;
    return (11);
  } else {
    return (5);
  }
}

static int op_rc(void)                  /* RC */
{
  register WORD i;

  if (F & C_FLAG) {
    i = memrdr(SP8++);
    i += memrdr(SP8++) << 8;
    PC8 = i;
    return (11);
  } else {
    return (5);
  }
}

static int op_rnc(void)                 /* RNC */
{
  register WORD i;

  if (!(F & C_FLAG)) {
    i = memrdr(SP8++);
    i += memrdr(SP8++) << 8;
    PC8 = i;
    return (11);
  } else {
    return (5);
  }
}

static int op_rpe(void)                 /* RPE */
{
  register WORD i;

  if (F & P_FLAG) {
    i = memrdr(SP8++);
    i += memrdr(SP8++) << 8;
    PC8 = i;
    return (11);
  } else {
    return (5);
  }
}

static int op_rpo(void)                 /* RPO */
{
  register WORD i;

  if (!(F & P_FLAG)) {
    i = memrdr(SP8++);
    i += memrdr(SP8++) << 8;
    PC8 = i;
    return (11);
  } else {
    return (5);
  }
}

static int op_rm(void)                  /* RM */
{
  register WORD i;

  if (F & S_FLAG) {
    i = memrdr(SP8++);
    i += memrdr(SP8++) << 8;
    PC8 = i;
    return (11);
  } else {
    return (5);
  }
}

static int op_rp(void)                  /* RP */
{
  register WORD i;

  if (!(F & S_FLAG)) {
    i = memrdr(SP8++);
    i += memrdr(SP8++) << 8;
    PC8 = i;
    return (11);
  } else {
    return (5);
  }
}

static int op_rst0(void)                /* RST 0 */
{
  memwrt(--SP8, PC8 >> 8);
  memwrt(--SP8, PC8);
  PC8 = 0;
  return (11);
}

static int op_rst1(void)                /* RST 1 */
{
  memwrt(--SP8, PC8 >> 8);
  memwrt(--SP8, PC8);
  PC8 = 0x08;
  return (11);
}

static int op_rst2(void)                /* RST 2 */
{
  memwrt(--SP8, PC8 >> 8);
  memwrt(--SP8, PC8);
  PC8 = 0x10;
  return (11);
}

static int op_rst3(void)                /* RST 3 */
{
  memwrt(--SP8, PC8 >> 8);
  memwrt(--SP8, PC8);
  PC8 = 0x18;
  return (11);
}

static int op_rst4(void)                /* RST 4 */
{
  memwrt(--SP8, PC8 >> 8);
  memwrt(--SP8, PC8);
  PC8 = 0x20;
  return (11);
}

static int op_rst5(void)                /* RST 5 */
{
  memwrt(--SP8, PC8 >> 8);
  memwrt(--SP8, PC8);
  PC8 = 0x28;
  return (11);
}

static int op_rst6(void)                /* RST 6 */
{
  memwrt(--SP8, PC8 >> 8);
  memwrt(--SP8, PC8);
  PC8 = 0x30;
  return (11);
}

static int op_rst7(void)                /* RST 7 */
{
  memwrt(--SP8, PC8 >> 8);
  memwrt(--SP8, PC8);
  PC8 = 0x38;
  return (11);
}

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
  F &= ~(N2_FLAG | N1_FLAG);
  F |= N_FLAG;
}

// This function builds the 8080 central processing unit.
// The opcode where PC points to is fetched from the memory
// and PC incremented by one. The opcode is used as an
// index to an array with function pointers, to execute a
// function which emulates this 8080 opcode.
void cpu_8080(void)
{
  static const int (*op_sim[256]) (void) = {
    op_nop,                         /* 0x00 */
    op_lxibnn,                      /* 0x01 */
    op_staxb,                       /* 0x02 */
    op_inxb,                        /* 0x03 */
    op_inrb,                        /* 0x04 */
    op_dcrb,                        /* 0x05 */
    op_mvibn,                       /* 0x06 */
    op_rlc,                         /* 0x07 */
    op_nop,                         /* 0x08 */
    op_dadb,                        /* 0x09 */
    op_ldaxb,                       /* 0x0a */
    op_dcxb,                        /* 0x0b */
    op_inrc,                        /* 0x0c */
    op_dcrc,                        /* 0x0d */
    op_mvicn,                       /* 0x0e */
    op_rrc,                         /* 0x0f */
    op_nop,                         /* 0x10 */
    op_lxidnn,                      /* 0x11 */
    op_staxd,                       /* 0x12 */
    op_inxd,                        /* 0x13 */
    op_inrd,                        /* 0x14 */
    op_dcrd,                        /* 0x15 */
    op_mvidn,                       /* 0x16 */
    op_ral,                         /* 0x17 */
    op_nop,                         /* 0x18 */
    op_dadd,                        /* 0x19 */
    op_ldaxd,                       /* 0x1a */
    op_dcxd,                        /* 0x1b */
    op_inre,                        /* 0x1c */
    op_dcre,                        /* 0x1d */
    op_mvien,                       /* 0x1e */
    op_rar,                         /* 0x1f */
    op_nop,                         /* 0x20 */
    op_lxihnn,                      /* 0x21 */
    op_shldnn,                      /* 0x22 */
    op_inxh,                        /* 0x23 */
    op_inrh,                        /* 0x24 */
    op_dcrh,                        /* 0x25 */
    op_mvihn,                       /* 0x26 */
    op_daa,                         /* 0x27 */
    op_nop,                         /* 0x28 */
    op_dadh,                        /* 0x29 */
    op_lhldnn,                      /* 0x2a */
    op_dcxh,                        /* 0x2b */
    op_inrl,                        /* 0x2c */
    op_dcrl,                        /* 0x2d */
    op_mviln,                       /* 0x2e */
    op_cma,                         /* 0x2f */
    op_nop,                         /* 0x30 */
    op_lxispnn,                     /* 0x31 */
    op_stann,                       /* 0x32 */
    op_inxsp,                       /* 0x33 */
    op_inrm,                        /* 0x34 */
    op_dcrm,                        /* 0x35 */
    op_mvimn,                       /* 0x36 */
    op_stc,                         /* 0x37 */
    op_nop,                         /* 0x38 */
    op_dadsp,                       /* 0x39 */
    op_ldann,                       /* 0x3a */
    op_dcxsp,                       /* 0x3b */
    op_inra,                        /* 0x3c */
    op_dcra,                        /* 0x3d */
    op_mvian,                       /* 0x3e */
    op_cmc,                         /* 0x3f */
    op_movbb,                       /* 0x40 */
    op_movbc,                       /* 0x41 */
    op_movbd,                       /* 0x42 */
    op_movbe,                       /* 0x43 */
    op_movbh,                       /* 0x44 */
    op_movbl,                       /* 0x45 */
    op_movbm,                       /* 0x46 */
    op_movba,                       /* 0x47 */
    op_movcb,                       /* 0x48 */
    op_movcc,                       /* 0x49 */
    op_movcd,                       /* 0x4a */
    op_movce,                       /* 0x4b */
    op_movch,                       /* 0x4c */
    op_movcl,                       /* 0x4d */
    op_movcm,                       /* 0x4e */
    op_movca,                       /* 0x4f */
    op_movdb,                       /* 0x50 */
    op_movdc,                       /* 0x51 */
    op_movdd,                       /* 0x52 */
    op_movde,                       /* 0x53 */
    op_movdh,                       /* 0x54 */
    op_movdl,                       /* 0x55 */
    op_movdm,                       /* 0x56 */
    op_movda,                       /* 0x57 */
    op_moveb,                       /* 0x58 */
    op_movec,                       /* 0x59 */
    op_moved,                       /* 0x5a */
    op_movee,                       /* 0x5b */
    op_moveh,                       /* 0x5c */
    op_movel,                       /* 0x5d */
    op_movem,                       /* 0x5e */
    op_movea,                       /* 0x5f */
    op_movhb,                       /* 0x60 */
    op_movhc,                       /* 0x61 */
    op_movhd,                       /* 0x62 */
    op_movhe,                       /* 0x63 */
    op_movhh,                       /* 0x64 */
    op_movhl,                       /* 0x65 */
    op_movhm,                       /* 0x66 */
    op_movha,                       /* 0x67 */
    op_movlb,                       /* 0x68 */
    op_movlc,                       /* 0x69 */
    op_movld,                       /* 0x6a */
    op_movle,                       /* 0x6b */
    op_movlh,                       /* 0x6c */
    op_movll,                       /* 0x6d */
    op_movlm,                       /* 0x6e */
    op_movla,                       /* 0x6f */
    op_movmb,                       /* 0x70 */
    op_movmc,                       /* 0x71 */
    op_movmd,                       /* 0x72 */
    op_movme,                       /* 0x73 */
    op_movmh,                       /* 0x74 */
    op_movml,                       /* 0x75 */
    op_hlt,                         /* 0x76 */
    op_movma,                       /* 0x77 */
    op_movab,                       /* 0x78 */
    op_movac,                       /* 0x79 */
    op_movad,                       /* 0x7a */
    op_movae,                       /* 0x7b */
    op_movah,                       /* 0x7c */
    op_moval,                       /* 0x7d */
    op_movam,                       /* 0x7e */
    op_movaa,                       /* 0x7f */
    op_addb,                        /* 0x80 */
    op_addc,                        /* 0x81 */
    op_addd,                        /* 0x82 */
    op_adde,                        /* 0x83 */
    op_addh,                        /* 0x84 */
    op_addl,                        /* 0x85 */
    op_addm,                        /* 0x86 */
    op_adda,                        /* 0x87 */
    op_adcb,                        /* 0x88 */
    op_adcc,                        /* 0x89 */
    op_adcd,                        /* 0x8a */
    op_adce,                        /* 0x8b */
    op_adch,                        /* 0x8c */
    op_adcl,                        /* 0x8d */
    op_adcm,                        /* 0x8e */
    op_adca,                        /* 0x8f */
    op_subb,                        /* 0x90 */
    op_subc,                        /* 0x91 */
    op_subd,                        /* 0x92 */
    op_sube,                        /* 0x93 */
    op_subh,                        /* 0x94 */
    op_subl,                        /* 0x95 */
    op_subm,                        /* 0x96 */
    op_suba,                        /* 0x97 */
    op_sbbb,                        /* 0x98 */
    op_sbbc,                        /* 0x99 */
    op_sbbd,                        /* 0x9a */
    op_sbbe,                        /* 0x9b */
    op_sbbh,                        /* 0x9c */
    op_sbbl,                        /* 0x9d */
    op_sbbm,                        /* 0x9e */
    op_sbba,                        /* 0x9f */
    op_anab,                        /* 0xa0 */
    op_anac,                        /* 0xa1 */
    op_anad,                        /* 0xa2 */
    op_anae,                        /* 0xa3 */
    op_anah,                        /* 0xa4 */
    op_anal,                        /* 0xa5 */
    op_anam,                        /* 0xa6 */
    op_anaa,                        /* 0xa7 */
    op_xrab,                        /* 0xa8 */
    op_xrac,                        /* 0xa9 */
    op_xrad,                        /* 0xaa */
    op_xrae,                        /* 0xab */
    op_xrah,                        /* 0xac */
    op_xral,                        /* 0xad */
    op_xram,                        /* 0xae */
    op_xraa,                        /* 0xaf */
    op_orab,                        /* 0xb0 */
    op_orac,                        /* 0xb1 */
    op_orad,                        /* 0xb2 */
    op_orae,                        /* 0xb3 */
    op_orah,                        /* 0xb4 */
    op_oral,                        /* 0xb5 */
    op_oram,                        /* 0xb6 */
    op_oraa,                        /* 0xb7 */
    op_cmpb,                        /* 0xb8 */
    op_cmpc,                        /* 0xb9 */
    op_cmpd,                        /* 0xba */
    op_cmpe,                        /* 0xbb */
    op_cmph,                        /* 0xbc */
    op_cmpl,                        /* 0xbd */
    op_cmpm,                        /* 0xbe */
    op_cmpa,                        /* 0xbf */
    op_rnz,                         /* 0xc0 */
    op_popb,                        /* 0xc1 */
    op_jnz,                         /* 0xc2 */
    op_jmp,                         /* 0xc3 */
    op_cnz,                         /* 0xc4 */
    op_pushb,                       /* 0xc5 */
    op_adin,                        /* 0xc6 */
    op_rst0,                        /* 0xc7 */
    op_rz,                          /* 0xc8 */
    op_ret,                         /* 0xc9 */
    op_jz,                          /* 0xca */
    op_jmp,                         /* 0xcb */
    op_cz,                          /* 0xcc */
    op_call,                        /* 0xcd */
    op_acin,                        /* 0xce */
    op_rst1,                        /* 0xcf */
    op_rnc,                         /* 0xd0 */
    op_popd,                        /* 0xd1 */
    op_jnc,                         /* 0xd2 */
    op_out,                         /* 0xd3 */
    op_cnc,                         /* 0xd4 */
    op_pushd,                       /* 0xd5 */
    op_suin,                        /* 0xd6 */
    op_rst2,                        /* 0xd7 */
    op_rc,                          /* 0xd8 */
    op_ret,                         /* 0xd9 */
    op_jc,                          /* 0xda */
    op_in,                          /* 0xdb */
    op_cc,                          /* 0xdc */
    op_call,                        /* 0xdd */
    op_sbin,                        /* 0xde */
    op_rst3,                        /* 0xdf */
    op_rpo,                         /* 0xe0 */
    op_poph,                        /* 0xe1 */
    op_jpo,                         /* 0xe2 */
    op_xthl,                        /* 0xe3 */
    op_cpo,                         /* 0xe4 */
    op_pushh,                       /* 0xe5 */
    op_anin,                        /* 0xe6 */
    op_rst4,                        /* 0xe7 */
    op_rpe,                         /* 0xe8 */
    op_pchl,                        /* 0xe9 */
    op_jpe,                         /* 0xea */
    op_xchg,                        /* 0xeb */
    op_cpe,                         /* 0xec */
    op_call,                        /* 0xed */
    op_xrin,                        /* 0xee */
    op_rst5,                        /* 0xef */
    op_rp,                          /* 0xf0 */
    op_poppsw,                      /* 0xf1 */
    op_jp,                          /* 0xf2 */
    op_di,                          /* 0xf3 */
    op_cp,                          /* 0xf4 */
    op_pushpsw,                     /* 0xf5 */
    op_orin,                        /* 0xf6 */
    op_rst6,                        /* 0xf7 */
    op_rm,                          /* 0xf8 */
    op_sphl,                        /* 0xf9 */
    op_jm,                          /* 0xfa */
    op_ei,                          /* 0xfb */
    op_cm,                          /* 0xfc */
    op_call,                        /* 0xfd */
    op_cpin,                        /* 0xfe */
    op_rst7                         /* 0xff */
  };

  do {
    tstates += (*op_sim[memrdr(PC8++)]) ();    // execute next opcode
  } while (State == Running);
}

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

void setup()
{
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  while (!Serial)
    ; // Wait for serial port to connect. Needed for native USB
  randomSeed(analogRead(UAP));
  init_cpu();
}

void loop()
{
  // put your main code here, to run repeatedly:
  // variables for measuring the run time
  unsigned long start, stop;

  Serial.println(F("Arduino Nano Intel 8080 emulator version 1.1"));
  Serial.println(F("Copyright (C) 2024 by Udo Munk"));
  Serial.println();

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
  display_freeram();
  Serial.println();
  Serial.flush();
  exit(0);
}
