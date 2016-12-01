/**
 * SuperC SPC Player Main
 */
.incdir "include"
.include "map.inc"
.include "header.inc"
.incdir ""

.bank 0
.org 0
.section "MAIN" force
;---------------------------------------
; mini-snsf対応用
;   "00 00 00 00 00 00 00 01 xx"の
;   データのmini-snsfを作成することで、
;   読み出す曲番号を変更できるように
;   しておく。
;---------------------------------------
LoadMusicNumber:
	.db	1	; デフォルト曲番号: 1

Main:
	sei
	lda.b	#1
	sta.w	$4200
	sta.w	$420d
	stz.w	SPC_PORT0
	stz.w	SPC_PORT1
	stz.w	SPC_PORT2
	stz.w	SPC_PORT3
	lda.b	#$80
	sta.w	$2100

	; エミュレーションフラグの解除
	clc
	xce

	rep	#$38
	lda.w	#$0000
	tcd
	lda.w	#$01ff
	tcs

	;phk
	;plb

	lda.w	#SPCDriver
	sta.b	Scratch
	sep	#$30
	lda.b	#:SPCDriver
	sta.b	Scratch+2
	jsr	UploadSPCDriver

	lda.w	LoadMusicNumber
	jsr	TransportMusic

	rep	#4			; bit2 clear ... 割り込み許可
	cli				; 割り込み許可
	lda.b	#$80
	sta.w	$4200
-	bra	-

.ends

