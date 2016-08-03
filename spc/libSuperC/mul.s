/**
 * SuperC 16bit MUL Func
 *
 *
 */

.section "MUL_FUNC" free
;------------------------------
; local values
;------------------------------
.enum $00
	lval	dw
.ende
.enum $04
	arg	db
	ret_24	db
.ende


;------------------------------
; 16bit * 8bit ‰‰ŽZ
;   a   : mul value
;   y   : 16bit arg(lower 8)
;   arg : 16bit arg(upper 8)
;
; returns : 
;   ya          : lower 16bit val
;   ret_24($05) : upper 8bit val
;------------------------------
mul16_8:
	push	a
	mul	ya
	movw	lval, ya
	mov	a, arg
	mov	y, a
	pop	a
	mul	ya
	push	a
	mov	a, y
	mov	ret_24, a
	pop	a
	mov	y, #0
	clrc
	addw	ya, lval
	adc	ret_24, #0
	ret


;------------------------------
; local values undefine
;------------------------------
.undefine	lval
.undefine	arg
.undefine	ret_24

.ends

