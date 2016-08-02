/**
 * SuperC N-Format RAM DEFINES
 *
 *
 */

.ifndef _RAM_INC
.define _RAM_INC

.ifndef _LIB
.define EMPTY $ff
.emptyfill EMPTY
.endif

;------------------------------------------------
; DEFINES...
;------------------------------------------------
.define MUSICTRACKS 10			; 音楽で使用するトラック数
.define SETRACKS 3			; 総トラックのうち、効果音で使用するトラック数
.define TRACKS (MUSICTRACKS + SETRACKS)	; 音楽と効果音で使用する総トラック数


.include "ramstr.inc"
.include "bflags.inc"

;------------------------------------------------
; RAM
;------------------------------------------------

;------------------------------
; DirectPage Pg0
;------------------------------
;     |------------ドラム缶-----------|
;     +---+---+---+---+---+---+---+---+
; $00 |                               |  ※テンポラリは $00 ~ $03 : ローカル変数
;  :  |        テンポラリ予約         |                 $04 ~ $07 : 引数
; $07 |                               |                 として使用、共通で使用する値は$08移行に格納すること
;     +---+---+---+---+---+---+---+---+
; $08 |                               |
;  :  |    LV1:メインルーチン予約     |
; $0f |                               |
;     +---+---+---+---+---+---+---+---+
; $10 |                               |
;  :  |  LV2:メインサブルーチン予約   |
; $17 |                               |
;     +---+---+---+---+---+---+---+---+

.enum $18
	;+----------------------------+
	;| rootSysFlags               |
	;|   bit0: DSP更新の有無      |
	;|   bit1: 効果音解析中       |
	;|   bit2:                    |
	;|   bit3:                    |
	;|   bit4:                    |
	;|   bit5:                    |
	;|   bit6:                    |
	;|   bit7:                    |
	;+----------------------------+
	rootSysFlags			db	; システムフラグ(音楽・効果音共通部)

	taskCounter			db	; 処理を回す回数
	sndTempoCounter			db	; 効果音のテンポカウンタ
	musicTempoCounter		db	; 音楽テンポカウンタ
	musicTempo			db	; 音楽テンポ
	musicGlobalVolume		db	; 音楽ボリューム
	eVolRatio			db	; エコー音量
	seChNums			db	; 効果音に割り当て可能なCH数
	ZERO				dw	; 恒久的に0を持つ2バイト領域
	seqBaseAddress			dw	; シーケンスデータのベースアドレス(データ開始位置)
;----------------------------------------
; DSPメモリ更新バッファ
;----------------------------------------
	buf_chData	INSTANCEOF stChannel 8
	buf_echo	db
	buf_eVol	db
	buf_noise	db
	buf_keyon	db
	buf_keyoff	db

;----------------------------------------
; Page0識別末端
; ※削除禁止
;----------------------------------------
	PAGE0TAIL                   db
;----------------------------------------
.ende


;------------------------------
; DirectPage Pg1
;------------------------------
;     |------------ドラム缶-----------|
;     +---+---+---+---+---+---+---+---+
; $00 |                               |
;  :  |        テンポラリ予約         |
; $08 |                               |
;     +---+---+---+---+---+---+---+---+
.enum $08
;----------------------------------------
; サブルーチンコマンド用変数
; 1回だけネスト可能
;----------------------------------------
	loopStartAdrL	ds TRACKS*2
	loopStartAdrH	ds TRACKS*2
	loopReturnAdrL	ds TRACKS*2
	loopReturnAdrH	ds TRACKS*2
	loopNumbers	ds TRACKS*2


;----------------------------------------
; Page1識別末端
; ※削除禁止
;----------------------------------------
	PAGE1TAIL                   db                  ; ページ1末端識別用(コンパイラで使用)
;----------------------------------------
.ende

;--------------------------------------------------
; ダイレクトページRAMチェック
;--------------------------------------------------
; ページ０
.if PAGE0TAIL > $f0
	.printt "[ ERROR ] ダイレクトページ０の定義範囲オーバー(0x"
	.printv HEX PAGE0TAIL
	.printt ")\n"
	.fail
.endif

; ページ１
.if PAGE1TAIL > $f0
	.printt "[ ERROR ] ダイレクトページ１の定義範囲オーバー(0x"
	.printv HEX PAGE1TAIL
	.printt ")\n"
	.fail
.endif
.if PAGE1TAIL > $e0
	.printt "[ WARNING ] スタックに使えるRAMが32バイト以下です(0x"
	.printv HEX PAGE1TAIL
	.printt ")\n"
.endif


;--- RAM $200~
.enum $0200
;+----------------------------+
;| musicSysFlags              |
;|   bit0: ポーズ中か         |
;|   bit1: ヨッシーフラグ     |
;|   bit2: Panpot OFF         |
;|   bit3:                    |
;|   bit4:                    |
;|   bit5:                    |
;|   bit6:                    |
;|   bit7:                    |
;+----------------------------+
	musicSysFlags	db

;----------------------------------------
; track情報
;----------------------------------------
	track INSTANCEOF stTrack

;+----------------------------+
;|End RAM / Program Area Start|
;+----------------------------+
  __CODE_START__      db  
.ende


;+----------------------------+
;| define RAM area size       |
;|   ( for Bank Map )         |
;+----------------------------+
.define __RAMSIZE__ __CODE_START__-$200

.endif
