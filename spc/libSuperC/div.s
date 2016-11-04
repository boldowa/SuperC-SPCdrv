/**
 * SuperC Division
 *
 *
 */

.section "DURATION_DIVISION" free
;------------------------------
; local values
;------------------------------
.enum $00
	upper	db
.ende


;------------------------------
; Duration -> 1tickñàÇÃïœâªíléZèo 
;   y   : Diff
;   a   : Duration(ticks)
;
; returns : 
;   a          : lower 8bit val
;   $00        : upper 8bit val
;------------------------------
DivDuration:
	push	x
	mov	x, a
	mov	a, y
	mov	y, #0
	div	ya, x
	mov	upper, a
	mov	a, #0
	div	ya, x
	pop	x
	ret


;------------------------------
; local values undefine
;------------------------------
.undefine	upper

.ends

