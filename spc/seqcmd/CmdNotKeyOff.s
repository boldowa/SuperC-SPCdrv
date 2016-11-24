/**
 * SuperC SeqCmd NOT-KeyOFF
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum	$00
.ende


CmdNotKeyOffOn:
	set1	tmpTrackSysBits.3
	ret

CmdNotKeyOffOff:
	clr1	tmpTrackSysBits.3
	ret


;------------------------------
; local values undefine
;------------------------------


