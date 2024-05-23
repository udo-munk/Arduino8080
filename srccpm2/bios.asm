;	8080 CBIOS for Arduino 8080
;
;	Copyright (C) 2024 by Udo Munk
;
MSIZE	EQU	64		;cp/m version memory size in kilobytes
;
;	"bias" is address offset from 3400H for memory systems
;	than 16K (referred to as "b" throughout the text).
;
BIAS	EQU	(MSIZE-20)*1024
CCP	EQU	3400H+BIAS	;base of ccp
BDOS	EQU	CCP+806H	;base of bdos
BIOS	EQU	CCP+1600H	;base of bios
NSECTS	EQU	(BIOS-CCP)/128	;warm start sector count
CDISK	EQU	0004H		;current disk number 0=A,...,15=P
IOBYTE	EQU	0003H		;intel i/o byte
;
;	I/O ports
;
CONSTA	EQU  0		; console status port
CONDAT	EQU  1		; console data port

	ORG     BIOS            ;origin of BIOS
;
;	jump vector for individual subroutines
;
	JMP	BOOT		;cold boot
WBE	JMP	WBOOT		;warm start
	JMP	CONST		;console status
	JMP	CONIN		;console character in
	JMP	CONOUT		;console character out
	JMP	LIST		;list character out
	JMP	PUNCH		;punch character out
	JMP	READER		;reader character in
	JMP	HOME		;move disk head to home position
	JMP	SELDSK		;select disk drive
	JMP	SETTRK		;set track number
	JMP	SETSEC		;set sector number
	JMP	SETDMA		;set dma address
	JMP	READ		;read disk sector
	JMP	WRITE		;write disk sector
	JMP	LISTST		;list status
	JMP	SECTRAN		;sector translate
;
;	data tables
;
SIGNON	DB	MSIZE / 10 + '0',MSIZE MOD 10 + '0'
	DB	'K CP/M 2.2 VERS B01',13,10,0
;
;	print a message to the console
;	pointer to string in hl
;
PRTMSG	MOV	A,M		;get next message byte
	ORA	A		;is it zero?
	RZ			;yes, done
	MOV	C,A		;no, print character on console
	CALL	CONOUT
	INX	H		;and do next
	JMP	PRTMSG
;
;	cold start
BOOT	LXI	SP,80H		;use space below buffer for stack
	LXI     H,SIGNON        ;print signon
	CALL	PRTMSG

	HLT



WBOOT
CONST
CONIN
	RET
;
;	console output
;
CONOUT	IN	CONSTA		;get status
	RLC			;test bit 7
	JC	CONOUT		;wait until transmitter ready
	MOV	A,C		;get character into accumulator
	OUT	CONDAT		;send to console
	RET



LIST
PUNCH
READER
HOME
SELDSK
SETTRK
SETSEC
SETDMA
READ
WRITE
LISTST
SECTRAN
	RET

	END			;of CBIOS
