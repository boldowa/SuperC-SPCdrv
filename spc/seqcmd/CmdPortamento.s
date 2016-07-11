/**
 * SuperC SeqCmd Portamento
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum	$00
	XXXXX		db
.ende

CmdPortamentoOn:
	mov	a, #TRKFLG_PORTAM
	or	a, track.bitFlags+x
	mov	track.bitFlags+x, a
	ret

CmdPortamentoOff:
	mov	a, #(TRKFLG_PORTAM ~ $ff)
	and	a, track.bitFlags+x
	mov	track.bitFlags+x, a
	ret

;------------------------------
; local values undefine
;------------------------------
.undefine	XXXXX


