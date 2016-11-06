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
	toneTblPtr	dw
.ende
;.enum $04
;			dw
;.ende

CmdSetInstrument:
	;--- read tone number
	call	readSeq

SetInstrumentA:
	cmp	a, #EXTONE_START
	bmi	+
	sbc	a, #EXTONE_START
	push	a
	movw	ya, exToneTablePtr
	movw	toneTblPtr, ya
	pop	a
	bra	++

	;--- calc table size
+	mov	toneTblPtr, #(ToneTable & $ff)
	mov	toneTblPtr+1, #(ToneTable >> 8)
++	mov	y, #_sizeof_stTone

GetToneCfg:
	mul	ya
	addw	ya, toneTblPtr
	movw	toneTblPtr, ya
	mov	y, #0
	mov	a, [toneTblPtr]+y
	bpl	_NoNoise

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
	set1	tmpTrackSysBits.5	; bit5: noise
	;--- BRR num = 0
	mov	a, y
	bra	+

_NoNoise:
	;--- clear noise flag
	clr1	tmpTrackSysBits.5

	;--- read tone settings
	mov	a, [toneTblPtr]+y
+	mov	track.brrInx+x, a
	inc	y
	mov	a, [toneTblPtr]+y
	mov	track.pitchRatio+x, a
	inc	y
	mov	a, [toneTblPtr]+y
	mov	track.detune+x, a
	inc	y
	mov	a, [toneTblPtr]+y
	mov	track.adr+x, a
	inc	y
	mov	a, [toneTblPtr]+y
	mov	track.sr+x, a
	inc	y
	mov	a, [toneTblPtr]+y
	mov	track.rr+x, a
	ret

;------------------------------
; local values undefine
;------------------------------
.undefine	tmpFlg
.undefine	toneTblPtr


