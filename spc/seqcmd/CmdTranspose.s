/**
 * SuperC SeqCmd Transpose
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum	$00
.ende


CmdTranspose:
	call	readSeq
-	mov	track.transpose+x, a
	ret

CmdRelativeTranspose:
	call	readSeq
	clrc
	adc	a, track.transpose+x
	bra	-


;------------------------------
; local values undefine
;------------------------------
;.undefine	XXXXX


