/**
 * SuperC SeqCmd Panpot
 *
 *
 */


CmdPanpot:
	call	readSeq
	mov	track.panH+x, a
CmdPanShare:
	mov	a, #0
	mov	track.panL+x, a
	ret


