all :		tb.c
	@echo
	@echo "Done."
	@echo

clean :
	rm -f tb.bin

tb.c :		tb.asm
	z80asm -8 -fb -l -sn -p0 -x $?
