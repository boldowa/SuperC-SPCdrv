/**
 * SuperC SeqCmd Portamento
 *
 *
 */


CmdPortamentoOn:
	mov	a, #TRKFLG_PORTAM
	or	a, track.bitFlags+x
	mov	track.bitFlags+x, a
	ret

CmdPortamentoOff:
	mov	a, #(TRKFLG_PORTAM ~ $ff)
	and	a, track.bitFlags+x
	mov	track.bitFlags+x, a
	ret

