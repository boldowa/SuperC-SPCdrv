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
.orga (DIR<<8)

.section "__DIR__" force
; --- DIR情報
;     ※注 : BRR実データと読み込み順を合わせること
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

