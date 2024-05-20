//
// Intel 8080 CPU emulation on an Arduino Nano
//            derived from Z80PACK
// Copyright 2024, Udo Munk
//
// History:
// 04-MAY-2024 Release 1.0, initial implementation
// 17-MAY-2024 use X and Y as names for the undocumented flag bits
//

// bit definitions for the 8080 CPU flags
#define S_FLAG		128
#define Z_FLAG		64
#define Y_FLAG		32
#define H_FLAG		16
#define X_FLAG		8
#define P_FLAG		4
#define N_FLAG		2
#define C_FLAG		1

#define S_SHIFT		7
#define Z_SHIFT		6
#define Y_SHIFT		5
#define H_SHIFT		4
#define X_SHIFT		3
#define P_SHIFT		2
#define N_SHIFT		1
#define C_SHIFT		0

// possible states of the 8080 CPU
enum CPUState { Running = 1, Halted = 2 };
