/**
 * SuperC SeqCmd SetInstrument
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum $00
	tmpFlg		db
.ende
;.enum $04
;			dw
;.ende

CmdSetInstrument:
	;--- read tone number
	call	readSeq

	;--- switch noise
	mov	y, a		; N flag check
	bmi	_Noise

	;--- calc table size
	mov	y, #_sizeof_stTone
	mul	ya

	;--- read tone settings
	mov	y, a
	mov	a, !Tone00.brrInx+y
	mov	track.brrInx+x, a
	mov	a, !Tone00.pitchRatio+y
	mov	track.pitchRatio+x, a
	mov	a, !Tone00.detune+y
	mov	track.detune+x, a
	mov	a, !Tone00.adr+y
	mov	track.adr+x, a
	mov	a, !Tone00.sr+y
	mov	track.sr+x, a
	mov	a, !Tone00.rr+y
	mov	track.rr+x, a
	ret
_Noise:
	;--- mask noise clock
	and	a, #$1f
	mov	tmpFlg, a
	;--- get current flag register
	mov	SPC_REGADDR, #DSP_FLG
	mov	a, SPC_REGDATA
	;--- apply noise clock
	and	a, #(FLG_RES|FLG_ECEN|FLG_MUTE)
	or	a, tmpFlg
	mov	SPC_REGDATA, a
	;--- set track noise flag
	mov	a, #TRKFLG_NOISE
	or	a, track.bitFlags+x
	mov	track.bitFlags+x, a
	ret

;------------------------------
; local values undefine
;------------------------------
.undefine	tmpFlg


