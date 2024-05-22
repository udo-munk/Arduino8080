;	CP/M 2.2 boot-loader for Arduino 8080
;
;	Copyright (C) 2024 by Udo Munk
;
	ORG	0		;mem base of boot
;
MSIZE	EQU	64		;mem size in kbytes
;
BIAS	EQU	(MSIZE-20)*1024	;offset from 20k system
CCP	EQU	3400H+BIAS	;base of the ccp
BIOS	EQU	CCP+1600H	;base of the bios
BIOSL	EQU	0300H		;length of the bios
BOOT	EQU	BIOS
SIZE	EQU	BIOS+BIOSL-CCP	;size of cp/m system
SECTS	EQU	SIZE/128	;# of sectors to load
;
;	I/O ports
;




	END			;of boot loader
