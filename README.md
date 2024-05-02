Intel 8080 CPU emulation running on an Arduino Nano, derived from z80pack

I managed to get the z80pack 8080 core squeezed into 32KB flash and 2 KB RAM
of the Arduino Nano, I wasn't certain it would fit before trying. This is the
smallest computer system I used so far for z80pack machines.

All you need is an Arduino Nano board, some PC, an USB cable and the
Arduino IDE for compiling the program und uploading it into the device. The
builtin LED shows when the 8080 emulation stopped running for some reason.

With 512 bytes RAM for the 8080 we can run very small programs only, but the
first Altair users got their systems with 256 bytes and used it. Included
are some 8080 example programs to show how.

A video that shows it in action is available: https://youtu.be/wKuIKwkP6O8

Of course one can add external memory, a SD card reader for disk images,
so that it could run CP/M and so on. I don't know if I will get so
far in this life, but you don't have to wait for me. There are some other
good looking projects, that already might do what you want:

https://github.com/WA6YDQ/nano80

https://forum.arduino.cc/t/cp-m-computer-on-arduino-nano-3-0/446350

Udo Munk, April 2024
