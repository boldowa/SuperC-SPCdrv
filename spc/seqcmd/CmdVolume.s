/**
 * SuperC SeqCmd Volume
 *
 *
 */


CmdVolume:
	call	readSeq
	mov	track.volumeH+x, a
	ret



