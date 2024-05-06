;
; Example program for the Arduino 8080
; Do some serial I/O
;
; Udo Munk, April 2024
;

	.8080		; we use 8080 syntax

TTYSTA	EQU  0		; tty status port
TTYDAT	EQU  1		; tty data port

CR	EQU  13		; carriage return
LF	EQU  10		; line feed

	ORG  0000H	; starting at memory location 0 in ROM

	DI		; disable interrupts
	LXI  SP,0A00H	; set stack to top of RAM

			; print instructions
	LXI  H,TEXT	; HL points to text to send
LOOP	MOV  A,M	; get next character into A
	ORA  A		; is it 0 termination?
	JZ   ECHO	; if yes done
	CALL OUTCH	; output character to tty
	INX  H		; point to next character
	JMP  LOOP	; and again

			; now echo what is typed
ECHO	IN   TTYSTA	; get tty status
	RRC		; is there any input?
	JC   ECHO	; wait if not
	IN   TTYDAT	; get character from tty into A
	CALL OUTCH	; echo it
	CPI  'X'	; is it a X?
	JZ   DONE	; done if so
	CPI  CR		; is it a carriage return?
	JNZ  ECHO	; no, go on
	MVI  A,LF	; else also send line feed
	CALL OUTCH
	JMP  ECHO	; repeat

DONE	HLT		; done

			; output character in A to tty
OUTCH	PUSH PSW	; save character in A
OUTCH1	IN   TTYSTA	; get tty status
	RLC		; output ready?
	JC   OUTCH1	; wait if not
	POP  PSW	; get character back into A
	OUT  TTYDAT	; send character to tty
	RET

TEXT	DEFM "Hello from the 8080 CPU"
	DEFB CR,LF
	DEFM "I will echo what you type, type X if done"
	DEFB CR,LF,0

	END
