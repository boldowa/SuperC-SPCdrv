/**
 * SuperC SeqCmd Tremolo
 *
 *
 */

/* TODO: メモリ定義と実装の書き換え */

CmdTremolo:
	call	readSeq
	mov	track.modulationDelay+x, a
	call	readSeq
	mov	track.modulationRate+x, a
	mov	a, #0
	mov	track.PitchSpan+x, a
	call	readSeq
CmdTremoloOff:
	mov	track.modulationDepth+x, a
	ret

