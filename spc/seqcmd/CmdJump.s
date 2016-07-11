/**
 * SuperC SeqCmd Jump
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum	$00
	XXXXX		db
.ende


CmdJump:
	call	readSeq
	push	a
	call	readSeq
	mov	y, a
	pop	a
	call	SetRelativePointer
	ret


;------------------------------
; local values undefine
;------------------------------
.undefine	XXXXX


