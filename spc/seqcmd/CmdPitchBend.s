/**
 * SuperC SeqCmd PitchBend
 *
 *
 */
;------------------------------
; local values
;------------------------------
.enum	$00
	lKey		db
.ende

CmdPitchBend:
	call	readSeq
	mov	track.pitchBendDelay+x, a
	call	readSeq
	mov	track.pitchBendSpan+x, a
	call	readSeq
	mov	track.pitchBendDiff+x, a
	mov	y, a
	bpl	+
	eor	a, #$ff
	inc	a
	mov	y, a

	; --- PitchBendをかけるticksを除数にセット
+	mov	a, track.pitchBendSpan+x
	dec	a

	call	DivDuration

	mov	track.pitchBendDelta+x, a
	mov	a, $00
	mov	track.pitchBendKey+x, a
	ret

;------------------------------
; local values undefine
;------------------------------
.undefine	lKey

