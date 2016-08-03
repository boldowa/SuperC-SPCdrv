/**
 * SuperC SeqCmd PitchModulation
 *
 *
 */


CmdPitchModulation:
	call	readSeq
	mov	track.modulationDelay+x, a
	call	readSeq
	mov	track.modulationRate+x, a
	mov	a, #0
	mov	track.PitchSpan+x, a
	call	readSeq
CmdPitchModulationOff:
	mov	track.modulationDepth+x, a
	ret

