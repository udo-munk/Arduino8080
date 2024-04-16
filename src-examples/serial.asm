;
; Example program for the Arduino 8080
; Sends some text to the serial I/O port
;
; Udo Munk, April 2024
;
	.8080			; we want 8080 code
	ORG 0000H		; starting at memory location 0

	LXI  SP,0100H		; set SP somewhere in upper memory
	LXI  D,TEXT		; DE points to the text we want to send
LOOP	LDAX D			; get next character into A
	INX  D			; increment pointer to the text
	ORA  A			; 0 termination?
	JZ   DONE		; if so we are done
        OUT  0			; output character to I/O port 0
        JP   LOOP		; and repeat until done

DONE	HLT			; we are done, halt CPU

				; text we want to send to serial port
TEXT	DEFM "Hello from the 8080 CPU"
	DEFB 13,10,0		; carriage return, linefeed and string termination

	END			; of program
