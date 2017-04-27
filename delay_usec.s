
/*
 * delay_usec.s
 *
 * Created: 1/22/2017 2:58:59 PM
 *  Author: Carter W, Drake S
 */ 
		.section .text
		.global	delay_usec

 delay_usec:		;4 cycles of RCALL
					;2 cycles for parameter => parameter is in r24 and r25
	ldi r18, 0x01	;set to one
	eor r19, r19	;zero out high bits
	cp	r18, r24
	cpc r19, r25	;Check if parameter is 1
	brne testzero	;two cycles if true, one if false
	ret				;If one, branch fails
	
testzero:			;at 12 cycles
	eor r18, r18	;zero low bits
	cp  r18, r24
	cpc r19, r25	
	breq zero		;at 16 cycles if fail, 17 if parameter is zero

loop:				;16 cycles have passed, parameter needs to be parameter-1
	sbiw r24, 0x01	;2 cycles, parameter-1
	nop
	nop
	nop
	ldi r18, 0x01	;set to one
	eor r19, r19	;zero out high bits
	cp	r18, r24
	cpc r19, r25
	breq one		;if one => 2 cycles, if greater than => 1 cycle
	nop
	nop
	nop
	nop
	rjmp loop

one:
	ret				;5 cycles
		
zero:				;17 cycles, need to burn 15, then jump to loop with MAX-1
	ldi	r24, 0xFF				
	ldi r25, 0xFF	;sets parameter to 65536-1
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	rjmp loop						;2 cycles