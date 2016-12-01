/**
 * SuperC snes ram define
 */

.ifndef _RAM_INC
.define _RAM_INC

;-------------------------------------------------
; 音楽関連RAM
;-------------------------------------------------
.enum $7f0000
	musicTransportFlag	db
	musicNumber		db
	previousMusicNumber	db
	brrSet			db
	previousBrrSet		db
.ende
.endif /* _RAM_INC */
