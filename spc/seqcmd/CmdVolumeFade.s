/**
 * SuperC SeqCmd XXXXX
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum	$00
.ende


CmdVolumeFade:
	call	readSeq
	mov	track.volumeFadeSpan+x, a
	call	readSeq
	setc
	sbc	a, track.volumeH+x
	push	psw
	bcs	+
	eor	a, #$ff
	inc	a
+	mov	y, a

	mov	a, track.volumeFadeSpan+x

	call	DivDuration
	pop	psw

	bcs	+
	eor	a, #$ff
	inc	a
+	mov	track.volumeFadeDtL+x, a
	mov	a, $00
	bcs	+
	eor	a, #$ff
+	mov	track.volumeFadeDtH+x, a
	jmp	CmdVolShare


;------------------------------
; local values undefine
;------------------------------
.undefine	XXXXX


