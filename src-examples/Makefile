all :		serial.c echo.c turnmon.c
	@echo
	@echo "Done."
	@echo

serial.c :	serial.asm
	z80asm -fc -l -sn -p0 $?

echo.c :	echo.asm
	z80asm -fc -l -sn -p0 $?

turnmon.c :	turnmon.asm
	z80asm -fc -l -sn -p0 -8 $?
