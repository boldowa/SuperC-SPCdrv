/**
 * SuperC SeqCmd Global Volume Fade
 *
 *
 */

;------------------------------
; local values
;------------------------------
.enum	$00
.ende


CmdGlobalVolumeFade:
	call	readSeq
	mov	mgvolFadeSpan, a
	call	readSeq
	setc
	sbc	a, musicGlobalVolume
	push	psw
	bcs	+
	eor	a, #$ff
	inc	a
+	mov	y, a

	mov	a, mgvolFadeSpan

	call	DivDuration
	pop	psw
	bcs	+
	eor	a, #$ff
	inc	a
+	mov	mgvolFadeDtL, a
	mov	a, $00
	bcs	+
	eor	a, #$ff
+	mov	mgvolFadeDtH, a
	mov	musicGlobalVolumeL, #0
	ret


;------------------------------
; local values undefine
;------------------------------


