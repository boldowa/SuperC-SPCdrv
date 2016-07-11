/**
 * SuperC SeqCmd Echo
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum	$00
	XXXXX		db
.ende

CmdEchoOff:
	mov	a, #(TRKFLG_ECHO ~ $ff)
	and	a, track.bitFlags+x
	mov	track.bitFlags+x, a
	ret

CmdEchoOn:
	mov	a, #TRKFLG_ECHO
	or	a, track.bitFlags+x
	mov	track.bitFlags+x, a
	ret

CmdEchoParam:
	; Volume
	call	readSeq
	mov	eVolRatio, a

	; Delay
	call	readSeq
	mov	SPC_REGADDR, #$7d
	mov	SPC_REGDATA, a

	; Feed-back
	call	readSeq
	mov	SPC_REGADDR, #$0d
	mov	SPC_REGDATA, a
	ret

;------------------------------
; local values undefine
;------------------------------
.undefine	XXXXX


