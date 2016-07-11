/**
 * SuperC SeqCmd Global Volume
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum	$00
	XXXXX		db
.ende


CmdGlobalVolume:
	call	readSeq
	mov	musicGlobalVolume, a
	ret


;------------------------------
; local values undefine
;------------------------------
.undefine	XXXXX


