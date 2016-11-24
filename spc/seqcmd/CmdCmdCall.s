/**
 * SuperC SeqCmd Sub-command call
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum	$00
.ende


CmdCmdCallArg0:
	call	readSeq
	asl	a
	mov	y, a
	mov	a, !_SubCmdTable+1+y
	mov	!_jmpTgt+2, a
	mov	a, !_SubCmdTable+y
	mov	!_jmpTgt+1, a
	mov	a, #0
_jmpTgt:
	jmp	$0000

_SubCmdTable:
	.dw	CmdPitchModulationOff
	.dw	CmdTremoloOff
	.dw	CmdPanVibrationOff
	.dw	CmdHWPitchModulationOn
	.dw	CmdHWPitchModulationOff
	.dw	CmdPitchEnvelopeOff
	.dw	CmdNotKeyOffOn
	.dw	CmdNotKeyOffOff

;------------------------------
; local values undefine
;------------------------------


