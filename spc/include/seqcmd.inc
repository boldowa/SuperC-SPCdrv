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
	CMD_TREMOLO		db
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
	CMD_PITCH_ENVELOPE	db
	CMD_CMD_ARG0		db

	SEQCMD_END		db
.ende

;------------------------------------------------
; Sub-Commands
;------------------------------------------------
.enum 0
	SCMD_MODURATION_OFF		db
	SCMD_TREMOLO_OFF		db
	SCMD_PAN_VIBRATION_OFF		db
	SCMD_HWPM_ON			db
	SCMD_HWPM_OFF			db
	SCMD_PITCH_ENVELOPE_OFF		db
	SCMD_NOT_KEYOFF_ON		db
	SCMD_NOT_KEYOFF_OFF		db
.ende

.printt "SEQCMD_START: 0x"
.printv hex SEQCMD_START
.printt "\n"
.printt "SEQCMD_END: 0x"
.printv hex SEQCMD_END-1
.printt "\n"

.endif

