/**
 * SuperC SeqCmd Global Volume
 *
 *
 */


CmdGlobalVolume:
	call	readSeq
	mov	musicGlobalVolume, a
	mov	musicGlobalVolumeL, #0
	ret


