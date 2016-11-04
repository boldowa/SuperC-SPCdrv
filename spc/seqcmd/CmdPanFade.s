/**
 * SuperC SeqCmd Pan Fade
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum	$01
	panSig	db
	panSub	db
.ende


CmdPanFade:
	call	readSeq
	mov	track.panFadeSpan+x, a
	call	readSeq
	mov	y, a
	mov	a, track.panH+x
	and	a, #$3f
	mov	panSub, a
	mov	a, y
	and	a, #$3f
	setc
	sbc	a, panSub
	push	psw
	bcs	+
	eor	a, #$ff
	inc	a
+	mov	y, a

	mov	a, track.panFadeSpan+x
	call	DivDuration

	pop	psw
	bcs	+
	eor	a, #$ff
	inc	a
+	mov	track.panFadeDtL+x, a
	mov	a, $00
	bcs	+
	eor	a, #$ff
+	mov	track.panFadeDtH+x, a
	jmp	CmdPanShare


;------------------------------
; local values undefine
;------------------------------
.undefine	panSig
.undefine	panSub


