/**
 * SuperC Timer Function
 *
 *
 */

.section "SLEEP" free

;------------------------------
; Sleep
;   a   : timer (125us * n)
;   y   : wait counter
;
;  e.g.  a=0x80,y=0x02 -> 32ms
;------------------------------
Sleep:
	;--- Setup Timer
	mov	SPC_TIMER1, a
	set1	spcControlRegMirror.1
	mov	SPC_CONTROL, spcControlRegMirror

	;--- Wait Timer
-	mov	a, SPC_COUNTER1
	beq	-
	dbnz	y,-

	;--- Stop Timer
	clr1	spcControlRegMirror.1
	mov	SPC_CONTROL, spcControlRegMirror

	ret

.ends


