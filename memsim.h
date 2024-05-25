//
// Intel 8080 CPU emulation on an Arduino Nano
//            derived from Z80PACK
// Copyright 2024, Udo Munk
//
// This module implements the low level functions to access external
// SPI memory. Supported is Adafruit FRAM, min 64 KByte for 8080 memory,
// and Adafruit MicroSD for standalone programms and disk images.
//
// Uses Arduino libraries:
// Adafruit BusIO
// Adafruit_FRAM_SPI
// SdFat - Adafruit Fork
//
// History:
// 04-MAY-2024 Release 1.0 implements a very basic 8080 system
// 06-MAY-2024 Release 1.1 add support for a ROM in flash
// 07-MAY-2024 Release 1.2 move 8080 memory into a FRAM
// 18-MAY-2024 Release 1.4 read 8080 code from a file on SD into FRAM
// 19-MAY-2024 Release 1.5 use SdFat lib instead of SD lib to save memory
// 21-MAY-2024 Release 1.6 added low level functions for disk images on SD
//

// 64 KB unbanked memory in FRAM
// we want hardware SPI
Adafruit_FRAM_SPI fram = Adafruit_FRAM_SPI(FRAM_CS);

// file handle, at any time we have only one file open
FatFile sd_file;

// transfer buffer for disk/memory transfers
static BYTE dsk_buf[SEC_SZ];

// setup FRAM
void init_memory(void)
{
  uint32_t i;

  // fill top page of 8080 memory with 0xff, write protected ROM
  for (i = 0xff00; i <= 0xffff; i++)
    fram.write8(i, 0xff);
}

// read a byte from 8080 CPU memory at address addr
static inline BYTE memrdr(WORD addr)
{
  return fram.read8((uint32_t) addr);
}

// write a byte data into 8080 CPU RAM at address addr 
static inline void memwrt(WORD addr, BYTE data)
{
  if (addr < 0xff00) // top memory page write protected for MITS BASIC
    fram.write8((uint32_t) addr, data);
}

#if 0	// for debugging, not enough memory left
// memory dump
void mem_dump(WORD addr)
{
  int i, j;
  WORD a = addr;
  BYTE c;

  for (j = 1; j <= 16; j++) {
    for (i = 1; i <= 16; i++) {
      c = fram.read8(a++);
      if (c < 0x10)
        Serial.print(F("0"));
      Serial.print(c, HEX);
      Serial.print(F(" "));
    }
    Serial.println();
  }
}
#endif

void complain(void)
{
  Serial.println(F("File not found\n"));
}

// load a file 'name' into FRAM
void load_file(char *name)
{
  uint32_t i = 0;
  unsigned char c;
  char SFN[25];
 
  strcpy(SFN, "/CODE80/");
  strcat(SFN, name);
  strcat(SFN, ".BIN");

#ifdef DEBUG
  Serial.print(F("Filename: "));
  Serial.println(SFN);
#endif

  if (!sd_file.openExistingSFN(SFN)) {
    complain();
    return;
  }

  while (sd_file.read(&c, 1))
    fram.write8(i++, c);

  sd_file.close();
  Serial.println();
}

// mount a disk image 'name' on disk 'drive'
void mount_disk(int8_t drive, char *name)
{
  char SFN[22];

  strcpy(SFN, "/DISKS80/");
  strcat(SFN, name);
  strcat(SFN, ".DSK");

#ifdef DEBUG
  Serial.print(F("Filename: "));
  Serial.println(SFN);
#endif

  if (!sd_file.openExistingSFN(SFN)) {
    complain();
    return;
  }

  sd_file.close();
  strcpy(disks[drive], SFN);
  Serial.println();
}

// prepare I/O for sector read and write routines
BYTE prep_io(int8_t drive, int8_t track, int8_t sector, WORD addr)
{
  uint32_t pos;

  // check if track and sector in range
  if (track > TRK)
    return FDC_STAT_TRACK;
  if ((sector < 1) || (sector > SPT))
    return FDC_STAT_SEC;

  // check if disk in drive
  if (!strlen(disks[drive])) {
    return FDC_STAT_NODISK;
  }

  // open file with the disk image
  if (!sd_file.openExistingSFN(disks[drive])) {
    return FDC_STAT_NODISK;
  }

  // seek to track/sector
  pos = (((uint32_t) track * (uint32_t) SPT) + sector - 1) * SEC_SZ;
  if (!sd_file.seekSet(pos)) {
    sd_file.close();
    return FDC_STAT_SEEK;
  }

  return FDC_STAT_OK;
}

// read from drive a sector on track into FRAM addr
BYTE read_sec(int8_t drive, int8_t track, int8_t sector, WORD addr)
{
  BYTE stat;

  // prepare for sector read
  if ((stat = prep_io(drive, track, sector, addr)) != FDC_STAT_OK)
    return stat;

  // read sector into FRAM
  if (sd_file.read(&dsk_buf[0], SEC_SZ) != SEC_SZ) {
    sd_file.close();
    return FDC_STAT_READ;
  }
  sd_file.close();
  if (!fram.write(addr, &dsk_buf[0], SEC_SZ)) {
    return FDC_STAT_DMA;
  }

  return FDC_STAT_OK;
}

// write to drive a sector on track from FRAM addr
BYTE write_sec(int8_t drive, int8_t track, int8_t sector, WORD addr)
{
  BYTE stat;

  // prepare for sector write
  if ((stat = prep_io(drive, track, sector, addr)) != FDC_STAT_OK)
    return stat;

  // write sector to disk image
  if (!fram.read(addr, &dsk_buf[0], SEC_SZ)) {
    sd_file.close();
    return FDC_STAT_DMA;
  }
  if (sd_file.write(&dsk_buf[0], SEC_SZ) != SEC_SZ) {
    sd_file.close();
    return FDC_STAT_WRITE;
  }

  sd_file.close();
  return FDC_STAT_OK;
}

// get FDC command from FRAM
void get_fdccmd(BYTE *cmd, WORD addr)
{
  fram.read(addr, cmd, 4);
}
