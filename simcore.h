//
// Intel 8080 CPU emulation on an Arduino Nano
//            derived from Z80PACK
// Copyright 2024, Udo Munk
//
// History:
// 04-MAY-2024 Release 1.0

// bit definitions for the 8080 CPU flags
#define S_FLAG          128
#define Z_FLAG          64
#define N2_FLAG         32
#define H_FLAG          16
#define N1_FLAG         8
#define P_FLAG          4
#define N_FLAG          2
#define C_FLAG          1

// possible states of the 8080 CPU
enum CPUState { Running = 1, Halted = 2 };