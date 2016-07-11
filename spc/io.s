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
	.dw   NULL                            ; 06h ... 
	.dw   NULL                            ; 07h ...
	.dw   NULL                            ; 08h ... 
.ends


.section "trans" free
.define syncCounter $00
.define distAddr    $01
.define brrIndex    $03

transMusic:
	waitSnes
	ret


.undef syncCounter, distAddr, brrIndex
.ends



.section "stub" free
SE:
setMasterVolume:
setMusicVolume:
setSoundVolume:
	mov	a, #0
	ret
.ends
