/**
 * SuperC SeqCmd Timpo
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum	$00
	XXXXX		db
.ende


CmdTempo:
	call	readSeq
	mov	musicTempo, a
	ret


;------------------------------
; local values undefine
;------------------------------
.undefine	XXXXX


