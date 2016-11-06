/**
 * SuperC SeqCmd Hard-ware Pitch Modulation
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum	$00
.ende


CmdHWPitchModulationOn:
	set1	tmpTrackSysBits.4
	ret

CmdHWPitchModulationOff:
	clr1	tmpTrackSysBits.4
	ret


;------------------------------
; local values undefine
;------------------------------
;.undefine	XXXXX


