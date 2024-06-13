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

Now with knowing this, we even can move more stuff into flash and free up
some more RAM. For example the parity table, not every instruction
computes parity, so that will reduce our performance less than another
10%, depends on the instructions a program uses. Acceptable and now we
can increase RAM to 1 KB.
Another large array we could move is the op-code jump table, but that
will reduce our speed for every single op-code fetch, for now we leave
this as is. To summarize, now we have:

	Intel 8080 CPU running at ca. 0.8 MHz
	UART for serial communication
	2 KB ROM for program code @ 0000H - 07FFH
	1 KB RAM for data	  @ 0800H - 0BFFH

And all that on a tiny piece of hardware smaller than a 8080 CPU chip,
leave alone a complete computer system.

Udo Munk, May 2024

<><><><><><><><><><><><><><><><><><><><><><><>

For going further with our litte machine we need to use external memory.
After much reading what is available I decided to use Adafruit FRAM SPI
memory. These are said to be fast, writable like normal RAM without tearing
it down like flash or EEPROM. And it even is persistent memory, which
keeps contents without power. Also Adafruit provides a library to drive
the FRAM via SPI, so we don't have to understand all the low level details
of this.

Today my modules arrived, soldered in the header for usage on a breadboard
and connected it to the Arduino Nano. Adafruit also provides some example
programs, so let's see if I got everything correct:

	18:13:51.136 -> FRAM Size = 0x80000
	18:13:51.136 -> Found SPI FRAM
	18:13:52.646 -> SPI FRAM address size is 3 bytes.
	18:13:52.680 -> SPI FRAM capacity appears to be..
	18:13:52.714 -> 524288 bytes
	18:13:52.714 -> 512 kilobytes
	18:13:52.755 -> 4096 kilobits
	18:13:52.755 -> 4 megabits

Looks good, and now we have a big 8080 computer system with:

	8080 CPU running with 0.04 MHz
	UART for serial communication
	64 KB RAM

While the FRAM chips can be clocked up to 40 Mhz, with the Arduino Nano
we are limited to SPI clock frequency 8 MHz, so we have to live with it.

Now, we still have a copy of our code in flash, so instead of copying
the code into the FRAM, we still can execute it from flash and use
the FRAM only for data. This will give us the following:

	8080 CPU running with 0.14 MHz
	UART for serial communication
	 2 KB ROM for program code @ 0000H - 07FFH
	62 KB RAM for data	   @ 0800H - FFFFH

Some final thoughts:

I got a 512 MB FRAM module, just because one cannot have enough memory, right?
Wrong, because for accessing a byte in this memory we must send it 3 bytes with
the address. For a 64 KB module 2 byte addresses should improve performance by
reducing the overhead. If someone is going to try this please tell us what you
get.

Udo Munk, May 2024

<><><><><><><><><><><><><><><><><><><><><><><>

Got some more stuff delivered today and after some soldering and wiring:

	Initializing SD card...Wiring is correct and a card is present.

	Card type:         SDHC
	Clusters:          473504
	Blocks x Cluster:  64
	Total Blocks:      30304256

	Volume type is:    FAT32
	Volume size (Kb):  15152128
	Volume size (Mb):  14797
	Volume size (Gb):  14.45

	Files found on the card (name, date and size in bytes):
	SYSTEM~1/     2024-05-14 15:02:00
	  WPSETT~1.DAT  2024-05-14 15:02:00 12
	  INDEXE~1      2024-05-14 15:02:00 76
	DATEI1.TXT    2024-05-14 15:02:42 24
	DATEI2.TXT    2024-05-14 15:03:56 61

Excellent, then let's see how to make use of this ...
After adding the SD library and initializing the device we have:

	Sketch uses 30028 bytes (97%) of program storage space. Maximum is 30720 bytes.
	Global variables use 1555 bytes (75%) of dynamic memory, leaving 493 bytes for local variables. Maximum is 2048 bytes.

Now it will get interesting, guess I'll need a while ;-)
Actually it is even worse than I thought already, just from trying to open
one file on the SD card:

	Sketch uses 32198 bytes (104%) of program storage space. Maximum is 30720 bytes.
	Global variables use 1581 bytes (77%) of dynamic memory, leaving 467 bytes for local variables. Maximum is 2048 bytes.
	Compilation error: text section exceeds available space in board

Udo Munk, May 2024

<><><><><><><><><><><><><><><><><><><><><><><>

In this version I have replaced the CPU implementation with a code size
optimized one from Thomas Eberhardt, everything else is the same as before.
Hopefully this will free enough memory to make use of the SD drive now.

Success, we do not compile any 8080 code into the application anymore,
instead we read our code from a MicroSD drive, and we are not limited
to 4 KB code size. However, free memory left is very very little, so
this machine will always read /code80.bin from the MicroSD and then
run the 8080 with whatever stuff you put in there.
Anyway, we have the following machine now:

	8080 CPU running with 0.04 MHz
	MITS Altair 88-SIO Rev. 1 for serial communication
	RAM @ 0000H - FEFFH
	ROM @ FF00H - FFFFH
	32 GB floppy disk drive

Udo Munk, May 2024

<><><><><><><><><><><><><><><><><><><><><><><>

Now that we have tweaked our CPU, the other component we can look at
for code optimizations is the SD library. While this library is great,
it includes lot's of stuff we don't need here, all we want is reading
files with good old 8.3 filenames.

Please install the following library in your Arduino IDE:

	SdFat - Adafruit Fork

This one can be better customisized for our needs here, and with doing
this we save a few KB code, that we can use to improve our application
now :-)

For example now we can implement a dialog that lets us enter the filename,
without the need to copy it to a fixed name. On your MicroSD card create
a directory CODE80 and copy all your 8080 binaries into this directory.
Filename to enter is just the name, without extension, which must be .bin.

Udo Munk, May 2024

<><><><><><><><><><><><><><><><><><><><><><><>

Now that we can load any code from the huge disk drive into the 8080
computer system we have a problem, when the software isn't working,
runs away and the whole machine seems to be stuck.
For this I added an interrupt key, which will stop the 8080 from
whatever it is doing. And then a register dump also is printed
to the terminal, so that we might get an idea what it was doing.

Udo Munk, May 2024

<><><><><><><><><><><><><><><><><><><><><><><>

	8080-SIM v1.6

	1 - port 255 value: 0x22
	2 - load file
	3 - Disk 0:
	4 - Disk 1:
	5 - run machine

	Command: 3

	Filename: cpm22
	1 - port 255 value: 0x22
	2 - load file
	3 - Disk 0: /DISKS80/cpm22.DSK
	4 - Disk 1:
	5 - run machine

	Command: 5

	64K CP/M 2.2 VERS B01

	A>dir
	A: ASM      COM : DDT      COM : ED       COM : MOVCPM   COM
	A: DUMP     COM : LOAD     COM : PIP      COM : STAT     COM
	A: SYSGEN   COM : XSUB     COM : WM       HLP : WM       COM
	A: BYE      COM : CLS      COM : SURVEY   MAC : SURVEY   COM
	A: XDIR     COM : SDIR     COM : VIEW     COM : SID      COM
	A: MAC      COM : MEMMAP   COM : SUBMIT   COM
	A>

Long live the Intel 8080!

Long live Digital Research CP/M!

--------------------------------------

No one is going to  believe this, but see your self ...
Thomas just released a code size optimized Z80 CPU implementation,
that still fits into the 32 KB flash memory of the Arduino Nano.
This Z80 is tested on UNIX workstations inside the z80pack framework
with code exercisers, to make sure it works the same as Zilog/Mostek
silicon with regards to the documented behaviour.

There are now two project files:

	arduino_8080.ino.8080
	arduino_8080.ino.z80

and arduino_8080.ino is a link to one of them. Currently the
project is setup to use the new Z80 CPU, but please be aware
that Microsoft BASIC only runs on a 8080 CPU, will get stuck
on a Z80.

Of course now the comfort features from my TODO list won't fit
anymore, but one can't have it all I guess.

Long live the Zilog Z80 !

With this release we have implemented a real computer system
with the following components:

	8080 or Z80 CPU running with 0.04 MHz
	MITS Altair 88-SIO Rev. 1 for serial communication
	RAM @ 0000H - FEFFH
	ROM @ FF00H - FFFFH
	Floppy disk controller with two 8" floppy disk drives
	Huge 32 GB MicroSD drive, that stores floppy disk images,
	standalone programs and the system configuration

This computer system certainly will not win any speed award, but it should
receive some coolness award, because with a few tiny microelectronic circuits
it substitutes a complete S100 mainframe from ca. 1980.

And here a video that shows this system in action: https://youtu.be/DQ6wM0TVROI

Udo Munk, May 2024

<><><><><><><><><><><><><><><><><><><><><><><>
