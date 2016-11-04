/**
 * SuperC SeqCmd Volume
 *
 *
 */


CmdVolume:
	call	readSeq
	mov	track.volumeH+x, a
CmdVolShare:
	mov	a, #0
	mov	track.volumeL+x, a
	ret



