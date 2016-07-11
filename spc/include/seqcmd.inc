/**
 * SuperC
 *
 *
 */

.ifndef _SEQCMD_INC
.define _SEQCMD_INC

;------------------------------------------------
; Command Start Point
;------------------------------------------------
.define SEQCMD_START	$da

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
.ende


.endif

