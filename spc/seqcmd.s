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
	.dw	CmdSetInstrument
	.dw	CmdVolume
	.dw	CmdPanpot
	.dw	CmdJump
	.dw	CmdTempo
	.dw	CmdGlobalVolume
	.dw	CmdEchoOn
	.dw	CmdEchoOff
	.dw	CmdEchoParam
	.dw	CmdSetFIR
	.dw	CmdPortamentoOn
	.dw	CmdPortamentoOff
	.dw	CmdPitchModulation
	.dw	CmdPitchModulationOff
	.dw	CmdTremolo
	.dw	CmdTremoloOff
	.dw	CmdSubroutine
	.dw	CmdSubroutineReturn
	.dw	CmdSubroutineBreak
	.dw	CmdPitchBend

CmdLengthTable:
	.db	2	; CmdSetInstrument
	.db	2	; CmdVolume
	.db	2	; CmdPanpot
	.db	3	; CmdJump
	.db	2	; CmdTempo
	.db	2	; CmdGlobalVolume
	.db	1	; CmdEchoOn
	.db	1	; CmdEchoOff
	.db	4	; CmdEchoParam
	.db	9	; CmdSetFIR
	.db	1	; CmdPortamentoOn
	.db	1	; CmdPortamentoOff
	.db	4	; CmdPitchModulation
	.db	1	; CmdPitchModulationOff
	.db	4	; CmdTremolo
	.db	1	; CmdTremoloOff
	.db	4	; CmdSubroutine
	.db	1	; CmdSubroutineReturn
	.db	1	; CmdSubroutineBreak
	.db	4	; CmdPitchBend

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

.ends

