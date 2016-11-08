/**
 * SuperC SeqCmd PitchEnvelope
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum	$00
;	XXXXX		db
.ende

CmdPitchEnvelope:
	call	readSeq
	mov	track.pitchEnvDelay+x, a
	call	readSeq
	mov	track.pitchEnvSpan+x, a
	mov	track.pitchBendSpan+x, a
	call	readSeq
CmdPitchEnvelopeOff:
	mov	track.pitchEnvDiff+x, a
	eor	a, #$ff
	inc	a
	mov	track.pitchBendDiff+x, a
	jmp	PitchEnvShare


;------------------------------
; local values undefine
;------------------------------
;.undefine	XXXXX


