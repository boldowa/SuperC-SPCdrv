/**
 * SuperC Commands define
 *
 *
 */

.ifndef _SEQCMD_INC
.define _SEQCMD_INC

;------------------------------------------------
; Command Start Point
;------------------------------------------------
.define SEQCMD_START	$d1

;------------------------------------------------
; Commands
;------------------------------------------------
.enum SEQCMD_START
	CMD_SET_INST		db
	CMD_VOLUME		db
	CMD_PAN			db
	CMD_JUMP		db
	CMD_TEMPO		db
	CMD_GVOLUME		db
	CMD_ECHO_ON		db
	CMD_ECHO_OFF		db
	CMD_ECHO_PARAM		db
	CMD_ECHO_FIR		db
	CMD_PORTAM_ON		db
	CMD_PORTAM_OFF		db
	CMD_MODURATION		db
	CMD_MODURATION_OFF	db
	CMD_TREMOLO		db
	CMD_TREMOLO_OFF		db
	CMD_SUBROUTINE		db
	CMD_SUBROUTINE_RETURN	db
	CMD_SUBROUTINE_BREAK	db
	CMD_PITCHBEND		db
.ende


.endif

