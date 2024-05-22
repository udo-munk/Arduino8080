//
// Intel 8080 CPU emulation on an Arduino Nano
//            derived from Z80PACK
// Copyright 2024, Udo Munk
//
// FDC for bare metal with disk images on SD drive
// Work in progress, don't try to use yet
// Needed for research what can be done with the memory left
//
// History:
// ??-MAY-2024
//

// offsets in disk command descriptor
#define DD_TRACK	0 // track number
#define DD_SECTOR	1 // sector number
#define DD_DMAL		2 // DMA address low
#define DD_DMAH		3 // DMA address high

// controller commands: MSB command, LSB drive number
#define FDC_SETDD	0x10  // set address of disk descriptor from
			                  // following two OUT's
#define FDC_READ	0x20  // read sector from disk
#define FDC_WRITE	0x40  // write sector to disk

// FDC status codes
#define FDC_STAT_OK      0 // command executed successfull
#define FDC_STAT_NODISK  1 // disk file open error
#define FDC_STAT_SEEK    2 // disk file seek error
#define FDC_STAT_READ    3 // disk file read error
#define FDC_STAT_WRITE   4 // disk file write error
#define FDC_STAT_DMA     5 // DMA memory read/write error

// floppy disk definition
#define SEC_SZ    128   // sector size
#define SPT       26    // sectors per track
#define TRK       77    // number of tracks

char disks[2][22];         // two disk images /DISKS80/filename.DSK
static BYTE fdc_state = 0; // state of the fdc state machine
       BYTE fdc_stat;      // status of last command
static WORD fdc_dd_addr;   // address of the disk descriptor

// I/O out interface to the 8080 CPU
const void fdc_out(BYTE data)
{
  switch (data & 0xf0) {
  case FDC_SETDD:
    break;

  case FDC_READ:
    break;

  case FDC_WRITE:
    break;
  }
}

// I/O in interface to the 8080 CPU
const BYTE fdc_in(void)
{
  return fdc_stat;
}
