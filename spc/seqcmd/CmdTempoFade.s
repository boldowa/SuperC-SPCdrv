/**
 * SuperC SeqCmd Tempo Fade
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum	$00
.ende


CmdTempoFade:
	call	readSeq
	mov	tempoFadeSpan, a
	call	readSeq
	setc
	sbc	a, musicTempo
	push	psw
	bcs	+
	eor	a, #$ff
	inc	a
+	mov	y, a

	; --- TempoFadeÇÇ©ÇØÇÈticksÇèúêîÇ…ÉZÉbÉg
	mov	a, tempoFadeSpan

	call	DivDuration
	pop	psw
	bcs	+
	eor	a, #$ff
	inc	a
+	mov	tempoFadeDeltaL, a
	mov	a, $00
	bcs	+
	eor	a, #$ff
+	mov	tempoFadeDeltaH, a
	mov	musicTempoLo, #0
	ret


;------------------------------
; local values undefine
;------------------------------


