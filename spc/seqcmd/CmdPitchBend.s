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
	mov	track.pitchBendPhase+x, a
	call	readSeq
	mov	track.pitchBendDelay+x, a
	call	readSeq
	mov	track.pitchBendSpan+x, a
	call	readSeq
	mov	track.pitchBendDiff+x, a
PitchEnvShare:
	mov	y, a
	bpl	+
	eor	a, #$ff
	inc	a
	mov	y, a

	; --- PitchBend‚ğ‚©‚¯‚éticks‚ğœ”‚ÉƒZƒbƒg
+	mov	a, track.pitchBendSpan+x

	call	DivDuration

	mov	track.pitchBendDelta+x, a
	mov	a, $00
	mov	track.pitchBendKey+x, a
	ret

;------------------------------
; local values undefine
;------------------------------
.undefine	lKey

