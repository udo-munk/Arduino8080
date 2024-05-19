all :		serial.bin banner.bin tb.bin test8080.bin
	@echo
	@echo "Done."
	@echo

clean :
	rm -f serial.bin banner.bin tb.bin test8080.bin

serial.bin :	serial.asm
	z80asm -8 -fb -l -sn -p0 $?

banner.bin :	banner.asm
	z80asm -8 -fb -l -sn -p0 $?

tb.bin :	tb.asm
	z80asm -8 -fb -l -sn -p0 -x $?

test8080.bin:	test8080.asm
	z80asm -8 -doncpm=0 -fb -l -sn -p0 $?
