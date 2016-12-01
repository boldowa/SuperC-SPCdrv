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

.bank 2 slot 2
.orga __CODE_START__ - 34
	.db	"VER "
	.db	$00, $85
	.db	"DIR "
	.dw	DirTbl
	.db	"ESA "
	.dw	ESALoc
	.db	"TBL "
	.dw	TrackLocation
	.db	"LOC "
	.dw	__CODE_START__
	.db	"DATA"

.incdir  ""
.bank 3 slot 3
.org 0
.printt "CODE_START : 0x"
.printv HEX __CODE_START__
.printt "\n"
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
	bmi	_EndInitDSP
	mov	SPC_REGADDR,a
	inc	x
	mov	a,!TAB_DSP_INIT+x
	mov	SPC_REGDATA,a
	inc	x
	bra	-

_EndInitDSP:

; (テスト用)シーケンスデータの初期設定を行います
;.ifdef _MAKESPC
;	call	InitDebugSeqData
;.endif
	mov	a, !TrackLocation
	mov	seqBaseAddress, a
	mov	a, !TrackLocation+1
	mov	seqBaseAddress+1, a

	call	InitSequenceData

; タイマレジスタの初期設定を行います
+	mov	SPC_TIMER0,	#TIMER				; タイマー周期をセットします
	mov	a, #CNT_ST0		; タイマー0スタート
	mov	spcControlRegMirror, a
	or	a, #(CNT_PC10|CNT_PC32)	; SPCポートクリア
	mov	SPC_CONTROL, a

; エコーメモリの初期化待ちをします
	mov	SPC_REGADDR, #DSP_FLG
	mov	a, SPC_REGDATA
	or	a, #FLG_ECEN
	mov	SPC_REGDATA, a
	mov	a, #$81
	mov	y, #16
	call	Sleep
	mov	a, SPC_REGDATA
	and	a, #($ff~FLG_ECEN)
	mov	SPC_REGDATA, a
; EDL最大値を保持します
	mov	SPC_REGADDR, #DSP_EDL
	mov	edlMax, SPC_REGDATA

; SPCのwポートをクリアします
	mov	a, #0
	mov	x, #SPC_PORT0
	mov	(x)+, a			;\
	mov	(x)+, a			; | PortX-Wレジスタクリア
	mov	(x)+, a			; |
	mov	(x)+, a			;/

; カウンタレジスタをクリアします
	mov	a, SPC_COUNTER0

; --- メインのループ処理です
_mainLoop:
	call	IOCommProcess		; SNES<-->APU通信処理
--	mov	a, SPC_COUNTER0
	beq	_mainLoop

	mov	taskCounter, a		; 処理を回す回数としてカウンタ値を保持します

	mov1	c, rootSysFlags.0
	bcc	+			; DSPの内容を更新する必要がある場合のみ、DSPへの書き込みをします

	call	DspPreProcess		; 前処理
					; キーオン等を行います

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
	.db   DSP_MVOLL,  127
	.db   DSP_MVOLR,  127
	.db   DSP_EVOLL,   0
	.db   DSP_EVOLR,   0
	.db   DSP_KON,     0
	.db   DSP_KOF,     0
	.db   DSP_EON,     0
	.db   DSP_PMON,    0
	.db   DSP_NON,     0
	.db   DSP_DIR
DIRLoc:
	.db   (DirTbl>>8)
	.db   DSP_EDL      0
	.db   DSP_ESA
ESALoc:
	.db              ESA

	.db   $ff ; termination code
.ends

.ifdef _MAKESPC

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

.endif

/* table data include */
.incdir  "./table/"
.include "miscTable.tab"

