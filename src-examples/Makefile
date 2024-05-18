all :		tb.bin test8080.bin
	@echo
	@echo "Done."
	@echo

clean :
	rm -f tb.bin

tb.bin :	tb.asm
	z80asm -8 -fb -l -sn -p0 -x $?

test8080.bin:	test8080.asm
	z80asm -8 -doncpm -fb -l -sn -p0 $?
