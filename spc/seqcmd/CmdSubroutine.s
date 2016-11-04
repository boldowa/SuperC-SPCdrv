/**
 * SuperC SeqCmd Subroutine
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum	$00
	XXXXX		db
.ende
;------------------------------
; Level1 val
;------------------------------
.define		lSeqPointer	$08


; --- ループ開始アドレスのセット
CmdSubroutine:
	push	x
	mov	a, x
	asl	a
	mov	x, a
	call	readSeq
	mov	y, a

	mov	a, (loopStartAdrH+$100)+x
	beq	+			; 現在ループ設定が1つもないなら次をスキップ

	inc	x			; 現在ループあり(ネスト)

	; --- ループ回数のセット
+	mov	a, y
	mov	(loopNumbers+$100)+x, a

	; --- ルーチンアドレス読み出し
	call	readSeq
	push	a
	call	readSeq
	mov	y, a

	; --- 戻りアドレスのセット
	mov	a, lSeqPointer
	mov	(loopReturnAdrL+$100)+x, a
	mov	a, lSeqPointer+1
	mov	(loopReturnAdrH+$100)+x, a

	; --- ルーチン開始アドレスのセット
	pop	a
	call	SetRelativePointer
	mov	(loopStartAdrL+$100)+x, a
	mov	a, y
	mov	(loopStartAdrH+$100)+x, a
	
+	pop	x
	ret


; --- ループを抜ける
CmdSubroutineReturn:
	push	x
	setp
	call	checkLoopNums
	beq	_subReturn		; 規定回数ループしている場合はループを抜ける

	; --- 残ループがある場合、ループ開始アドレスに戻す
	dec	loopNumbers+x
	mov	a, loopStartAdrH+x
	mov	y, a
	mov	a, loopStartAdrL+x
	bra	_subShareCode

_subReturn:
	mov	loopStartAdrH+x, a
	mov	a, loopReturnAdrH+x
	mov	y, a
	mov	a, loopReturnAdrL+x
_subShareCode:
	pop	x
	clrp
	movw	lSeqPointer, ya
;	stop
	ret


; --- 最終ループ時のみ処理を抜ける
CmdSubroutineBreak:
	push	x
	setp
	call	checkLoopNums
	beq	_subReturn
	clrp
	pop	x
	ret				; 残ループがある場合は何もしない


checkLoopNums:
	mov	a, x
	asl	a
	mov	x, a
	inc	x
	mov	a, loopStartAdrH+x
	bne	+
	dec	x
+	mov	a, loopNumbers+x	; ループ残り回数チェック
	ret


;------------------------------
; local values undefine
;------------------------------
.undefine	lSeqPointer


