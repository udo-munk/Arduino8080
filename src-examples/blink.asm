;
; Example program for the Arduino 8080
; Blink builtin LED for a bit
;
; Udo Munk, April 2024
;

	.8080			; we use Intel 8080 syntax
	ORG 0000H		; starting at memory location 0

LED	EQU  0			; builtin LED port

	LXI  SP,0100H		; set stack far enough in upper memory

	MVI  H,10		; blink LED 10 times

L1	MVI  A,1		; switch LED on
	OUT  LED
	LXI  B,0		; wait a second
L2	DCX  B
	MOV  A,B
	ORA  C
	JNZ  L2
	MVI  A,0		; switch LED off
	OUT  LED
	LXI  B,0		; wait a second
L3	DCX  B
	MOV  A,B
	ORA  C
	JNZ  L3

	DCR  H
	JNZ  L1			; again if not done

	HLT			; done, halt CPU

	END			; of program
