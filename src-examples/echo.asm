;
; Example program for the Arduino 8080
; Echos text on the serial I/O port
;
; Udo Munk, April 2024
;

	.8080			; we use Intel 8080 syntax
	ORG 0000H		; starting at memory location 0

CONST 	EQU  0			; console status port
CONDA	EQU  1			; console data port

	LXI  SP,0100H		; set stack far enough in upper memory
	LXI  D,TEXT		; DE points to the text we want to send
LOOP1	LDAX D			; get next character into A
	INX  D			; increment pointer to the text
	ORA  A			; 0 termination?
	JZ   INCH		; if so we are done
	OUT  CONDA		; output character in A to console
	JMP  LOOP1		; and repeat until done
INCH	IN   CONST		; get console status into A
	RRC			; is input available?
	JNC  INCH		; if not try again
	IN   CONDA		; get input into A
	ANI  7FH		; strip parity
	PUSH PSW		; save our data on stack
LOOP2	IN   CONST		; get console status into A
	RLC			; is output ready?
	JC   LOOP2		; if not try again
	POP  PSW		; get our data back into A
	OUT  CONDA		; echo to console
	JMP  INCH		; and do again

TEXT	DEFM "The 8080 CPU will echo what you type"
	DEFB 13,10,0		; carriage return, linefeed and string termination

	END			; of program
