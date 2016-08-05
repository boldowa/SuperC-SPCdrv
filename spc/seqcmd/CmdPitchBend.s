/**
 * SuperC SeqCmd PitchBend
 *
 *
 */
;------------------------------
; local values
;------------------------------
.enum	$00
	lTemp		db
	lKey		db
.ende

CmdPitchBend:
	call	readSeq
	mov	track.pitchBendDelay+x, a
	call	readSeq
	mov	track.pitchBendSpan+x, a
	call	readSeq
	mov	track.pitchBendDiff+x, a
	mov	lTemp, a
	mov	y, a
	bpl	+
	eor	a, #$ff
	inc	a
+	mov	y, a
	push	x
	mov	a, track.pitchBendSpan+x
	mov	x, a
	mov	a, y
	mov	y, #0
	div	ya, x
	mov	lKey, a
	mov	a, #0
	div	ya, x
	pop	x
	mov	track.pitchBendDelta+x, a
	mov	a, lKey
	mov	track.pitchBendKey+x, a
	ret

;------------------------------
; local values undefine
;------------------------------
.undefine	lTemp

