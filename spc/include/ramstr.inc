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
	bitFlags	dsb	TRACKS
	; bit0   - is next note tie
	; bit1~4 - not use
	; bit5   - noise
	; bit6   - echo
	; bit7   - portament

	brrInx		dsb	TRACKS		; 負数のときはノイズ
	pitchRatio	dsb	TRACKS
	curKey		dsb	TRACKS
	gate		dsb	TRACKS
	releaseTiming	dsb	TRACKS		; RRに移行するタイミング
	stepLeft	dsb	TRACKS		; 0になったときに次Noteを解析する
	step		dsb	TRACKS
	velocity	dsb	TRACKS
	volumeL		dsb	TRACKS
	volumeH		dsb	TRACKS
	panL		dsb	TRACKS
	panH		dsb	TRACKS
	tremoloDelay	dsb	TRACKS
	tremoloRate	dsb	TRACKS
	tremoloDepth	dsb	TRACKS
	modulationDelay	dsb	TRACKS
	modulationRate	dsb	TRACKS
	modulationDepth	dsb	TRACKS
	PitchSpan	dsb	TRACKS
	PitchCounter	dsb	TRACKS
	PitchPhase	dsb	TRACKS
	adr		dsb	TRACKS		; GAINモードのとき、2バイト目はGAIN値になります
	sr		dsb	TRACKS		; GAINモードのとき、2バイト目はGAIN値になります
	rr		dsb	TRACKS		; releaseTimingで、SR値をこの値に書き換えます
.endst

;----------------------------------------
; 音色テーブル構成
;----------------------------------------
.struct stTone
	brrInx		db		; 波形番号
	pitchRatio	db		; ピッチ倍率
	adr		db		; Attack and Decay rate
	sr		db		; Sustain rate
	rr		db		; Release rate
.endst


.endif

