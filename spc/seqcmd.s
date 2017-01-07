/**
 * SuperC Library codes
 *
 *
 */

.incdir  "./include/"
.include "spc.inc"
.include "var.inc"
.include "seqcmd.inc"


.incdir "./seqcmd/"

.section "SEQCMD" free

;--------------------------------------------------
; Command Jump Table
;--------------------------------------------------
CmdTable:
	.dw	CmdJump
	.dw	CmdSubroutine
	.dw	CmdSubroutineReturn
	.dw	CmdSubroutineBreak
	.dw	CmdSetInstrument
	.dw	CmdGlobalVolume
	.dw	CmdGlobalVolumeFade
	.dw	CmdVolume
	.dw	CmdVolumeFade
	.dw	CmdTremolo
	.dw	CmdPanpot
	.dw	CmdPanFade
	.dw	CmdPanVibration
	.dw	CmdTempo
	.dw	CmdTempoFade
	.dw	CmdTranspose
	.dw	CmdRelativeTranspose
	.dw	CmdPitchBend
	.dw	CmdPitchEnvelope
	.dw	CmdPitchModulation
	.dw	CmdEchoParam
	.dw	CmdSetFIR
	.dw	CmdEchoOn
	.dw	CmdEchoOff
	.dw	CmdPortamentoOn
	.dw	CmdPortamentoOff
	.dw	CmdCmdCall
	.dw	CmdCmdCall

CmdLengthTable:
	.db	2	; CmdJump
	.db	3	; CmdSubroutine
	.db	0	; CmdSubroutineReturn
	.db	0	; CmdSubroutineBreak
	.db	1	; CmdSetInstrument
	.db	1	; CmdGlobalVolume
	.db	2	; CmdGlobalVolumeFade
	.db	1	; CmdVolume
	.db	2	; CmdVolumeFade
	.db	3	; CmdTremolo
	.db	1	; CmdPanpot
	.db	2	; CmdPanFade
	.db	2	; CmdPanVibration
	.db	1	; CmdTempo
	.db	2	; CmdTempoFade
	.db	1	; CmdTranspose
	.db	1	; CmdRelativeTranspose
	.db	3	; CmdPitchBend
	.db	3	; CmdPitchEnvelope
	.db	3	; CmdPitchModulation
	.db	3	; CmdEchoParam
	.db	8	; CmdSetFIR
	.db	0	; CmdEchoOn
	.db	0	; CmdEchoOff
	.db	0	; CmdPortamentoOn
	.db	0	; CmdPortamentoOff
	.db	0	; CmdCmdCall(Arg:0)
	.db	1	; CmdCmdCall(Arg:1)

;--------------------------------------------------
; Sub Command
;--------------------------------------------------

CmdCmdCall:
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

	.dw	CmdPitchModulationOff		; arg: 0
	.dw	CmdTremoloOff			; arg: 0
	.dw	CmdPanVibrationOff		; arg: 0
	.dw	CmdHWPitchModulationOn		; arg: 0
	.dw	CmdHWPitchModulationOff		; arg: 0
	.dw	CmdPitchEnvelopeOff		; arg: 0
	.dw	CmdNotKeyOffOn			; arg: 0
	.dw	CmdNotKeyOffOff			; arg: 0

	.dw	CmdSetAR			; arg: 1
	.dw	CmdSetDR			; arg: 1
	.dw	CmdSetSL			; arg: 1
	.dw	CmdSetSR			; arg: 1
	.dw	CmdSetRR			; arg: 1
	.dw	CmdSetGain1			; arg: 1
	.dw	CmdSetGain2			; arg: 1
	.dw	CmdSpwavFreq			; arg: 1


;--------------------------------------------------
; Command Include
;--------------------------------------------------
.include	"CmdSetInstrument.s"
.include	"CmdVolume.s"
.include	"CmdPanpot.s"
.include	"CmdJump.s"
.include	"CmdTempo.s"
.include	"CmdGlobalVolume.s"
.include	"CmdEcho.s"
.include	"CmdPortamento.s"
.include	"CmdPitchModulation.s"
.include	"CmdTremolo.s"
.include	"CmdSubroutine.s"
.include	"CmdPitchBend.s"
.include	"CmdTempoFade.s"
.include	"CmdVolumeFade.s"
.include	"CmdGlobalVolumeFade.s"
.include	"CmdTranspose.s"
.include	"CmdPanFade.s"
.include	"CmdPanVibration.s"
.include	"CmdHWPitchModulation.s"
.include	"CmdPitchEnvelope.s"
.include	"CmdNotKeyOff.s"
.include	"CmdAdsr.s"
.include	"CmdSpwavFreq.s"

.ends

.export CMD_PITCH_ENVELOPE

