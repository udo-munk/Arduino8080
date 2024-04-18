//
// Intel 8080 CPU emulation on an Arduino Nano
//            derived from Z80PACK
// Copyright 2024, Udo Munk
//

// I/O function port 0 read:
// read status of the Arduino tty and return:
// bit 0 = 0, character available for input from tty
// bit 7 = 0, transmitter ready to write character to tty
// bit 7 is always 0 for now, because we cannot get easily at this
static BYTE p000_in(void)
{
  if (Serial.available())
    return(0);
  else
    return(1);
}

// I/O function port 1 read:
// Read byte from Arduino tty.
static BYTE p001_in(void)
{
  while (!Serial.available())   // block until data available
    ;                           // compatible with z80sim
  return((BYTE)Serial.read());  // read data
}

// This array contains function pointers for every input
// I/O port (0 - 255), to do the required I/O.
const static BYTE (*port_in[5]) (void) = {
  p000_in,                // port 0
  p001_in                 // port 1
};

// read a byte from 8080 CPU I/O
static BYTE io_in(BYTE addrl, BYTE addrh)
{
  if (addrl <= 1) // for now we can use 0 and 1 only
    return((*port_in[addrl]) ());
  else
    return(0xff); // all other return 0xff
}

// I/O function port 1 write:
// Write byte to Arduino tty.
static void p001_out(BYTE data)
{
  Serial.write(data);
}

// This array contains function pointers for every output
// I/O port (0 - 255), to do the required I/O.
const static void (*port_out[5]) (BYTE) = {
  0,                      // port 0
  p001_out                // port 1
};

// write a byte to 8080 CPU I/O
static void io_out(BYTE addrl, BYTE addrh, BYTE data)
{
  if (addrl == 1) // for now we can use 1 only, all other do nothing
    (*port_out[addrl]) (data);
}
