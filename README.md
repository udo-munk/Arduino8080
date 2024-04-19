Intel 8080 CPU emulation running on an Arduino Nano, derived from z80pack

I managed to get the z80pack 8080 core squeezed into 32KB flash and 2 KB RAM
of the Arduino Nano. This is the smallest computer system I used so far for
z80pack machines.

So far all you need is an Arduino Nano board, some PC, an USB cable and the
Arduino IDE for compiling the program und uploading it into the device. The
builtin LED shows when the 8080 emulation stopped running for some reason.
Reasons are the 8080 executed a HLT instruction, or the emulation ran into
some problem.

With the tiny 512 bytes RAM we can run very small programs only, but the
first Altair users got their systems with 256 bytes and were happy. Of
course one can add external memory, a SD card reader for disk immages,
so that it could run CP/M and so on. I don't know if I will go this
far in this life, but you don't have to wait for me. Here is another
good looking project, that already might do all you want:
https://github.com/WA6YDQ/nano80

Udo Munk, April 2024
