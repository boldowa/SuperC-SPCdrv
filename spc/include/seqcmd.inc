/**
 * SuperC Commands define
 *
 *
 */

.ifndef _SEQCMD_INC
.define _SEQCMD_INC

;------------------------------------------------
; SEQ OCTAVE RANGE
;------------------------------------------------
.define OCTAVE_MIN	0
.define OCTAVE_MAX	6
.define OCTAVE_RANGE	(OCTAVE_MAX - OCTAVE_MIN + 1)

;------------------------------------------------
; TIE and REST
;------------------------------------------------
.define TIE		((12 * OCTAVE_RANGE) | $80)
.define REST		(TIE + 1)

;------------------------------------------------
; Drum note numbers
;------------------------------------------------
.define DRUM_NOTE	REST + 1
.define DRUM_NOTE_NUMS	7

;------------------------------------------------
; TONE Settings
;------------------------------------------------
.define EXTONE_START	$40

;------------------------------------------------
; Command Start Point
;------------------------------------------------
.define SEQCMD_START	(DRUM_NOTE + DRUM_NOTE_NUMS)

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
	CMD_TEMPO_FADE		db
	CMD_VOLUME_FADE		db
	CMD_GVOLUME_FADE	db
	CMD_TRANSPOSE		db
	CMD_REL_TRANSPOSE	db
	CMD_PAN_FADE		db
	CMD_PAN_VIBRATION	db
	CMD_PAN_VIBRATION_OFF	db
	CMD_HARDPM_ON		db
	CMD_HARDPM_OFF		db

	SEQCMDEND		db
.ende

.printt "SEQCMDEND: 0x"
.printv hex SEQCMDEND-1
.printt "\n"

.endif

