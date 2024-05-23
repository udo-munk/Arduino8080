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

// memory dump
void mem_dump(WORD addr)
{
  uint32_t i, j;
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

// load a file 'name' into FRAM
void load_file(char *name)
{
  uint32_t i = 0;
  unsigned char c;
  FatFile sd_file;
  char SFN[25];
 
  strcpy(SFN, "/CODE80/");
  strcat(SFN, name);
  strcat(SFN, ".BIN");

#ifdef DEBUG
  Serial.print(F("Filename: "));
  Serial.println(SFN);
#endif

  if (!sd_file.openExistingSFN(SFN)) {
    Serial.println(F("File not found\n"));
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
  FatFile sd_file;
  char SFN[25];

  strcpy(SFN, "/DISKS80/");
  strcat(SFN, name);
  strcat(SFN, ".DSK");

#ifdef DEBUG
  Serial.print(F("Filename: "));
  Serial.println(SFN);
#endif

  if (!sd_file.openExistingSFN(SFN)) {
    Serial.println(F("File not found\n"));
    return;
  }

  sd_file.close();
  strcpy(disks[drive], SFN);
}

// read from drive a sector on track into FRAM addr
int read_sec(int8_t drive, int8_t track, int8_t sector, WORD addr)
{
  FatFile sd_file;
  WORD a = addr;
  BYTE c;
  uint32_t pos;
  int i;

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

  // read sector into FRAM
  for (i = 0; i < SEC_SZ; i++) {
    if (sd_file.read(&c, 1) != 1) {
      sd_file.close();
      return FDC_STAT_READ;
    }
    fram.write8(a++, c);
  }

  sd_file.close();
  return FDC_STAT_OK;
}

// write to drive a sector on track from FRAM addr
int write_sec(int8_t drive, int8_t track, int8_t sector, WORD addr)
{
  FatFile sd_file;
  WORD a = addr;
  BYTE c;
  uint32_t pos;
  int i;

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

  // write sector to disk image
  for (i = 0; i < SEC_SZ; i++) {
    c = fram.read8(a++);
    if (sd_file.write(&c, 1) != 1) {
      sd_file.close();
      return FDC_STAT_WRITE;
    }
  }

  sd_file.close();
  return FDC_STAT_OK;
}

// get FDC command from FRAM
void get_fdccmd(BYTE *cmd, WORD addr)
{
  fram.read(addr, cmd, 4);
}
