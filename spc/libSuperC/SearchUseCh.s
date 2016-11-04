/**
 * SuperC Search Using Channel
 *
 *
 */

.section "SEARCH_CHANNEL" free
;------------------------------
; local values
;------------------------------
.define		lTrackIndex	$00

SearchUseCh:
	mov	lTrackIndex, x
	mov	x, #0
	mov	y, #0
-	mov	a, buf_chData.allocTrack+x
	cmp	a, lTrackIndex			; 現在解析中のtrackと一致する？
	beq	+				; 割り当てられているチャンネル情報を返却
	mov	a, x
	clrc
	adc	a, #_sizeof_stChannel
	mov	x, a
	inc	y
	cmp	y, #8				; 8CH分ループ
	bmi	-

	; 割り当てなし
	mov	a, #$ff
	bra	++

+	mov	a, y
++	mov	y, lTrackIndex
	ret					; a <- ChNum, x <- chBufferHead, y <- track

;------------------------------
; local values undefine
;------------------------------
.undefine	lTrackIndex

.ends

