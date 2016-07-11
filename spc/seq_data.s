/**
 * SuperC test seq data
 *
 *
 */

.incdir  "./include/"
.include "spc.inc"
.include "var.inc"
.include "seqcmd.inc"

.orga $2000
.section "SEQ_DATA" force

TrackHeader:
	.dw	_Track1
	.dw	_Track2
	.dw	_Track3
	.dw	_Track4
	.dw	_Track5
	.dw	_Track6
	.dw	_Track7
	.dw	_Track8
	.dw	_Track9
	.dw	_Track10

_Track1:
	.db	$60
	.db	CMD_GVOLUME, $60
	.db	CMD_ECHO_FIR, $7f, $00, $00, $00, $00, $00, $00, $00
	.db	CMD_ECHO_PARAM, $c0, $04, $40
_T1Lp:
	.db	CMD_TEMPO, 45
	.db	$c9, $c9
	.db	CMD_TEMPO, 80
	.db	$c9, $c9
	.db	CMD_TEMPO, 160
	.db	$c9, $c9
	.db	CMD_TEMPO, 255
	.db	$c9, $c9
	.db	CMD_TEMPO, 20
	.db	$c9, $c9
	.db	CMD_JUMP
	.dw	(_T1Lp-_T1End)
_T1End:

_Track5:
	.db	CMD_SET_INST, $00
	.db	CMD_VOLUME, $40
	.db	CMD_PAN, $20
	.db	$30, $7f
	.db	$bc, $bb, $bc
	.db	$00

_Track6:
	.db	CMD_SET_INST, $9f
	.db	CMD_VOLUME, $80
	.db	CMD_PAN, $20
	.db	$30, $7f
	.db	$c5, $c5, $c5
	.db	$00

_Track2:
	.db	CMD_VOLUME, $ff
	.db	CMD_SET_INST, $04
_Join:
	.db	CMD_ECHO_ON
	.db	CMD_PORTAM_ON
	.db	CMD_PAN, $10
	.db	$18 $3f
	.db	$b0, $b2
	.db	CMD_PAN, $60
	.db	$b4, $b2
	.db	CMD_PAN, $b0
	.db	CMD_PORTAM_OFF
	.db	$b0, $bc
	.db	CMD_PAN, $e0
	.db	$b0, $c9
	.db	CMD_JUMP
	.dw	(_Join-_Track2End)
_Track2End:

_Track3:
	.db	$0c
	.db	$c9
	.db	CMD_SET_INST, $04
	.db	CMD_VOLUME, $80
	.db	CMD_JUMP
	.dw	(_Join-_Track3End)
_Track3End:

_Track4:
	.db	$18
	.db	$c9
	.db	CMD_SET_INST, $04
	.db	CMD_VOLUME, $40
	.db	CMD_JUMP
	.dw	(_Join-_Track4End)
_Track4End:

_Track7:
	.db	$24
	.db	$c9
	.db	CMD_SET_INST, $04
	.db	CMD_VOLUME, $20
	.db	CMD_JUMP
	.dw	(_Join-_Track7End)
_Track7End:

_Track8:
	.db	$30
	.db	$c9
	.db	CMD_SET_INST, $04
	.db	CMD_VOLUME, $10
	.db	CMD_JUMP
	.dw	(_Join-_Track8End)
_Track8End:

_Track9:
	.db	CMD_SET_INST, $02
	.db	CMD_VOLUME, $ff
	.db	CMD_PAN, $20
	.db	$30, $7b
	.db	$b8
	.db	CMD_SET_INST, $03
	.db	CMD_ECHO_ON
	.db	$30, $7f
	.db	$b8
	.db	CMD_SET_INST, $02
	.db	CMD_ECHO_OFF
	.db	$18, $7b
	.db	$b8, $b8
	.db	CMD_SET_INST, $03
	.db	CMD_ECHO_ON
	.db	$18, $7f
	.db	$b8
	.db	CMD_SET_INST, $02
	.db	CMD_ECHO_OFF
	.db	$06, $7c
	.db	$b8
	.db	$06, $77
	.db	$b8, $b8, $b8
	.db	CMD_JUMP
	.dw	(_Track9-_Track9End)
_Track9End:

_Track10:
	.db	$00

_Track13:	; ƒ|ƒeƒg
	.db	CMD_SET_INST, $00
	.db	CMD_VOLUME, $70
	.db	CMD_PAN, $20
	.db	$18, $6f
	.db	$ab, $a9, $ab, $c9
	.db	CMD_JUMP
	.dw	(_Track13-_Track13End)
_Track13End:

.ends
