/**
 * SuperC SeqCmd Panpot Vibration
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum	$00
.ende


CmdPanVibration:
	call	readSeq
	mov	track.panVibRate+x, a
	call	readSeq
CmdPanVibrationOff:
	mov	track.panVibDepth+x, a
	mov	a, #0
	mov	track.panVibPhase+x, a
	ret


;------------------------------
; local values undefine
;------------------------------
;.undefine	XXXXX


