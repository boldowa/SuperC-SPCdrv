/**
 * SuperC 16bit MUL Function
 *
 *
 */

.section "MUL_FUNC1" free
;------------------------------
; local values
;------------------------------
.enum $00
	lval	dw
	lval2	db
.ende
.enum $04
	arg	db
.ende


;------------------------------
; 16bit * 8bit ‰‰ŽZ
;   a   : mul value
;   y   : 16bit arg(lower 8)
;   arg : 16bit arg(upper 8)
;
; returns(24bit value) : 
;   ya          : upper 16bit val
;   $00         : lower 8bit val
;------------------------------
mul16_8:
	push	a
	mul	ya
	movw	lval, ya
	pop	y
	mov	a, arg
	mul	ya
	mov	lval2, #0
	clrc
	addw	ya, lval+1
	ret

;------------------------------
; local values undefine
;------------------------------
.undefine	lval
.undefine	lval2
.undefine	arg

.ends


.section "MUL_FUNC2" free
;------------------------------
; local values
;------------------------------
.enum $00
	lval	dw
	lval2	dw
.ende
.enum $04
	argmul	dw
.ende


;------------------------------
; 16bit * 16bit ‰‰ŽZ
;   ya     : value
;   argmul : mul value
;   arg    : 16bit arg(upper 8)
;
; returns(32bit value) : 
;   ya          : upper 16bit val
;   $00         : lower 16bit val
;------------------------------
mul16_16:
	push	y
	push	y
	push	a
	mov	y, a
	mov	a, argmul
	mul	ya
	movw	lval, ya
	pop	y
	mov	a, argmul+1
	mul	ya
	mov	lval2, #0
	clrc
	addw	ya, lval+1
	movw	lval+1, ya
	pop	y
	mov	a, argmul
	mul	ya
	mov	lval2+1, #0
	clrc
	addw	ya, lval+1
	bcc	+
	inc	y
+	movw	lval+1, ya
	pop	y
	mov	a, argmul+1
	mul	ya
	clrc
	addw	ya, lval2
	ret

;------------------------------
; local values undefine
;------------------------------
.undefine	lval
.undefine	lval2
.undefine	argmul

.ends

