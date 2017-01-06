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
	call	readSeq
CmdPitchEnvelopeOff:
	mov	track.pitchEnvDiff+x, a
	ret


;------------------------------
; local values undefine
;------------------------------
;.undefine	XXXXX


