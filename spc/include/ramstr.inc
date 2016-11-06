/**
 * SuperC N-Format RAM STRUCT DEFINES
 *
 *
 */

.ifndef _RAMSTR_INC
.define _RAMSTR_INC

;----------------------------------------
; DSPメモリ チャンネルデータ
;----------------------------------------
.struct stChannel
	lvol		db
	rvol		db
	pitch		dw
	srcn		db
	adr		db
	sr		db
	gain		db
	/**
	 * チャンネルに割り当てられているTrack番号
	 * 0xff(負数)で割り当てトラック無しとなる
	 */
	allocTrack	db
.endst

;----------------------------------------
; トラックデータ
;----------------------------------------
.struct stTrack
	seqPointerL	dsb	TRACKS
	seqPointerH	dsb	TRACKS
	trackSysBits	dsb	TRACKS
	; bit0     - is next note tie
	; bit1~3   - not use
	; bit4     - H/W modulation
	; bit5     - noise
	; bit6     - echo
	; bit7     - portament

	volFuncBits	dsb	TRACKS
	; bit0-3 : Tremolo bits
	;   bit0~1 - TremoloPhase bit9,bit10
	;   bit2~3 - none
	; bit4-7 : PanVibration bits
	;   bit4~5 - PanVibrationPhase bit9,bit10
	;   bit6~7 - none

	pitchFuncBits	dsb	TRACKS
	; bit0-3 : PitchModulaion bits
	;   bit0~1 - PitchModulationPhase bit9,bit10
	;   bit2~3 - none
	; bit4-7 : none

	brrInx		dsb	TRACKS		; 負数のときはノイズ
	pitchRatio	dsb	TRACKS
	detune		dsb	TRACKS
	adr		dsb	TRACKS		; GAINモードのとき、2バイト目はGAIN値になります
	sr		dsb	TRACKS		; GAINモードのとき、2バイト目はGAIN値になります
	rr		dsb	TRACKS		; releaseTimingで、SR値をこの値に書き換えます
	curKey		dsb	TRACKS
	transpose	dsb	TRACKS
	panL		dsb	TRACKS
	panH		dsb	TRACKS
	volumeL		dsb	TRACKS
	volumeH		dsb	TRACKS
	gate		dsb	TRACKS
	releaseTiming	dsb	TRACKS		; RRに移行するタイミング
	stepLeft	dsb	TRACKS		; 0になったときに次Noteを解析する
	step		dsb	TRACKS
	velocity	dsb	TRACKS
	tremoloDelay	dsb	TRACKS
	tremoloRate	dsb	TRACKS
	tremoloDepth	dsb	TRACKS
	tremoloPhase	dsb	TRACKS
	tremoloWaits	dsb	TRACKS
	modulationDelay	dsb	TRACKS
	modulationRate	dsb	TRACKS
	modulationDepth	dsb	TRACKS
	modulationPhase	dsb	TRACKS
	modulationWaits	dsb	TRACKS
	pitchBendDelay	dsb	TRACKS
	pitchBendSpan	dsb	TRACKS
	pitchBendDiff	dsb	TRACKS
	pitchBendWaits	dsb	TRACKS
	pitchBendDelta	dsb	TRACKS
	pitchBendPhase	dsb	TRACKS
	pitchBendKey	dsb	TRACKS
	volumeFadeSpan	dsb	TRACKS
	volumeFadeDtL	dsb	TRACKS
	volumeFadeDtH	dsb	TRACKS
	panFadeSpan	dsb	TRACKS
	panFadeDtL	dsb	TRACKS
	panFadeDtH	dsb	TRACKS
	panVibRate	dsb	TRACKS
	panVibDepth	dsb	TRACKS
	panVibPhase	dsb	TRACKS
.endst

;----------------------------------------
; 音色テーブル構成
;----------------------------------------
.struct stTone
	brrInx		db		; 波形番号
	pitchRatio	db		; ピッチ倍率
	detune		db		; デチューン
	adr		db		; Attack and Decay rate
	sr		db		; Sustain rate またはGAIN
	rr		db		; Release rate
.endst
;----------------------------------------
; ドラム用音色テーブル構成
;----------------------------------------
.struct stDrumTone
	brrInx		db		; 波形番号
	pitchRatio	db		; ピッチ倍率
	detune		db		; デチューン
	adr		db		; Attack and Decay rate
	sr		db		; Sustain rate
	rr		db		; Release rate
	panpot		db		; ドラムのパンポット
	note		db		; 鳴らす音階値
.endst


.endif

