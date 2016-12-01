/**
 * SuperC SPC Player SPCDriver Transport
 */
.incdir "include"
.include "map.inc"
.include "header.inc"
.incdir ""

/* GLOBAL VALUE */
.define ESA 0		/* 初期ESA値定義のあるARAMアドレス */
.define TBL 0		/* トラックテーブルのARAMアドレス */
.define LOC 0		/* SPCドライバの挿入位置のARAMアドレス */
.define MUSICBASE 0	/* ESAを含めた音楽データの転送先ARAMアドレス */

;--------------------------------------------------
;   rom read from pointer ...
;   < args >
;       - ptr ... pointer ( dp )
;   < require >
;       "A" register must be 8bit mode.
;--------------------------------------------------
.macro readByteDp args dp
	lda.b	[dp]
	inc.b	dp
	bne	+
	inc.b	dp+1
	bne	+
	inc	dp+2
	pha
	lda.b	#$80
	sta.b	dp+1
	pla
+
.endm


/**************************************************/
/* SPCドライバの転送処理                          */
/*   電源ON時に一度だけ使用します。               */
/*   それ以降のデータ転送は、                     */
/*   TransportMusicを使用します。                 */
/**************************************************/

.bank 0
.org 0
.section "UPLOAD_SPCDRIVER" free

/**************************************************/
/* ローカル変数定義                               */
/**************************************************/
.enum Scratch
	lDataPointer	dsb 3
	lSyncCounter	db
.ende

UploadSPCDriver:
	php

	rep	#$30

	; SPCの準備待ち
	lda.w	#$bbaa
-	cmp.w	SPC_PORT0
	bne	-

	sep	#$20
	lda.b	#$cc
	sta.b	lSyncCounter

_UploadLoop:
	readByteDp lDataPointer	; 読み込みサイズ Lo
	xba
	readByteDp lDataPointer	; 読み込みサイズ Hi
	pha
	readByteDp lDataPointer	; 転送先 Lo
	sta.w	SPC_PORT2
	readByteDp lDataPointer	; 転送先 Hi
	sta.w	SPC_PORT3
	pla
	xba
	rep	#$20
	cmp.w	#0
	beq	_ReturnUpload	; 読み込みサイズが0の場合、転送を終了する

	tax			; 転送サイズをXに移動
	sep	#$20
	; SPCに書き込み準備完了を通知します
	lda.b	#1
	sta.w	SPC_PORT1
	lda.b	lSyncCounter
	sta.w	SPC_PORT0
-	cmp.w	SPC_PORT0	; SPCの応答を待ちます
	bne	-
	stz.b	lSyncCounter

_SPCWriteLoop:
	readByteDp lDataPointer
	sta.w	SPC_PORT1
	lda.b	lSyncCounter
	sta.w	SPC_PORT0
-	cmp.w	SPC_PORT0
	bne	-
	inc	lSyncCounter
	dex
	bne	_SPCWriteLoop

	; 次ブロックの読み出し処理に移行します
	inc	lSyncCounter
	jmp	_UploadLoop

_ReturnUpload:
	sep	#$20
	stz.w	SPC_PORT1
	lda.b	lSyncCounter
	sta.w	SPC_PORT0
-	cmp.w	SPC_PORT0
	bne	-
	;-----------------------------
	; ドライバの初期化処理完了を待ちます
-	lda.w	SPC_PORT0
	bne	-

	; Pレジスタの内容を復元して処理を抜けます
	plp
	rts

/**************************************************/
/* ローカル変数定義の削除                         */
/**************************************************/
.undefine	lDataPointer
.undefine	lSyncCounter

.ends


/*----------------------------------------------*/
/* SPC音楽データの転送処理                      */
/*----------------------------------------------*/
.section "TRANSPORT_MUSIC" free

; ローカル変数
.define lMusicNumber	$01	;sp 1-2
.define lBrrSetIndex	$03	;sp 3-4
.define lTablePtr	$05	;sp 5-6
.define lBrrNumberTmp	$07	;sp 7-8

/*--------------------------*/
/* データ転送同期           */
/*--------------------------*/
.macro SyncApu
	lda.w	SPC_PORT0
	inc	a
	bne	+
	inc	a
+	sta.w	SPC_PORT0
-	cmp	SPC_PORT0
	bne	-
.endm

/*--------------------------*/
/* データ転送同期           */
/* (末端データ用)           */
/*--------------------------*/
.macro SyncApuDataTerm
	lda.b	#0
+	sta.w	SPC_PORT0
-	cmp	SPC_PORT0
	bne	-
.endm

/*--------------------------*/
/* データ転送               */
/*--------------------------*/
TransportMusic:
	sta.l	previousMusicNumber
	rep	#$30
	phx
	phy
	and.w	#$00ff
	tax

	;--- ローカル変数用エリア定義
	tsc
	sec
	sbc	#8
	tcs

	txa
	sta.b	lMusicNumber,s
	asl
	tay

	sep	#$20

	lda.l	BrrSetNumber,x
	cmp.l	previousBrrSet
	php
	bne	_ChangeBrrSet
	lda.b	#$ff			; EDL変更なし
	bra	_StartTransport

_ChangeBrrSet:
	lda	#:BrrSetAddress		; Bank read (固定)
	sta.b	$02
	rep	#$20
	tyx
	lda.l	BrrSetAddress,x
	sta.b	$00
	sep	#$20
	ldy.w	#0
	lda.b	[$00],y

_StartTransport:
	sta.w	SPC_PORT1
	lda.b	#1
	sta	SPC_PORT0
	;-----------------------------
	; EchoArea初期化完了待ち
	;-----------------------------
-	cmp	SPC_PORT0
	bne	-

	rep	#$20
	txa
	clc
	adc	lMusicNumber+1,s
	sta.b	lTablePtr+1,s
	tax
	plp
	rep	#$30
	bne	_SendBrrSet
	lda.w	#0			; "0"は、転送末端を意味する
	sta.w	SPC_PORT1
	jmp	_SendSequence

_SendBrrSet:
	iny
	tya
	sta	lBrrSetIndex,s
	sep	#$20
	lda.b	[$00],y
	sta.w	SPC_PORT1
	beq	_SendSequence
	;SyncApuDataTerm
	rep	#$20
	lda.b	[$00],y
	and.w	#$00ff
	sta.b	lBrrNumberTmp,s
	asl
	clc
	adc	lBrrNumberTmp,s
	tax
	lda.l	BrrDataAddress+1,x
	sta.b	$04
	lda.l	BrrDataAddress,x
	sta.b	$03
	ldy.w	#0
	lda.b	[$03],y
	sta.w	SPC_PORT2
	iny
	iny
	lda.b	[$03],y
	sta.w	$4204
	lda.w	#4
	clc
	adc	$03
	bcc	+
	ora	#$8000
	inc	$05
+	sta.b	$03
	sep	#$20
	lda.b	#3		/* データサイズ/3 = 転送処理を回す回数 */
	sta.w	$4206
	SyncApuDataTerm
	ldx.w	$4214
	lda.w	$4216
	beq	+		/* 転送端数が出たら、もう一回転送するドン！(BRRは普通出ない) */
	inx
+	jsr	ParallelPortSend
	rep	#$20
	lda	lBrrSetIndex,s
	tay
	jmp	_SendBrrSet

_SendSequence:
	rep	#$20
	lda	lTablePtr,s
	tax
	lda.l	SeqDataAddress+1,x
	sta.b	$04
	lda.l	SeqDataAddress,x
	sta.b	$03
	ldy.w	#0
	lda.b	[$03],y
	sta.w	$4204
	lda.w	#2
	clc
	adc	$03
	bmi	+
	ora	#$8000
	inc	$05
+	sta.b	$03
	sep	#$20
	lda.b	#3		/* データサイズ/3 = 転送処理を回す回数 */
	sta.w	$4206
	SyncApuDataTerm
	ldx.w	$4214
	lda.w	$4216
	beq	+		/* 転送端数が出たら、もう一回転送するドン！(シーケンスは端数が出ることあり) */
	inx
+	jsr	ParallelPortSend
	SyncApuDataTerm

	;-----------------------------
	; ポートクリアはSPCに任せる
	; (SPC_CONTROL に#$30書込)
	;-----------------------------

	;--- ローカル変数用エリア解放
	rep	#$30
	tsc
	clc
	adc	#8
	tcs

	ply
	plx
	sep	#$30
-	rts
;---------------------------------------
; パラレルポートデータ転送処理
;   - BRR、およびシーケンスを転送する
;     メインのサブルーチン
;   - Xレジスタを転送カウンタとして使用
;---------------------------------------
ParallelPortSend:
	dex
	bmi	-
	readByteDp $03
	sta.w	SPC_PORT1
	readByteDp $03
	sta.w	SPC_PORT2
	readByteDp $03
	sta.w	SPC_PORT3
	SyncApu
	jmp	ParallelPortSend

.undef lMusicNumber, lTablePtr, lBrrSetIndex

.ends

.bank 0
.org $1000
.section "SPCDRIVER" superfree
SPCDriver:
	;ReadDriver "../spc/SuperC.bin"
	.db	0		; dummy data
	;.incbin "hc.bin"
.ends

/*----------------------------------------------*/
/* mmlcコンパイル時にドライバコードを挿入する為 */
/* bank0のおしりに<del>肉○器と</del>挿入先を   */
/* 書いておきます                               */
/*----------------------------------------------*/
.org $7fbd
	.dw	SPCDriver
	.db	:SPCDriver

/* デバッグ用に、シンボル情報をエクスポートしておく */
/* この値がmmlcで検出される値と違うと、よろしくない */
.export ESA
.export TBL
.export LOC
.export MUSICBASE

