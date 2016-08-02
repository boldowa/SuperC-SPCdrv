/**
 * SuperC SeqCmd Jump
 *
 *
 */


CmdJump:
	call	readSeq
	push	a
	call	readSeq
	mov	y, a
	pop	a
	jmp	SetRelativePointer

