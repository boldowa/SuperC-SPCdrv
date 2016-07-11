/**
 * SuperC SYSTEM DEFINE
 *
 */


.ifndef _SPC_INC
.define _SPC_INC

;-------------------------------------------------
; RAM定義ファイル読込
;-------------------------------------------------
.include "ram.inc"

;-------------------------------------------------
; バンク設定
;-------------------------------------------------
.ifndef _LIB
.memorymap
; 初期スロット = 3 ( SPC700 プログラムRAM )
  defaultslot 3

; スロット0 : SPCファイルヘッダ
  slotsize 256
  slot 0 0

; スロット1 : SPC700 ダイレクトページRAM
  slotsize 512
  slot 1 0

; スロット2 : SPC700 ユーザーRAM
  slotsize __RAMSIZE__
  slot 2 512

; スロット3 : SPC700 プログラムRAM
  slotsize $fe00-__RAMSIZE__
  slot 3 __CODE_START__

; スロット4 : SPCファイル DSPレジスタエリア
  slotsize 256
  slot 4 $10000
.endme

.rombankmap
  bankstotal 5

; SPCファイルヘッダ
  banksize 256
  banks 1

; SPC700 ダイレクトページRAM
  banksize 512
  banks 1

; SPC700 ユーザーRAM
  banksize __RAMSIZE__
  banks 1

; SPC700 プログラムRAM
  banksize $fe00-__RAMSIZE__
  banks 1

; SPCファイル DSPレジスタエリア
  banksize 256
  banks 1

.endro

.bank 3 slot 3
.endif

.define NULL 0

;-------------------------------------------------
; spc dsp define
;-------------------------------------------------
.define DSP_VOL		$00
.define DSP_P		$02
.define DSP_SRCN	$04
.define DSP_ADSR	$05
.define DSP_GAIN	$07
.define DSP_ENVX	$08	; *
.define DSP_OUTX	$09	; *
.define DSP_MVOLL	$0c
.define DSP_MVOLR	$1c
.define DSP_EVOLL	$2c
.define DSP_EVOLR	$3c
.define DSP_KON		$4c
.define DSP_KOF		$5c
.define DSP_FLG		$6c
.define DSP_ENDX	$7c	; *
.define DSP_EFB		$0d
.define DSP_PMON	$2d
.define DSP_NON		$3d
.define DSP_EON		$4d
.define DSP_DIR		$5d
.define DSP_ESA		$6d
.define DSP_EDL		$7d
.define DSP_FIR		$0F

;-------------------------------------------------
; spc register define
;-------------------------------------------------
.define SPC_TEST	$f0
.define SPC_CONTROL	$f1
.define SPC_REGADDR	$f2
.define SPC_REGDATA	$f3
.define SPC_PORT0	$f4
.define SPC_PORT1	$f5
.define SPC_PORT2	$f6
.define SPC_PORT3	$f7
.define SPC_TIMER0	$FA
.define SPC_TIMER1	$FB
.define SPC_TIMER2	$FC
.define SPC_COUNTER0	$FD
.define SPC_COUNTER1	$FE
.define SPC_COUNTER2	$FF

;-------------------------------------------------
; spc flug define
;-------------------------------------------------
.define FLG_ECEN	$20
.define FLG_MUTE	$40
.define FLG_RES		$80

;-------------------------------------------------
; spc control define
;-------------------------------------------------
.define CNT_ST0     $01
.define CNT_ST1     $02
.define CNT_ST2     $04
.define CNT_PC10    $10
.define CNT_PC32    $20
.define CNT_IPL     $80


.endif
