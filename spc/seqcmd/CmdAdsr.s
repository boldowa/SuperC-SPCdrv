/**
 * SuperC SeqCmd ADSR Change
 *
 *
 */

;------------------------------
; local values
;------------------------------
.define tmpADSR 0

CmdSetAR:
	mov	a, track.adr+x
	and	a, #$f0
	mov	tmpADSR, a
	call	readSeq
	and	a, #$0f
	or	a, tmpADSR
	mov	track.adr+x, a
	ret

CmdSetDR:
	mov	a, track.adr+x
	and	a, #$8f
	mov	tmpADSR, a
	call	readSeq
	and	a, #$07
	xcn	a
	or	a, tmpADSR
	mov	track.adr+x, a
	ret

CmdSetSL:
	mov	a, track.sr+x
	and	a, #$1f
	mov	tmpADSR, a
	call	readSeq
	and	a, #$07
	xcn	a
	asl	a
	or	a, tmpADSR
	mov	track.sr+x, a
	ret

CmdSetSR:
	mov	a, track.sr+x
	and	a, #$e0
	mov	tmpADSR, a
	call	readSeq
	and	a, #$1f
	or	a, tmpADSR
	mov	track.sr+x, a
	ret

CmdSetRR:
	mov	a, track.rr+x
	and	a, #$e0
	mov	tmpADSR, a
	call	readSeq
	and	a, #$1f
	or	a, tmpADSR
	mov	track.rr+x, a
	ret

CmdSetGain1:
	call	readSeq
	mov	track.sr+x, a
	ret

CmdSetGain2:
	call	readSeq
	mov	track.rr+x, a
	ret


;------------------------------
; local values undefine
;------------------------------
.undefine tmpADSR


