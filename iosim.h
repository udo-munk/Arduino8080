//
// Intel 8080 CPU emulation on an Arduino Nano
//            derived from Z80PACK
// Copyright 2024, Udo Munk
//

// read a byte from 8080 CPU I/O
static BYTE io_in(BYTE addrl, BYTE addrh)
{
}

// I/O function port 0 write:
// Write byte to Arduino tty.
static void p000_out(BYTE data)
{
  Serial.print(data);
}

// This array contains function pointers for every output
// I/O port (0 - 255), to do the required I/O.
const static void (*port_out[1]) (BYTE) = {
  p000_out                // port 0
};

// write a byte to 8080 CPU I/O
static void io_out(BYTE addrl, BYTE addrh, BYTE data)
{
  if (addrl == 0) // for now we can use 0 only
    (*port_out[addrl]) (data);
}