all :		serial.c turnmon.c blink.c banner.c
	@echo
	@echo "Done."
	@echo

clean :
	rm -f *.c

serial.c :	serial.asm
	z80asm -fc -l -sn -p0 $?

turnmon.c :	turnmon.asm
	z80asm -fc -l -sn -p0 -8 $?

blink.c :	blink.asm
	z80asm -fc -l -sn -p0 $?

banner.c :	banner.asm
	z80asm -fc -l -sn -p0 $?
