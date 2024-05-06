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

Udo Munk, April 2024

<><><><><><><><><><><><><><><><><><><><><><><>

The little 8080 computer looked promising, so in the next step we will
provide more memory. For this the AVR CPU allows to put data in flash
and get it out of the tiny RAM. Now this data needs to be accessed with
functions provided by the runtime, which will cost some performance.
Documentation is a bit spare about this, and others claim it costs a lot
of performance. Our CPU core provides exact run time statistics, so I
just have a look my self.

It doesn't cost a lot of performance, about 10% slower, but for that we
now got 2KB ROM and 512 Bytes RAM, a very good tradeoff. And with this
"much" memory we now can run some more interesting 8080 programs, like
Li-Chen Wang's famous TINY BASIC.

Udo Munk, May 2024
