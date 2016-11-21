/**
 * SuperC Music process
 *
 *
 */

.incdir  "./include/"
.include "spc.inc"
.include "var.inc"
.include "seqcmd.inc"

.section "__music__" free
;--------------------------------------------------
; MusicProcess - MUSIC CONTROLL PROCESS
;   Level       : 1 (Main Level)
;   Input       : none
;   Output      : none
;   Description : 音楽制御プログラム
;                 トラック毎のシーケンスを解釈し
;                 演奏メモリを更新する
;--------------------------------------------------

;------------------------------
; local values
;------------------------------
.define		lSeqPointer	$08

MusicProcess:
	mov	x, #0

_TrackLoop
	call	LoadTrackBitMemory		; bit単位で管理するメモリをdpに読み出す
	mov	a, track.stepLeft+x
	beq	_SeqAna
	dec	a
	mov	track.stepLeft+x, a	; gate消化
	beq	_Release
	mov	a, track.releaseTiming+x
	beq	_Loop
	dec	a
	mov	track.releaseTiming+x, a
	bne	_Loop
	call	RRApply
	bra	_Loop
_Release:
	mov	a, tmpTrackSysBits
	and	a, #(TRKFLG_TIE | TRKFLG_PORTAM)
	bne	_Loop
	call	ReleaseChannel
	bra	_Loop

_SeqAna:
	mov	a, track.seqPointerH+x
	beq	_Loop
	mov	y, a
	mov	a, track.seqPointerL+x
	movw	lSeqPointer, ya

	call	AnalyzeSeq		; シーケンス解析

	movw	ya, lSeqPointer
	mov	track.seqPointerL+x, a
	mov	a, y
	mov	track.seqPointerH+x, a

_Loop:
	; --- フェーズ処理
	call	PhaseIncrease

	; --- ビット情報のリストア
	call	StoreTrackBitMemory		; bit単位で管理するメモリをdpからメモリに書き込む

	; --- 次トラック解析ループ
	inc	x
	cmp	x, #MUSICTRACKS
	bne	_TrackLoop

	/* 全CH共通処理 */
_GlobalVolumeFade:
	mov	a, mgvolFadeSpan
	beq	_TempoFade
	mov	a, mgvolFadeDtL
	clrc
	adc	a, musicGlobalVolumeL
	mov	musicGlobalVolumeL, a
	mov	a, mgvolFadeDtH
	adc	a, musicGlobalVolume
	mov	musicGlobalVolume, a
	dec	mgvolFadeSpan

_TempoFade:
	mov	a, tempoFadeSpan
	beq	_EndRoutine
	mov	a, tempoFadeDeltaL
	clrc
	adc	a, musicTempoLo
	mov	musicTempoLo, a
	mov	a, tempoFadeDeltaH
	adc	a, musicTempo
	mov	musicTempo, a
	dec	tempoFadeSpan

_EndRoutine:
	ret

;--------------------------------------------------
; MusicProcess - ANALYZE SEQUENCE PROCESS
;   Level       : 2 (Main Sub)
;   Input       : none
;   Output      : none
;   Description : シーケンスデータを解析する
;--------------------------------------------------
/****************************************
 *
 * シーケンス解析処理
 *
 ****************************************/
AnalyzeSeq:
	call	readSeq			; シーケンスを1バイト読みます
	mov	y, a			; フラグ操作
	bne	+			; $00(EndSeq)ではない場合、コマンド解析

	; --- トラック解析終了処理 --- ここから
	mov	y, a			;\
	movw	lSeqPointer, ya		;/ 次Tick移行のトラック解析を停止します

	; キーON中のチャンネルをサーチして、チャンネルを開放します
ReleaseChannel:
	call	SearchUseCh
	and	a, #$ff			; フラグ操作
	bmi	_RetChAlloc		; チャンネル使用なし -> 処理なし
	push	a
	mov	a, #$ff			;\ 割り当て解除
	mov	buf_chData.allocTrack+x, a	;/
	pop	x
	mov	a, !ChBitTable+x	;\  keyoff
	push	a
	or	a, buf_keyoff		; |
	mov	buf_keyoff, a		;/
	pop	a

	eor	a, #$ff
	mov	x, a
	and	a, buf_echo
	mov	buf_echo, a

	mov	a, x
	and	a, buf_noise
	mov	buf_noise, a

	mov	a, x
	and	a, buf_pm
	mov	buf_pm, a
_RetChAlloc:
	mov	a, y			;\ 解析中Trackインデックスを復元
	mov	x, a			;/
	; --- ONのままにしておくと後に影響が出るパラメータ類のリセット
	mov	a, #0
	mov	track.pitchBendSpan+x, a	; 切っておかないと、次の音もスライドする
	mov	track.pitchBendPhase+x, a
	ret
	; --- トラック解析終了処理 --- ここまで

+	bmi	_NoteOrCommand		; 負数である場合は、ノートかコマンド

	; 音の長さ指定
	mov	track.step+x, a		; 整数 -> 長さ指定

	call	readSeq			; シーケンスを1バイト読みます
	mov	y, a
	bmi	_NoteOrCommand		; 負数である場合は、ノートかコマンド

	; Velocity and Gate指定
	call	setVelocityAndGate

	call	readSeq			; シーケンスを1バイト読みます
	mov	y, a
	bmi	_NoteOrCommand		; 負数である場合は、ノートかコマンド
;	stop				; ここに来る場合、フォーマット不正

-	jmp	_JudgeTieRest
_NoteOrCommand:
	cmp	a, #TIE
	bpl	-

	; 普通のNote($80 ~ $c7)
_NormalNote:
	and	a, #$7f
	mov	y, a
	mov	a, track.pitchEnvDiff+x
	bne	+
	mov	a, y
	bra	_KeyStore

	; --- Pitch Envelope
+	eor	a, #$ff
	inc	a
	mov	track.pitchBendDiff+x, a
	mov	a, track.pitchEnvDelay+x
	mov	track.pitchBendDelay+x, a
	mov	a, track.pitchEnvSpan+x
	mov	track.pitchBendSpan+x, a
	mov	a, y
	clrc
	adc	a, track.pitchEnvDiff+x
_KeyStore:
	mov	track.curKey+x, a
	mov	a, track.step+x
	dec	a
	mov	track.stepLeft+x, a
	inc	a
	mov	y, a
	mov	a, track.gate+x
	mul	ya
	mov	a, y
	mov	track.releaseTiming+x, a

	mov	a, tmpTrackSysBits
	and	a, #(TRKFLG_TIE | TRKFLG_PORTAM)
	mov	$04, a
	beq	+
	call	SearchUseCh
	and	a, #$ff			; フラグ操作
	bpl	_Alloc
	and	$04, #TRKFLG_PORTAM
	beq	_AllocFailed		; トラック→チャンネル割り当てなし -> 処理なし
	mov	a, y
	mov	x, a

	; Ch割り付け
+	call	GetFreeCh		; a <- chNumber, y <- trackInx, x <- chInx
	and	a, #$ff			; フラグ操作
	bmi	_AllocFailed		; 割り付け失敗

	push	x
	mov	x, a
	; --- keyon
	mov	a, !ChBitTable+x
	mov	$01, a
	or	a, buf_keyon
	mov	buf_keyon, a

	; --- echo
	mov	a, $01
	eor	a, #$ff
	and	a, buf_echo
	bbc	tmpTrackSysBits.6, +	; bit6: エコー有無効
	or	a, $01
+	mov	buf_echo, a

	; --- noise
	mov	a, $01
	eor	a, #$ff
	and	a, buf_noise
	bbc	tmpTrackSysBits.5, +	; bit5: ノイズ有無効
	or	a, $01
+	mov	buf_noise, a

	; --- pitch moduration
	mov	a, $01
	eor	a, #$ff
	and	a, buf_pm
	bbc	tmpTrackSysBits.4, +	; bit4: ピッチモジュレーション
	or	a, $01
+	mov	buf_pm, a
	pop	x

	; --- lfo reset
	mov	a, track.modulationDelay+y
	mov	track.modulationWaits+y, a
	mov	a, track.tremoloDelay+y
	mov	track.tremoloWaits+y, a
	mov	a, #0
	mov	track.modulationPhase+y, a
	mov	track.tremoloPhase+y, a

_Alloc:
	mov	buf_chData.allocTrack+x, y
	mov	a, !track.brrInx+y
	mov	buf_chData.srcn+x, a
	mov	a, !track.adr+y
	mov	buf_chData.adr+x, a
	bmi	+
	mov	a, !track.sr+y
	mov	buf_chData.gain+x, a
	bra	++
+	mov	a, !track.sr+y
++	mov	buf_chData.sr+x, a

_AllocFailed:
	mov	a, y
	mov	x, a

	; --- Tieの為の先読み処理
	call	LookAheadTie
	ret

_JudgeTieRest:
	cmp	a, #DRUM_NOTE
	bpl	_JudgeDrum
	push	a
	;--- 休符判定
	mov	a, track.step+x
	dec	a
	mov	track.stepLeft+x, a
	pop	a
	cmp	a, #REST
	beq	+
	; --- Tieの為の先読み処理
	call	LookAheadTie
+	ret
	; --- CH開放
;+	call	ReleaseChannel
;	ret

_JudgeDrum:
	cmp	a, #SEQCMD_START
	bpl	_Commands
	; ドラムノート
	push	a
	movw	ya, drumToneTablePtr
	movw	$01, ya
	pop	a
	setc
	sbc	a, #DRUM_NOTE
	mov	y, #_sizeof_stDrumTone
	call	GetToneCfg		; 音色を切り替える
	inc	y
	mov	a, [$01]+y
	call	setVelocityAndGate
	inc	y
	mov	a, [$01]+y
	mov	track.panH+x, a
	inc	y
	mov	a, [$01]+y
	jmp	_NormalNote		; これ以降は普通Noteと同じ処理

_Commands:
	setc				;\  Jump先をセット
	sbc	a, #SEQCMD_START	; |
	asl	a			; |
	mov	y, a			; |
	mov	a, !CmdTable+1+y	; |
	mov	!_jmpTgt+2, a		; |
	mov	a, !CmdTable+y		; |
	mov	!_jmpTgt+1, a		;/
	mov	a, #0
_jmpTgt:
	call	$0000			; <- ここのアドレスは動的に書き換わる
	jmp	AnalyzeSeq		; 普通のNoteを読み込むまでLoopする

/****************************************
 * ReleaseRateApply
 ****************************************/
RRApply:
	call	SearchUseCh
	and	a, #$ff			; フラグ操作
	bmi	_RRReturn		; チャンネル使用なし -> 処理なし
	push	x
	mov	a, y
	mov	x, a
	mov	a, track.adr+x		; ADSR(1) の値を取得
	bmi	+
	; GAIN
	mov	a, track.rr+x
	pop	x
	mov	buf_chData.gain+x, a
	bra	_RRReturn

	; ADSR
+	mov	a, track.rr+x
	and	a, #$1f
	mov	$00, a
	pop	x
	mov	a, buf_chData.sr+x
	and	a, #$e0
	or	a, $00
	mov	buf_chData.sr+x, a

_RRReturn:
	mov	a, y			;\ 解析中Trackインデックスを復元
	mov	x, a			;/
	ret
	
/**************************************************/
/* Velocity / Gate メモリセット                   */
/**************************************************/
setVelocityAndGate:
	push	y
	push	a
	; --- ベロシティ値のセット
	and	a, #$0f
	mov	y, a
	mov	a, !VelocityTable+y
	mov	track.velocity+x, a
	; --- ゲートタイムのセット
	pop	a
	xcn	a
	and	a, #$07
	mov	y, a
	mov	a, !GateTable+y
	mov	track.gate+x, a

	pop	y
	ret

/****************************************
 *
 * シーケンス読み込み
 *
 ****************************************/
readSeq:
	mov	y, #0
	mov	a, [lSeqPointer]+y
	incw	lSeqPointer
	ret

/****************************************
 *
 * シーケンスアドレスの計算
 *
 ****************************************/
SetRelativePointer:
	bbs	rootSysFlags.1, +
	addw	ya, seqBaseAddress
	bra	++
+	addw	ya, sfxBaseAddress
++	movw	lSeqPointer, ya
	ret

/**************************************************/
/* フェーズ処理(LFO等向けの処理)                  */
/**************************************************/
PhaseIncrease:
	; Pitchbend
	mov	a, track.pitchBendSpan+x
	beq	_Modulation
	mov	a, track.pitchBendDelay+x
	beq	_PitchbendPhase
	dec	a
	mov	track.pitchBendDelay+x, a
	bra	_Modulation
_PitchbendPhase:
	mov	a, track.pitchBendSpan+x
	dec	a
	mov	track.pitchBendSpan+x, a
	mov	a, track.pitchBendDiff+x
	bmi	_BendMinus
	; ピッチベンド +方向
	mov	a, track.pitchBendDelta+x
	clrc
	adc	a, track.pitchBendPhase+x
	mov	track.pitchBendPhase+x, a
	mov	a, track.curKey+x
	adc	a, track.pitchBendKey+x
	mov	track.curKey+x, a
	bra	_Modulation
_BendMinus:
	mov	a, track.pitchBendPhase+x
	setc
	sbc	a, track.pitchBendDelta+x
	mov	track.pitchBendPhase+x, a
	mov	a, track.curKey+x
	sbc	a, track.pitchBendKey+x
	mov	track.curKey+x, a

_Modulation:
	mov	a, track.modulationDepth+x
	beq	_Tremolo
	mov	a, track.modulationWaits+x
	beq	_ModulationPhase
	dec	a
	mov	track.modulationWaits+x, a
	bra	_Tremolo
_ModulationPhase:
	mov	a, track.modulationRate+x
	clrc
	adc	a, track.modulationPhase+x
	mov	track.modulationPhase+x, a
	bcc	_Tremolo
	mov	a, tmpPitchFuncBits
	inc	a
	and	a, #3
	and	tmpPitchFuncBits, #$fc
	or	a, tmpPitchFuncBits
	mov	tmpPitchFuncBits, a

_Tremolo:
	mov	a, track.tremoloDepth+x
	beq	_PanFade
	mov	a, track.tremoloWaits+x
	beq	_TremoloPhase
	dec	a
	mov	track.tremoloWaits+x, a
	bra	_PanFade
_TremoloPhase:
	mov	a, track.tremoloRate+x
	clrc
	adc	a, track.tremoloPhase+x
	mov	track.tremoloPhase+x, a
	bcc	_PanFade
	mov	a, tmpVolFuncBits
	inc	a
	and	a, #3
	and	tmpVolFuncBits, #$fc
	or	a, tmpVolFuncBits
	mov	tmpVolFuncBits, a

_PanFade:
	; --- パンフェード
	mov	a, track.panFadeSpan+x
	beq	_VolumeFade
	mov	a, track.panFadeDtL+x
	clrc
	adc	a, track.panL+x
	mov	track.panL+x, a
	mov	a, track.panFadeDtH+x
	adc	a, track.panH+x
	mov	track.panH+x, a
	mov	a, track.panFadeSpan+x
	dec	a
	mov	track.panFadeSpan+x, a

_VolumeFade:
	; --- ボリュームフェード
	mov	a, track.volumeFadeSpan+x
	beq	_PanVibration
	mov	a, track.volumeFadeDtL+x
	clrc
	adc	a, track.volumeL+x
	mov	track.volumeL+x, a
	mov	a, track.volumeFadeDtH+x
	adc	a, track.volumeH+x
	mov	track.volumeH+x, a
	mov	a, track.volumeFadeSpan+x
	dec	a
	mov	track.volumeFadeSpan+x, a

_PanVibration:
	; --- パン振動
	mov	a, track.panVibDepth+x
	beq	_End
	mov	a, track.panVibRate+x
	clrc
	adc	a, track.panVibPhase+x
	mov	track.panVibPhase+x, a
	bcc	_End
	mov	a, tmpVolFuncBits
	xcn	a
	inc	a
	and	a, #3
	xcn	a
	and	tmpVolFuncBits, #$cf
	or	a, tmpVolFuncBits
	mov	tmpVolFuncBits, a

_End:
	ret


/**************************************************/
/* ビット単位で管理するメモリの読み出し           */
/**************************************************/
LoadTrackBitMemory:
	mov	a, track.trackSysBits+x
	mov	tmpTrackSysBits, a
	mov	a, track.volFuncBits+x
	mov	tmpVolFuncBits, a
	mov	a, track.pitchFuncBits+x
	mov	tmpPitchFuncBits, a
	ret

/**************************************************/
/* ビット単位で管理するメモリの保存               */
/**************************************************/
StoreTrackBitMemory:
	mov	a, tmpTrackSysBits
	mov	track.trackSysBits+x, a
	mov	a, tmpVolFuncBits
	mov	track.volFuncBits+x, a
	mov	a, tmpPitchFuncBits
	mov	track.pitchFuncBits+x, a
	ret

;--------------------------------------------------
; MusicProcess - LOOK AHEAD TIE PROCESS
;   Level       : 3 (Main Sub-Sub)
;   Input       : none
;   Output      : none
;   Description : タイ用の先読み処理
;                 次のNoteがTIEの場合に
;                 KEYOFF対象外にする
;--------------------------------------------------
.enum $00
	lLoopStartAdrL		dw
	lLoopStartAdrH		dw
	lLoopReturnAdrL		dw
	lLoopReturnAdrH		dw
	lLoopNumbers		dw
	llntmp			db
.ende

TieNothingToDo:
	ret
LookAheadTie:
	; --- シーケンスポインタを退避する
	movw	ya, lSeqPointer
	beq     TieNothingToDo		; NULLの場合、処理なし
	movw	$00, ya			; $00 に退避
	mov	$03, #0			; 次Tieの長さ

	; --- X(チャンネル番号)を退避する
	push	x

	; --- ループ先読みで変更する為
	; --- サブルーチン関連メモリをコピー
	mov	a, x
	asl	a
	mov	x, a
	setp
	mov	a, loopStartAdrL+x
	mov	lLoopStartAdrL, a
	mov	a, loopStartAdrL+1+x
	mov	lLoopStartAdrL+1, a
	mov	a, loopStartAdrH+x
	mov	lLoopStartAdrH, a
	mov	a, loopStartAdrH+1+x
	mov	lLoopStartAdrH+1, a
	mov	a, loopReturnAdrL+x
	mov	lLoopReturnAdrL, a
	mov	a, loopReturnAdrL+1+x
	mov	lLoopReturnAdrL+1, a
	mov	a, loopReturnAdrH+x
	mov	lLoopReturnAdrH, a
	mov	a, loopReturnAdrH+1+x
	mov	lLoopReturnAdrH+1, a
	mov	a, loopNumbers+x
	mov	lLoopNumbers, a
	mov	a, loopNumbers+1+x
	mov	lLoopNumbers+1, a
	clrp
	bra	_laLoop

_SetNextSteps:
	mov	y, $03
	bne	_laLoop
	mov	$03, a
; 先読みループ
_laLoop:
	call	readSeq
	mov	y, a			; ネガティブフラグの操作
	bpl	_SetNextSteps		; <--- 音長/ゲートタイム・ベロシティ指定判定
	cmp	a, #TIE			; is tie?
	beq	_Tie

	cmp	a, #SEQCMD_START
	bmi	_NotTie			; 通常音符の場合、切る
	cmp	a, #CMD_PORTAM_OFF
	bne	+
	clr1	tmpTrackSysBits.7
	bra	_NotTie			; 次コマンドがポルタメントOFFの場合、切る

+	mov	x, #0			; Jumpの分岐で使用する為、0(false)に
	cmp	a, #CMD_JUMP
	beq	_lookJump
	cmp	a, #CMD_SUBROUTINE
	bne	_lookSubBreak

	; --- サブルーチン
	call	readSeq
	mov	llntmp+$100, a
	inc	x

	; --- ジャンプ・サブルーチン共通処理
_lookJump:
	call	_laJumpCmd
	bra	_laLoop

	; --- サブルーチンの途中離脱
_lookSubBreak:
	cmp	a, #CMD_SUBROUTINE_BREAK
	bne	_lookSubReturn
	setp
	call	_checkLoopNumsLA
	beq	_subreturn
	clrp
	jmp	_laLoop

	; --- サブルーチン末端
_lookSubReturn:
	cmp	a, #CMD_SUBROUTINE_RETURN
	bne	_skipCmd			; ジャンプ系以外のコマンドは、スキップ
	setp
	call	_checkLoopNumsLA
	bne	+
_subreturn:
	mov	a, #0
	mov	lLoopStartAdrH+x, a
	mov	a, lLoopReturnAdrH+x
	mov	y, a
	mov	a, lLoopReturnAdrL+x
	bra	++

+	mov	a, #0				; 疑似的にループを全て消化したものとする
	mov	lLoopNumbers+x, a		; (※サブルーチン内が全てコマンドの場合の対策処理)
	mov	a, lLoopStartAdrH+x
	mov	y, a
	mov	a, lLoopStartAdrL+x
++	clrp
	movw	lSeqPointer, ya
	jmp	_laLoop

_NotTie:
	pop	x
	cmp	a, #REST			; 次が休符の場合は音を止めます
	beq	++
	bbc	tmpTrackSysBits.7, ++		; bit7: Portament
	mov	a, $03
	bne	+
	mov	a, track.step+x
+	clrc
	adc	a, track.releaseTiming+x
	mov	track.releaseTiming+x, a
++	clr1	tmpTrackSysBits.0
	bra	_Join
_Tie:
	pop	x
	mov	a, track.gate+x
	mov	y, a
	mov	a, $03
	bne	+
	mov	a, track.step+x
+	mul	ya
	mov	$02, y
	mov	a, track.releaseTiming+x
	beq	+
	clrc
	adc	a, $02
	mov	track.releaseTiming+x, a
+	set1	tmpTrackSysBits.0
_Join:
	movw	ya, $00
	movw	lSeqPointer, ya
	ret

;--- ジャンプ系以外のコマンドの読み飛ばし処理
_skipCmd:
	setc
	sbc	a, #SEQCMD_START
	mov	x, a
	mov	a, !CmdLengthTable+x
	mov	y, #0
	clrc
	addw	ya, lSeqPointer
	movw	lSeqPointer, ya
	jmp	_laLoop


_laJumpCmd:
	; --- ルーチンアドレス読み出し
	call	readSeq
	push	a
	call	readSeq
	mov	y, a

	; --- Jump分岐チェック
	mov	a, x
	bne	+			; --- false(ただのJump命令)
	pop	a
	jmp	SetRelativePointer

	; --- サブルーチン用処理
+	mov	x, #0
	mov	a, lLoopStartAdrH+$100
	beq	+
	inc	x
	; --- ループ回数
+	mov	a, llntmp
	mov	lLoopNumbers+x, a
	; --- 戻りアドレスのセット
	mov	a, lSeqPointer
	mov	lLoopReturnAdrL+$100+x, a
	mov	a, lSeqPointer+1
	setp
	mov	lLoopReturnAdrH+x, a
	; --- ルーチン開始アドレスのセット
	clrp
	pop	a
	call	SetRelativePointer
	setp
	mov	lLoopStartAdrL+x, a
	mov	a, y
	mov	lLoopStartAdrH+x, a
	clrp
	ret

_checkLoopNumsLA:
	mov	x, #0
	mov	a, lLoopStartAdrH+1
	beq	+
	inc	x
+	mov	a, lLoopNumbers+x	; ループ残り回数チェック
	ret

.undefine	lLoopStartAdrL
.undefine	lLoopStartAdrH
.undefine	lLoopReturnAdrL
.undefine	lLoopReturnAdrH
.undefine	lLoopNumbers
.undefine	llntmp

/****************************************
 * FIR セットコマンド
 ****************************************/
CmdSetFIR:
	mov	y, #0
-	mov	a, y
	xcn	a
	or	a, #DSP_FIR
	mov	SPC_REGADDR, a
	mov	a, [lSeqPointer]+y
	mov	SPC_REGDATA, a
	inc	y
	cmp	y, #8
	bmi	-

	mov	a, y
	mov	y, #0
	clrc
	addw	ya, lSeqPointer
	movw	lSeqPointer, ya
	ret

/****************************************/
/* シーケンスの初期化                   */
/****************************************/
InitSequenceData:
	mov	a, #0
	mov	y, a
	mov	x, a
	mov	drumToneTablePtr, a
	mov	drumToneTablePtr+1, a
	mov	exToneTablePtr, a
	mov	exToneTablePtr+1, a
	; ベースポインタ読み出し
	mov	a, !TrackLocation
	mov	seqBaseAddress, a
	mov	a, !TrackLocation+1
	mov	seqBaseAddress+1, a
	; ドラムトーンテーブル位置の設定
	mov	drumToneTablePtr, #(MUSICTRACKS*2)
	movw	ya, seqBaseAddress
	clrc
	addw	ya, drumToneTablePtr
	movw	drumToneTablePtr, ya
	; 拡張音色テーブル位置の設定
	mov	exToneTablePtr, #(_sizeof_stDrumTone * 7)
	clrc
	addw	ya, exToneTablePtr
	movw	exToneTablePtr, ya

	; シーケンスアドレスの設定
	mov	y, #0
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

	; チャンネルデータの初期化
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

	; トラックデータの初期化
	mov	a, #0
	mov	y, a
	mov	x, a
-	mov	a, #0
	mov	track.trackSysBits+x, a
	mov	track.volFuncBits+x, a
	mov	track.pitchFuncBits+x, a
	mov	track.modulationDepth+x, a
	mov	track.tremoloDepth+x, a
	mov	track.pitchBendSpan+x, a
	mov	track.panL+x, a
	mov	track.volumeL+x, a
	call	SetInstrumentA
	mov	a, #32
	mov	track.panH+x, a
	mov	a, #192
	mov	track.volumeH+x, a
	inc	x
	cmp	x, #TRACKS
	bmi	-

	; 全体データの初期化
	mov	musicGlobalVolume, a
	mov	musicTempo, #60
	mov	eVolRatio, #0
	mov	specialWavFreq, #SPECIAL_WAV_FREQ

	; ミュート解除
	mov	SPC_REGADDR, #DSP_FLG
	mov	SPC_REGDATA, #0
	ret

TrackLocation:
	.dw	$2000

;------------------------------
; local values undefine
;------------------------------
.undefine		lSeqPointer

.ends

.incdir "./table/"
.include "musicTable.tab"

