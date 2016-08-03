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
	mov	a, track.stepLeft+x
	beq	_SeqAna
	dec	a
	mov	track.stepLeft+x, a	; gate消化
	beq	_Release
	mov	a, track.releaseTiming+x
	dec	a
	mov	track.releaseTiming+x, a
	bne	_Loop
	call	RRApply
	bra	_Loop
_Release:
	mov	a, track.bitFlags+x
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
	inc	x
	cmp	x, #MUSICTRACKS
	bne	_TrackLoop

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
;	mov	buf_chData.srcn+x, a	; debug
	pop	x
	mov	a, !ChBitTable+x	;\  keyoff
	push	a
	or	a, buf_keyoff		; |
	mov	buf_keyoff, a		;/
	pop	a
	eor	a, #$ff
	and	a, buf_echo
	mov	buf_echo, a
_RetChAlloc:
	mov	a, y			;\ 解析中Trackインデックスを復元
	mov	x, a			;/
	ret
	; --- トラック解析終了処理 --- ここまで

+	bmi	_NoteOrCommand		; 負数である場合は、ノートかコマンド

	; 音の長さ指定
	mov	track.step+x, a		; 整数 -> 長さ指定

	call	readSeq			; シーケンスを1バイト読みます
	mov	y, a
	bmi	_NoteOrCommand		; 負数である場合は、ノートかコマンド

	; Velocity and Gate指定
	push	x

	push	a
	and	a, #$0f
	mov	x, a
	mov	a, !VelocityTable+x
	mov	$00, a
	pop	a
	xcn	a
	and	a, #$07
	mov	x, a
	mov	a, !GateTable+x

	pop	x
	mov	track.gate+x, a
	mov	a, $00
	mov	track.velocity+x, a

	call	readSeq			; シーケンスを1バイト読みます
	mov	y, a
	bmi	_NoteOrCommand		; 負数である場合は、ノートかコマンド
;	stop				; ここに来る場合、フォーマット不正

-	jmp	_JudgeTieRest
_NoteOrCommand:
	cmp	a, #$c8
	bpl	-

	; 普通のNote($80 ~ $c7)
	and	a, #$7f
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

	mov	a, track.bitFlags+x
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
	mov	a, !track.bitFlags+y
	asl	a
	mov	$00, a
	; --- keyon
	mov	a, !ChBitTable+x
	mov	$01, a
	or	a, buf_keyon
	mov	buf_keyon, a

	; --- echo
	mov	a, $01
	eor	a, #$ff
	and	a, buf_echo
	asl	$00
	bcc	+
	or	a, $01
+	mov	buf_echo, a

	; --- noise
	mov	a, $01
	eor	a, #$ff
	and	a, buf_noise
	asl	$00
	bcc	+
	or	a, $01
+	mov	buf_noise, a
	pop	x

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
	mov	a, track.modulationDelay+y
	mov	track.modulationWaits+y, a
	mov	a, #0
	mov	a, track.modulationPhase+x

_AllocFailed:
	mov	a, y
	mov	x, a

	; --- Tieの為の先読み処理
	call	LookAheadTie
	ret

_JudgeTieRest:
	cmp	a, #$ca
	bpl	_JudgeDrum
	push	a
	;--- c8 : tie, c9 : Rest
	mov	a, track.step+x
	dec	a
	mov	track.stepLeft+x, a
	pop	a
	cmp	a, #$c9
	beq	+
	; --- Tieの為の先読み処理
	call	LookAheadTie
	ret
	; --- CH開放
+	call	ReleaseChannel
	ret

_JudgeDrum:
	cmp	a, #SEQCMD_START
	bpl	_Commands
	; todo - Drum Note($ca ~ SEQCMD_START-1, 計7コ)
	call	LookAheadTie
	ret

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
	addw	ya, seqBaseAddress
	movw	lSeqPointer, ya
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
TieNothingToDo:
	ret
LookAheadTie:
	movw	ya, lSeqPointer
	beq     TieNothingToDo		; NULLの場合、処理なし
	movw	$00, ya
	push	x

	; --- ループ先読みで変更する為
	; --- ループ開始アドレスをバックアップ
	push	x
	mov	a, x
	asl	a
	mov	x, a
	mov	a, loopStartAdrH+$100+x
	mov	$03, a
	mov	a, loopStartAdrH+$101+x
	mov	$04, a
	pop	x

--	mov	y, #0
-	mov	a, [lSeqPointer]+y
	bmi	+
	inc	y
	bra	-
+	cmp	a, #$c8			; is tie?
	beq	_Tie

+	cmp	a, #SEQCMD_START
	bmi	_NotTie
	cmp	a, #CMD_JUMP
	beq	_lookJump
	cmp	a, #CMD_SUBROUTINE
	bne	_lookSubBreak
	inc	y
_lookJump:
	inc	y
	mov	a, y
	mov	y, #0
	clrc
	addw	ya, lSeqPointer
	movw	lSeqPointer, ya
	call	CmdJump
	bra	--

_lookSubBreak:
	cmp	a, #CMD_SUBROUTINE_BREAK
	bne	_lookSubReturn
	push	x
	setp
	call	checkLoopNums
	beq	_subreturn
	pop	x
	clrp
	inc	y
	bra	-
_lookSubReturn:
+	cmp	a, #CMD_SUBROUTINE_RETURN
	bne	_skipCmd
	push	x
	setp
	call	checkLoopNums
	bne	+
_subreturn:
	mov	a, #0
	mov	loopStartAdrH+x, a
	mov	a, loopReturnAdrH+x
	mov	y, a
	mov	a, loopReturnAdrL+x
	bra	++
+	mov	a, loopStartAdrH+x
	mov	y, a
	mov	a, loopStartAdrL+x
++	clrp
	movw	lSeqPointer, ya
	pop	x
	jmp	--

_skipCmd:
	setc
	sbc	a, #SEQCMD_START
	mov	x, a
	mov	a, y
	clrc
	adc	a, !CmdLengthTable+x
	mov	y, a
	bra	-

_NotTie:
	pop	x
	cmp	a, #$c9				; 次が休符の場合は音を止めます
	beq	+
	mov	a, track.bitFlags+x
	and	a, #TRKFLG_PORTAM
	beq	+
	mov	a, track.releaseTiming+x
	clrc
	adc	a, track.step+x
	mov	track.releaseTiming+x, a
+	mov	a, track.bitFlags+x
	and	a, #(TRKFLG_TIE ~ $ff)
	bra	_Join
_Tie:
	pop	x
	mov	a, track.gate+x
	mov	y, a
	mov	a, track.step+x
	mul	ya
	mov	$02, y
	mov	a, track.releaseTiming+x
	beq	+
	clrc
	adc	a, $02
	mov	track.releaseTiming+x, a
+	mov	a, track.bitFlags+x
	or	a, #TRKFLG_TIE
_Join:
	mov	track.bitFlags+x, a

	; --- ループ開始アドレスをリストア
	push	x
	mov	a, x
	asl	a
	mov	x, a
	mov	a, $03
	mov	loopStartAdrH+$100+x, a
	mov	a, $04
	mov	loopStartAdrH+$101+x, a
	pop	x

	movw	ya, $00
	movw	lSeqPointer, ya
	ret

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

;------------------------------
; local values undefine
;------------------------------
.undefine		lSeqPointer

.ends

.incdir "./table/"
.include "musicTable.tab"

