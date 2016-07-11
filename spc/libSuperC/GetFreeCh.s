/**
 * SuperC Get free channel
 *
 *
 */

.section "GET_FREE_CHANNEL" free
;------------------------------
; local values
;------------------------------
.enum $00
	lTrackIndex	db
.ende

GetFreeCh:
	mov	lTrackIndex, x

	; 割り当て済Chを検索
	mov	y, #0
	mov	x, #0
-	mov	a, buf_chData.allocTrack+x
	cmp	a, lTrackIndex
	beq	_FindCh				; 割り当て済みチャンネル
	mov	a, x
	clrc
	adc	a, #_sizeof_stChannel
	mov	x, a
	inc	y
	cmp	y, #8				; 8CH分ループ
	bmi	-

	; 空きChを検索
	mov	y, #0
	mov	x, #0
-	mov	a, buf_chData.allocTrack+x
	bmi	_FindCh				; 空きチャンネル
	mov	a, x
	clrc
	adc	a, #_sizeof_stChannel
	mov	x, a
	inc	y
	cmp	y, #8				; 8CH分ループ
	bmi	-

	; 使用可能チャンネルなし
	mov1	c, rootSysFlags.1		; 効果音解析中?
	bcc	+

	; 8CHから効果音用CHに使えるCHを探す
	mov	x, #(_sizeof_stChannel * 7)
	mov	y, #0
-	cmp	y, seChNums			; 効果音で上書きできるCH数
	beq	+
	inc	y
	mov	a, buf_chData.allocTrack+x
	cmp	a, #MUSICTRACKS
	bmi	_OverwriteCh
	mov	a, x
	setc
	sbc	a, #_sizeof_stChannel
	mov	x, a
	bra	-
+	mov	a, #$ff
	bra	++

_OverwriteCh:
	mov	a, y
	eor	a, #$ff
	inc	a
	clrc
	adc	a, #8
	bra	++

_FindCh:
	mov	a, y

++	mov	y, lTrackIndex
	ret					; a <- ChNum, x <- chBufferHead, y <- track

;------------------------------
; local values undefine
;------------------------------
.undefine	lTrackIndex

.ends

