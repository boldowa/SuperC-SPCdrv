/**
 * SuperC SeqCmd Volume
 *
 *
 */

;------------------------------
; local values
;------------------------------


CmdVolume:
	call	readSeq
	mov	track.volumeH+x, a
	ret


;------------------------------
; local values undefine
;------------------------------


