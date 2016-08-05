/**
 * SuperC Main program
 *
 *
 */

.incdir  "./include/"
.include "spc.inc"
.include "var.inc"

.incdir  "./table/"
.include "id666.tab"
.include "dspreg.tab"


.incdir  ""
.bank 3 slot 3
.org 0
.section "__main__" force
;--------------------------------------------------
; Start - SPC PROGRAM ROOT
;   Level       : - (root Level)
;   Input       : none
;   Output      : none
;   Description : rootプログラム
;--------------------------------------------------
Start:
	di
	mov	a,#0
	mov	x,a
	mov	$00,a

; $0200～ のRAM領域をクリアします
	mov	y, #((__CODE_START__-1)&$ff)
	mov	$01, #((__CODE_START__-1)>>8)
	bra     _f

-	dec	$01
__	mov	[$00]+y, a
	dbnz	y, _b
	cmp	$01, #$02		; $0200
	bne	-
	mov	[$00]+y, a

; ダイレクトページを初期化します
; --- Page1
	setp
-	mov	(x)+,a
	cmp	x,#0
	bne	-
; --- Page0
	clrp
-	mov	(x)+,a
	cmp	x,#$f0
	bcc	-

; スタックポインタを初期化します
	mov	x,#$ff
	mov	sp,x  ; スタックポインタ初期化

; DSPレジスタを初期化します
	mov	x,a
-	mov	a, !TAB_DSP_INIT+x
	bmi	+
	mov	SPC_REGADDR,a
	inc	x
	mov	a,!TAB_DSP_INIT+x
	mov	SPC_REGDATA,a
	inc	x
	bra	-

; (テスト用)シーケンスデータの初期設定を行います
+	call	InitDebugSeqData

; タイマレジスタの初期設定を行います
	mov	a, SPC_COUNTER0					; カウンタレジスタをクリアします
+	mov	SPC_TIMER0,	#TIMER				; タイマー周期をセットします
	mov	musicTempo,	#MUSIC_TEMPO_DEFAULT		; 音楽テンポの初期値をセットします
	mov	SPC_CONTROL,	#(CNT_ST0|CNT_PC10|CNT_PC32)	; タイマー0スタート / SPCポートクリア

	mov	a, #$ff			;\  カウンタ値をMAX値にします
	mov	sndTempoCounter, a	; | こうすることで、テンポが0以外なら
	mov	musicTempoCounter, a	;/  初回のtick動作で必ず解析処理が実行されます

; 特殊波形用の周波数値に初期値を入れます
	mov	specialWavFreq, #SPECIAL_WAV_FREQ

; --- メインのループ処理です
_mainLoop:
	call	IOCommProcess		; SNES<-->APU通信処理
--	mov	a, SPC_COUNTER0
	beq	_mainLoop

	mov	taskCounter, a		; 処理を回す回数としてカウンタ値を保持します

	mov1	c, rootSysFlags.0
	bcc	+

	call	DspPreProcess		; 前処理
					; キーオン等を行います

;	call	DspReleaseMuteChannel	; 音の鳴っていないチャンネルをあらかじめ開放します

+	clr1	rootSysFlags.0		; DSP更新が終わったので、フラグクリア
	bra	_Sound			; 効果音処理を開始します

;----- タスクループ - ここから --------------------

	; タスクが増えた際にIOレスポンスが悪くなることを防ぐため、
	; タスク実行毎にIOポートを確認します
-	call	IOCommProcess		; SNES<-->APU通信処理
	bne	--			; 要リセットの場合、タスクを中断
_Sound:
	mov	a, #SND_TEMPO
	clrc
	adc	a, sndTempoCounter
	mov	sndTempoCounter, a
	bcc	_Music

	set1	rootSysFlags.1		; 効果音解析中状態とします
	call	SoundEffectProcess	; 効果音処理を行います
	set1	rootSysFlags.0		; DSP更新を予約します

_Music:
	mov	a, musicTempo
	clrc
	adc	a, musicTempoCounter
	mov	musicTempoCounter, a
	bcc	_SpecialWav

	clr1	rootSysFlags.1		; 効果音解析中状態を解除します
	call	MusicProcess		; 音楽処理を行います
	set1	rootSysFlags.0		; DSP更新を予約します
	
_SpecialWav:				; 特殊波形の処理
	mov	a, specialWavFreq
	clrc
	adc	a, specialWavFreqCounter
	mov	specialWavFreqCounter, a
	bcc	_End
	call	SpecialWavFunc

_End:
	dbnz	taskCounter, -

;----- タスクループ - ここまで --------------------

	mov1	c, rootSysFlags.0
	bcc	_mainLoop		; トラック・効果音処理なしの場合はループ

	call	DspPostProcess		; 後処理
					; 実は不要になるかもしれません。

	bra   _mainLoop			; ループさせます。
.ends


.section "initData" free

TAB_DSP_INIT:
	.db   DSP_FLG,    (FLG_ECEN|FLG_MUTE|FLG_RES)
	.db   DSP_MVOLL,  64
	.db   DSP_MVOLR,  64
	.db   DSP_EVOLL,   0
	.db   DSP_EVOLR,   0
	.db   DSP_KON,     0
	.db   DSP_KOF,     0
	.db   DSP_EON,     0
	.db   DSP_PMON,    0
	.db   DSP_NON,     0
	.db   DSP_DIR,   (DirTbl>>8)
	.db   DSP_EDL      0
	.db   DSP_ESA    ESA

	.db   $ff ; termination code
.ends

.section "DebugCode" free
InitDebugSeqData:
	mov	a, #0
	mov	y, a
	mov	x, a
	; シーケンスのベース位置セット
	mov     seqBaseAddress, #(TrackHeader & $ff)
	mov     seqBaseAddress+1, #(TrackHeader >> 8)
	; シーケンスアドレスの設定
-	mov	a, [seqBaseAddress]+y
	mov     $00, a
	inc	y
	mov	a, [seqBaseAddress]+y
	mov     $01, a
	inc	y
	push    y
	movw    ya, $00
	beq     +				; 空トラック
	clrc
	addw    ya, seqBaseAddress
	mov	track.seqPointerL+x, a
	mov     a, y
	mov	track.seqPointerH+x, a
+	pop     y
	inc	x
	cmp	y, #(MUSICTRACKS*2)
	bmi	-

	; トラックデータの初期化
	mov	x, #0
	mov	y, #0
-	mov	a, #$ff
	mov	buf_chData.allocTrack+x, a
	mov	a, #$ff
	mov	buf_chData.adr+x, a
	mov	a, #$f0
	mov	buf_chData.sr+x, a
	mov	a, x
	clrc
	adc	a, #_sizeof_stChannel
	mov	x, a
	inc	y
	cmp	y, #8
	bmi	-

	mov	SPC_REGADDR, #DSP_FLG
	mov	SPC_REGDATA, #0
	mov	musicGlobalVolume, #$ff
;	setc
;	mov1	musicSysFlags.2, c		; モノラル
	mov	seChNums, #3
	ret
.ends

/* table data include */
.incdir  "./table/"
.include "miscTable.tab"

