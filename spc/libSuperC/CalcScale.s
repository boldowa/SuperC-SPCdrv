/**
 * SuperC Calculate Scale
 *
 *
 */

.section "CALC_SCALE" free
;------------------------------
; local values
;------------------------------
.define		lPitchH		$00

CalcScale:
	mov	y, #0
	asl	a			; テーブルは2ケタ値なので
	mov	x, #24
	div	ya, x
	mov	x, a
	mov	a, !_PitchTable+1+y
	mov	lPitchH, a
	mov	a, !_PitchTable+y
	bra	+
-	inc	x
	lsr	lPitchH
	ror	a
+	cmp	x, #5
	bmi	-
	mov	y, lPitchH
	ret

_PitchTable:
	.dw	$10be			; c
	.dw	$11bd			; c+
	.dw	$12cb			; d
	.dw	$13e9			; d+
	.dw	$1518			; e
	.dw	$1659			; f
	.dw	$17ad			; f+
	.dw	$1916			; g
	.dw	$1a94			; g+
	.dw	$1c28			; a
	.dw	$1dd5			; a+
	.dw	$1f9b			; b

;------------------------------
; local values undefine
;------------------------------
.undefine	lPitchH

.ends

