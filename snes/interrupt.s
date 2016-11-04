/**
 * SuperC SPC Player Interrupt Handler
 */
.incdir "include"
.include "map.inc"
.include "header.inc"
.incdir ""

.bank 0
.section "EMPTYHANDLER" semifree
EmptyHandler:
	rti
.ends

