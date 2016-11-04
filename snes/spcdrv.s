/**
 * SuperC SPC Player Interrupt Handler
 */
.incdir "include"
.include "map.inc"
.include "header.inc"
.incdir ""

/**************************************************/
/* データ読み込みマクロ                           */
/* 指定ポインタからAレジスタに1バイト読み出す     */
/*   pointer : 読み込みアドレスポインタ           */
/**************************************************/
.macro LoadData args pointer
	lda.b	[pointer]
	ldy.b	pointer
	iny
	bne	+
	inc	pointer+2
	ldy.w	#$8000
+	sty	pointer
.endm

/**************************************************/
/* SPCドライバ読み込み処理                        */
/**************************************************/



.bank 0
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
	LoadData lDataPointer	; 読み込みサイズ Lo
	xba
	LoadData lDataPointer	; 読み込みサイズ Hi
	pha
	LoadData lDataPointer	; 転送先 Lo
	sta.w	SPC_PORT2
	LoadData lDataPointer	; 転送先 Hi
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
	LoadData lDataPointer
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

	; SPCポートの後始末をします
	stz.w	SPC_PORT0
	stz.w	SPC_PORT1
	stz.w	SPC_PORT2
	stz.w	SPC_PORT3

	; Pレジスタの内容を復元して処理を抜けます
	plp
	rts

/**************************************************/
/* ローカル変数定義の削除                         */
/**************************************************/
.undefine	lDataPointer
.undefine	lSyncCounter

.ends


/**************************************************/
/* SPCドライバ読み込み                            */
/**************************************************/
.macro ReadDriver args fileName
	.fopen fileName fp
	.fsize fp size
	; "VER ", "DIR " の情報と、"ESA "の文字読み飛ばし
	.rept 16
	.fread fp data
	.endr
	; esa情報の格納位置を取得
	.fread fp data
	.redefine ESA data
	.fread fp data
	.redefine ESA (ESA+(data<<8))
	; TBL情報を得る
	.rept 4
	.fread fp data
	.endr
	.fread fp data
	.redefine TBL data
	.fread fp data
	.redefine TBL (TBL+(data<<8))
	; LOC情報を得る
	.rept 4
	.fread fp data
	.endr
	.fread fp data
	.redefine LOC data
	.fread fp data
	.redefine LOC (LOC+(data<<8))
	.redefine MUSICDATA (size-34+LOC)
	.if (MUSICDATA & $ff) != 0
	.redefine MUSICDATA ((MUSICDATA&$ff00)+$100)
	.endif
	; SPCドライバソースを挿入する
	.rept 4
	.fread fp data
	.endr
	.dw LOC
	.rept size-34
	.fread fp data
	.db data
	.endr
	.fclose fp
	.undef size,data
.endm

.bank 1
.org 0
.section "SPCDRIVER" semifree
.define ESA 0
.define TBL 0
.define LOC 0
.define MUSICDATA 0
SPCDriver:
	;ReadDriver "../spc/SuperC.bin"
	;.incbin "hc.bin"
	.incbin "x.bin"
.ends

