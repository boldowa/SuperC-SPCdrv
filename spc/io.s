/**
 * SuperC I/O port process
 *
 *
 */

.incdir  "./include/"
.include "./spc.inc"
.include "./var.inc"

.macro waitSnes
	mov   SPC_PORT0,a
-	cmp   a,SPC_PORT0
	bne   -
.endm


.section "__io__" free
;--------------------------------------------------
; IOProc - SPC I/O PORT PROCESS
;   Level       : 0 (Clock Level)
;   Input       : none
;   Output      : none
;   Description : SPCポートの内容を読み取り、
;                 各種処理を起動する
;--------------------------------------------------
_resetSpc:
	mov   SPC_CONTROL,#(CNT_IPL|CNT_PC32|CNT_PC10)
	jmp   $ffc0
_return:
	ret

; ***** Entry point *****
IOCommProcess:
	mov	a, SPC_PORT0                     ; Port0読み出し
	beq	_return                         ; Port0が空なのでNOP
	cmp	a, #$ff                          ; > is port0 = 255 ?
	beq	_resetSpc                       ; Port0が255なのでSPCシステムリセット起動

	asl   a
	mov   x,a                             ; Xレジスタにテーブルインデクス格納
; ***** I/Oコマンド  *****
	jmp   [!_jmpTable-2+x]                ; I/O コマンド ジャンプ

_jmpTable:
	.dw   transMusic                      ; 01h ... 音楽転送
	.dw   SE                              ; 02h ... 効果音処理
	.dw   setMasterVolume                 ; 03h ... マスター音量調整
	.dw   setMusicVolume                  ; 04h ... 音楽音量調整
	.dw   setSoundVolume                  ; 05h ... 効果音音量調整
	.dw   switchStereoMono                ; 06h ... ステレオ・モノラル切替
	.dw   DspWrite                        ; 07h ...
	.dw   SpcWrite                        ; 08h ... 
	.dw   EchoTest                        ; 09h ... 
.ends


.section "trans" free
;--- dp0
.enum $00
	dest		dw	/* データ書き込み先          */
	dirInx		dw	/* 更新するDIRテーブルの位置 */
.ende

.macro SyncCpu
	mov	a, SPC_PORT0		; 読み出し
	mov	SPC_PORT0, a		; エコーバック
-	cmp	a, SPC_PORT0		; SNES側で次情報が書き込まれるのを待つ
	beq	-
.endm

transMusic:
	;------------------------------
	; Step1 : Receive EDL
	;------------------------------
	mov	a, !DIRLoc
	mov	dirInx+1, a
	mov	dirInx, #($20*4)
	mov	a, SPC_PORT1
	bpl	_ChangeEdl		; EDLが負数の場合、Waitなしの変則モード
	movw	ya, brrReceiveHead
	movw	dest, ya
	bra	_NoWait

_ChangeEdl
	and	a, #$0f
	mov	y, a
	mov	edlMax, a
	bne	+
	mov	a,#4
	bra	++
	;--- EDLを8倍
+	xcn	a
	lsr	a
	;--- 波形転送先算出
	clrc
	adc	a, !ESALoc	; ESA + EDL -> 波形転送先
	mov	dest+1, a
	mov	brrReceiveHead+1, a
	mov	a, #0
++	mov	dest, a
	mov	brrReceiveHead, a
	;--- エコーカウンタのリセット待ち
	mov	SPC_REGADDR, #DSP_FLG
	mov	a, SPC_REGDATA
	or	a, #(FLG_ECEN|FLG_MUTE)		; エコーを切り、ミュート状態にします
	mov	SPC_REGDATA, a
	mov	SPC_REGADDR, #DSP_EDL
	mov	a, y
	cmp	a, SPC_REGDATA
	bpl	+
	mov	y, SPC_REGDATA
+	cmp	y, #0
	beq	+
	mov	SPC_REGDATA, a
	mov	a, #$81
	call	Sleep

	/*----------------------------------------------*/
	/* 以下は、EDL/ESA 両方とも書き換える場合でも   */
	/* 異音を出さないようにする安全コード           */
	/* ただし、待機時間が最大で↑のコードの２倍近く */
	/* になります。                                 */
	/* つまりどういうことかというと、               */
	/* 音楽の転送にかかる時間が増大します。         */
	/*----------------------------------------------*/
.if 1 == 0	; <- C言語等でいうところの、"#if 0" 
+	mov	y, edlMax
+	mov	SPC_REGDATA, a
	beq	+
	mov	a, #$81
	call	Sleep
+	mov	a, SPC_PORT1
	and	a, #$0f
	mov	edlMax, a
	mov	y, a
	mov	SPC_REGADDR, #DSP_EDL
	mov	SPC_REGDATA, a
	beq	+
	mov	a, #$81
	call	Sleep
.endif
	/* FLG復元 */
+	mov	SPC_REGADDR, #DSP_FLG
	mov	a, SPC_REGDATA
	and	a, #($ff~FLG_ECEN)
	mov	SPC_REGDATA, a
_NoWait:
	SyncCpu

_BrrExists:
	;------------------------------
	; Step2-1 : Receive BRR Loop
	;------------------------------
	mov	a, SPC_PORT1		; 転送有無(BRR波形番号)を取得する
	beq	_Sequence		; 転送なし(0x00) ならば、シーケンス転送処理に移行する

	mov	y, #0
	mov	a, dest
	mov	[dirInx]+y, a
	inc	y
	mov	a, dest+1
	mov	[dirInx]+y, a
	inc	y
	mov	a, SPC_PORT2
	clrc
	adc	a, dest
	mov	[dirInx]+y, a
	inc	y
	mov	a, SPC_PORT3
	adc	a, dest+1
	mov	[dirInx]+y, a
	inc	y
	mov	a, y
	mov	y, #0
	clrc
	addw	ya, dirInx
	movw	dirInx, ya
	;------------------------------
	; Step2-2 : Receive BRR
	;------------------------------
	call	ParallelPortReceive
	bra	_BrrExists

_Sequence:
	;------------------------------
	; Step3 : Receive Sequence
	;------------------------------
	mov	seqBaseAddress, dest
	mov	seqBaseAddress+1, dest+1
	call	ParallelPortReceive

	;------------------------------
	; 音楽の初期化
	;------------------------------
	call	InitSequenceData
	; ミュート状態を解除します
	mov	SPC_REGADDR, #DSP_FLG
	mov	a, SPC_REGDATA
	and	a, #($ff~FLG_MUTE)
	mov	SPC_REGDATA, a
	; カウンタレジスタをクリアします
	mov	a, SPC_COUNTER0

	;------------------------------
	; 後始末
	;------------------------------
PortClear:
	mov	a, #0
	mov	x, #SPC_PORT0
	mov	(x)+, a			;\
	mov	(x)+, a			; | PortX-Wレジスタクリア
	mov	(x)+, a			; |
	mov	(x)+, a			;/
	mov	a, spcControlRegMirror
	or	a, #(CNT_PC32|CNT_PC10)	; PortX-Rレジスタクリア
	mov	SPC_CONTROL, a
_Return:
	ret

;-------------------------------------------------
; パラレルポート転送
;   CPU-APU間の同期が、シリアルポート転送の1/3に
;   なる為、転送速度はシリアルポートと比べると
;   ３倍弱程度になることが期待できます。
;-------------------------------------------------
ParallelPortReceive:
	SyncCpu
	mov	a, SPC_PORT0
	beq	_Return
	mov	y, #0
	mov	a, SPC_PORT1
	mov	[dest]+y, a
	inc	y
	mov	a, SPC_PORT2
	mov	[dest]+y, a
	inc	y
	mov	a, SPC_PORT3
	mov	[dest]+y, a
	inc	y
	mov	a, y
	clrc
	adc	a, dest
	mov	dest, a
	adc	dest+1, #0
	bra	ParallelPortReceive

.undef dest, dirInx
.ends



.section "MINI-IO-FUNC" free
;---------------------------------------
; マスター音量設定
;---------------------------------------
setMasterVolume:
	mov	a, SPC_PORT1
	mov	x, #SPC_REGDATA
	mov	SPC_REGADDR, #DSP_MVOLL
	mov	(x), a
	mov	SPC_REGADDR, #DSP_MVOLR
	mov	(x), a
_EndIOFunc:
	SyncCpu
	call	PortClear
	mov	a, #0
	ret
;---------------------------------------
; ステレオ・モノラル切替
;---------------------------------------
switchStereoMono:
	clrc
	mov	a, SPC_PORT1
	bne	+
	setc
+	mov1	musicSysFlags.2, c
	bra	_EndIOFunc
;---------------------------------------
; DSP WRITE Command
;---------------------------------------
DspWrite:
	mov	SPC_REGADDR, SPC_PORT1
	mov	SPC_REGDATA, SPC_PORT2
	bra	_EndIOFunc
;---------------------------------------
; SPC WRITE Command
;---------------------------------------
SpcWrite:
	mov	y, #0
	mov	a, SPC_PORT3
	mov	[SPC_PORT1]+y, a
	bra	_EndIOFunc
;---------------------------------------
; Echo Test Command
;---------------------------------------
EchoTest:
	mov	SPC_REGADDR, #DSP_KOF
	mov	SPC_REGDATA, #$ff
	mov	SPC_REGADDR, #DSP_FLG
	mov	SPC_REGDATA, #(FLG_ECEN|FLG_MUTE)
-	bra	-



setMusicVolume:
setSoundVolume:
SE:
	mov	a, #0
	ret
.ends
