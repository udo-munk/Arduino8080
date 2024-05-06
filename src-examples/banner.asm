;
; Example program for the Arduino 8080
; Print cool banner on the serial I/O port
;
; Udo Munk, April 2024
;

	.8080			; we use Intel 8080 syntax
	ORG  0000H		; starting at memory location 0 in ROM

CR	EQU  13			; carriage return
LF	EQU  10			; line feed

CONST 	EQU  0			; console status port
CONDA	EQU  1			; console data port

	DI			; disable interrupts
	LXI  SP,0A00H		; set stack to top of RAM

	LXI  D,BANNER		; DE points to the array with text to send
	MVI  B,7		; 7 lines to print
LOOP	CALL PRTLN		; print a line on console
	DCR  B			; next one
	JNZ  LOOP
	HLT			; done

PRTLN	IN   CONST		; get console status into A
	RLC			; is output ready?
	JC   PRTLN		; if not try again
PRTLN1	LDAX D			; get next character into A
	INX  D			; increment pointer to the text
	ORA  A			; 0 termination?
	RZ			; if so we are done
	OUT  CONDA		; output character in A to I/O port 1 (tty)
	JMP  PRTLN1		; and repeat until done

BANNER	DEFM " #####    ###     #####    ###            #####    ###   #     #"
	DEFB CR,LF,0
	DEFM "#     #  #   #   #     #  #   #          #     #    #    ##   ##"
	DEFB CR,LF,0
	DEFM "#     # #     #  #     # #     #         #          #    # # # #"
	DEFB CR,LF,0
	DEFM " #####  #     #   #####  #     #  #####   #####     #    #  #  #"
	DEFB CR,LF,0
	DEFM "#     # #     #  #     # #     #               #    #    #     #"
	DEFB CR,LF,0
	DEFM "#     #  #   #   #     #  #   #          #     #    #    #     #"
	DEFB CR,LF,0
	DEFM " #####    ###     #####    ###            #####    ###   #     #"
	DEFB CR,LF,0

	END			; of program
