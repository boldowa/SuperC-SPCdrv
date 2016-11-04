/**
 * SuperC SeqCmd Timpo
 *
 *
 */

CmdTempo:
	call	readSeq
	mov	musicTempo, a
	mov	a, #0
	mov	musicTempoLo, a
	mov	tempoFadeSpan, a
	ret



