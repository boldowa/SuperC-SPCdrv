/**
 * SuperC SeqCmd Tremolo
 *
 *
 */

CmdTremolo:
	call	readSeq
	mov	track.tremoloDelay+x, a
	call	readSeq
	mov	track.tremoloRate+x, a
	call	readSeq
CmdTremoloOff:
	mov	track.tremoloDepth+x, a
	ret

