/**
 * SuperC brr Table
 *
 *
 */

.incdir  "./include/"
.include "spc.inc"
.include "var.inc"

.incdir  "./macro/"
.include "readbrr.mac"

.bank 3 slot 3
;.orga (DIR<<8)

.section "__DIR__" size $180 align 256 free
DirTbl:
; --- DIR情報
;     ※注 : BRR実データと読み込み順を合わせること
	.dw	SPWAV0, SPWAV0+9
;	.dw	SPWAV1, SPWAV1+9
	DirInfo "./brr/p0.brr"	
	DirInfo "./brr/p1.brr"	
	DirInfo "./brr/poke-rgb_tri.brr"	
	DirInfo "./brr/kick.brr"
	DirInfo "./brr/snare.brr"
	DirInfo "./brr/pi_d6.brr"
;	DirInfo "./brr/smwpiano.brr"
.ends

.orga (DIR<<8 + $100)
.section "BRR" force
BrrData:
; --- BRR実データ
;     ※注 : DIR情報と読み込み順を合わせること
	BrrData "./brr/p0.brr"	
	BrrData "./brr/p1.brr"	
	BrrData "./brr/poke-rgb_tri.brr"	
	BrrData "./brr/kick.brr"
	BrrData "./brr/snare.brr"
;	BrrData "./brr/smwpiano.brr"
	BrrData "./brr/pi_d6.brr"
.ends


/**
 * 特殊波形生成機能
 */
.org 0
.section "SPECIALWAVE" free
SpecialWavFunc:
	mov	a, specialWavPtr
	mov	x, a
	mov	a, !SPWAV0+10+x
	eor	a, #$ff
	mov	!SPWAV0+10+x, a
;	mov	a, !SPWAV1+10+x
;	eor	a, #$ff
;	mov	!SPWAV1+10+x, a
	mov	a, specialWavPtr
	inc	a
	cmp	a, #8
	bne	+
	inc	a
+	cmp	a, #17
	bne	+
	mov	a, #0
+	mov	specialWavPtr, a
	ret

SPWAV0:
	.db	$02, $00, $00, $00, $00, $00, $00, $00, $00
	.db	$b2, $77, $77, $77, $77, $77, $77, $77, $77
	.db	$b3, $99, $99, $99, $99, $99, $99, $99, $99

/*
   ; 出力元波形を違うものにすれば面白いおとになるかな？と思ったけど
   ; SPWAV0と大差なかったので、コメントアウト
SPWAV1:
	.db	$02, $00, $00, $00, $00, $00, $00, $00, $00
	.db	$b2, $00, $11, $22, $33, $44, $55, $66, $77
	.db	$b3, $88, $99, $aa, $bb, $cc, $dd, $ee, $ff
SPWAV2:
	.db	$02, $00, $00, $00, $00, $00, $00, $00, $00
	.db	$c2, $01, $23, $45, $67, $76, $54, $32, $10
	.db	$c3, $fe, $dc, $ba, $98, $89, $ab, $cd, $ef
*/

.ends
