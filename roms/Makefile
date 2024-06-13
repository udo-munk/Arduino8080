Z80ASMDIR = ~/work/z80pack/z80asm
Z80ASM = $(Z80ASMDIR)/z80asm
Z80ASMFLAGS = -l -T -sn -p0

all: ../bootrom.h

../bootrom.h: bootrom.asm
	$(Z80ASM) $(Z80ASMFLAGS) -fc -o../bootrom.h $<
