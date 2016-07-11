/**
 * SuperC SeqCmd Panpot
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum	$00
	XXXXX	db
.ende


CmdPanpot:
	call	readSeq
	mov	track.panH+x, a
	ret


;------------------------------
; local values undefine
;------------------------------
.undefine	XXXXX


