/**
 * SuperC Calculate Scale
 *
 *
 */

.section "CALC_SCALE" free
;------------------------------
; Scale value Define
;------------------------------
.define		ScaleC		$10be	; c
.define		ScaleCs		$11bd	; c+
.define		ScaleD		$12cb	; d
.define		ScaleDs		$13e9	; d+
.define		ScaleE		$1518	; e
.define		ScaleF		$1659	; f
.define		ScaleFs		$17ad	; f+
.define		ScaleG		$1916	; g
.define		ScaleGs		$1a94	; g+
.define		ScaleA		$1c28	; a
.define		ScaleAs		$1dd5	; a+
.define		ScaleB		$1f9b	; b

;------------------------------
; local values
;------------------------------
.define		lPitchH		$00
.define		lPitchTbl	$01

CalcScale:
	mov	lPitchTbl, #(_PitchTable & $ff)
	mov	lPitchTbl+1, #(_PitchTable >> 8)
_ScaleJoin:
	mov	y, #0
	asl	a			; テーブルは2ケタ値なので
	mov	x, #24
	div	ya, x
	mov	x, a
	inc	y
	mov	a, [lPitchTbl]+y
	mov	lPitchH, a
	dec	y
	mov	a, [lPitchTbl]+y
	bra	+
-	inc	x
	lsr	lPitchH
	ror	a
+	cmp	x, #(OCTAVE_RANGE - 1)
	bmi	-
	mov	y, lPitchH
	ret

CalcScaleDiff:
	mov	lPitchTbl, #(_PitchDiffTable & $ff)
	mov	lPitchTbl+1, #(_PitchDiffTable >> 8)
	bra	_ScaleJoin

_PitchTable:
	.dw	ScaleC			; c
	.dw	ScaleCs			; c+
	.dw	ScaleD			; d
	.dw	ScaleDs			; d+
	.dw	ScaleE			; e
	.dw	ScaleF			; f
	.dw	ScaleFs			; f+
	.dw	ScaleG			; g
	.dw	ScaleGs			; g+
	.dw	ScaleA			; a
	.dw	ScaleAs			; a+
	.dw	ScaleB			; b

_PitchDiffTable:
	.dw	ScaleCs - ScaleC	; c
	.dw	ScaleD - ScaleCs	; c+
	.dw	ScaleDs - ScaleD	; d
	.dw	ScaleE - ScaleDs	; d+
	.dw	ScaleF - ScaleE		; e
	.dw	ScaleFs - ScaleF	; f
	.dw	ScaleG - ScaleFs	; f+
	.dw	ScaleGs - ScaleG	; g
	.dw	ScaleA - ScaleGs	; g+
	.dw	ScaleAs - ScaleA	; a
	.dw	ScaleB - ScaleAs	; a+
	.dw	(ScaleC*2) - ScaleB	; b

;------------------------------
; local values undefine
;------------------------------
.undefine	lPitchH
.undefine	lPitchTbl

.ends

