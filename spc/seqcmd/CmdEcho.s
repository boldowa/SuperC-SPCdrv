/**
 * SuperC SeqCmd Echo
 *
 *
 */


CmdEchoOff:
	clr1	tmpTrackSysBits.6
	ret

CmdEchoOn:
	set1	tmpTrackSysBits.6
	ret

CmdEchoParam:
	; Volume
	call	readSeq
	mov	eVolRatio, a
	; Delay
	call	readSeq
	mov	SPC_REGADDR, #$7d
	mov	SPC_REGDATA, a
	; Feed-back
	call	readSeq
	mov	SPC_REGADDR, #$0d
	mov	SPC_REGDATA, a
	ret


