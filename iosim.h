//
// Intel 8080 CPU emulation on an Arduino Nano
//            derived from Z80PACK
// Copyright 2024, Udo Munk
//
// This module implements the I/O functions for the 8080 CPU.
//
// History:
// 04-MAY-2024 Release 1.0 implements a very basic 8080 system
// 10-MAY-2024 added frontpanel port, handle reads to UART ignoring status
// 21-MAY-2024 added FDC port
//

static BYTE sio_last;        // last byte read from the UART
       BYTE fp_value = 0x22; // the value for port 255 read

// I/O function port 0 read:
// read status of the Arduino UART and return:
// bit 0 = 0, character available for input from tty
// bit 7 = 0, transmitter ready to write character to tty
const static BYTE p000_in(void)
{
  register BYTE stat = 0b10000001;  // initially not ready

  if (Serial.available())           // check if there is input from tty
    stat &= 0b11111110;             // if so flip status bit
  if (Serial.availableForWrite())   // check if output to tty is possible
    stat &= 0b01111111;             // if so flip status bit
    
  return (stat);
}

// I/O function port 1 read:
// Read byte from Arduino UART.
const static BYTE p001_in(void)
{
  if (!Serial.available()) // someone reading without checking first
    return (sio_last);
  else {
    sio_last = Serial.read();
    return (sio_last);
  }
}

// This array contains function pointers for every input
// I/O port (0 - 255), to do the required I/O.
// To save memory we use the first 5 ports only.
const static BYTE (*port_in[5]) (void) = {
  p000_in,                // port 0
  p001_in,                // port 1
  0,                      // port 2
  0,                      // port 3
  fdc_in                  // port 4
};

// read a byte from 8080 CPU I/O
BYTE io_in(BYTE addrl, BYTE addrh)
{
  if ((addrl <= 4) && (*port_in[addrl] != 0)) // for now we use 0-4
    return ((*port_in[addrl]) ());
  else if (addrl == 255)                      // and 255, frontpanel port
    return (fp_value);
  else
    return (0xff);                            // all other return 0xff
}

// I/O function port 0 write:
// Switch builtin LED on/off.
const static void p000_out(BYTE data)
{
  if (!data)
    digitalWrite(LED_BUILTIN, false); // 0 switches LED off
  else
    digitalWrite(LED_BUILTIN, true);  // everything else on
}

// I/O function port 1 write:
// Write byte to Arduino UART.
const static void p001_out(BYTE data)
{
  Serial.write(data & 0x7f); // strip parity, some software won't
}

// This array contains function pointers for every output
// I/O port (0 - 255), to do the required I/O.
// To save memory we use the first 5 ports only.
const static void (*port_out[5]) (BYTE) = {
  p000_out,               // port 0
  p001_out,               // port 1
  0,                      // port 2
  0,                      // port 3
  fdc_out                 // port 4
};

// Write a byte to 8080 CPU I/O
void io_out(BYTE addrl, BYTE addrh, BYTE data)
{
  if ((addrl <= 4) && (*port_out[addrl] != 0)) // for now we use 0-4
      (*port_out[addrl]) (data);               // and all other do nothing
}
